/*
 *  Copyright 2011 The OpenMx Project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "R.h"
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Rdynload.h>
#include <mpi.h>

#include "init_finalize.h"
#include "commands.h"
#include "state.h"


// Helper functions
void sendFunctionName(SEXP functionName);
void sendRemainder(SEXP serializeRemainder);
void sendArgCounts(int *argcounts, int supervisorWorkCount, int numArgs);
void sendRawByteCounts(int *rawByteCounts, int *argcounts, SEXP serializeArgs, size_t *totalLength);
void generateRawByteDisplacements(int *rawByteDisplacements, int *rawByteCounts);
void sendArgRawBytes(unsigned char *argRawBytes, int *rawByteDisplacements, int *rawByteCounts,
    unsigned char *receiveBuffer, SEXP serializeArgs);

void sendFunctionName(SEXP functionName) {
   SEXP functionNameExpression = PROTECT(STRING_ELT(functionName, 0));
   char* functionString = (char*) CHAR(functionNameExpression);
   int nameLength = strlen(functionString) + 1;

   MPI_Bcast(&nameLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast((void*) functionString, nameLength, MPI_CHAR, 0, MPI_COMM_WORLD);

   UNPROTECT(1); // STRING_ELT(functionName, 0)
}

void sendRemainder(SEXP serializeRemainder) {
   int remainderLength = LENGTH(serializeRemainder);
   MPI_Bcast(&remainderLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast((void*) RAW(serializeRemainder), remainderLength, MPI_BYTE, 0, MPI_COMM_WORLD);
}

void sendArgCounts(int *argcounts, int supervisorWorkCount, int numArgs) {
   int i, div, mod, offset;

   argcounts[0] = supervisorWorkCount;
   div = (numArgs - supervisorWorkCount) / (readonly_nproc - 1);
   mod = (numArgs - supervisorWorkCount) % (readonly_nproc - 1);
   offset = 1 + mod;
   for(i = 1; i < offset; i++) {
      argcounts[i] = div + 1;
   }
   for(i = offset; i < readonly_nproc; i++) {
      argcounts[i] = div;
   }

   MPI_Scatter(argcounts, 1, MPI_INT, &supervisorWorkCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

void sendRawByteCounts(int *rawByteCounts, int *argcounts, SEXP serializeArgs, size_t *totalLength) {
   size_t currentLength = 0, offset = 0;
   int supervisorByteCount;
   int i, proc, current = 1;
   int numArgs = LENGTH(serializeArgs);
   int supervisorWorkCount = argcounts[0];
   SEXP arg;

   *totalLength = 0;
   proc = (supervisorWorkCount == 0) ? 1 : 0;
   for(i = 0; i < numArgs; i++) {
      PROTECT(arg = VECTOR_ELT(serializeArgs, i));
      offset = LENGTH(arg);
      *totalLength += offset;
      currentLength += offset;
      if(argcounts[proc] == current) {
         rawByteCounts[proc] = currentLength;
         currentLength = 0;
         current = 1;
         proc++;
      } else {
         current++;
      }
      UNPROTECT(1); // VECTOR_ELT(serializeArgs, i)
   }

   MPI_Scatter(rawByteCounts, 1, MPI_INT, &supervisorByteCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

void generateRawByteDisplacements(int *rawByteDisplacements, int *rawByteCounts) {
   int i;
   for(i = 1; i < readonly_nproc; i++) {
      rawByteDisplacements[i] = rawByteCounts[i - 1] + rawByteDisplacements[i - 1];
   }
}

void sendArgRawBytes(unsigned char *argRawBytes, int *rawByteDisplacements, int *rawByteCounts,
    unsigned char *receiveBuffer, SEXP serializeArgs) {

   int i, numArgs = LENGTH(serializeArgs);
   size_t offset, currentLength = 0;
   SEXP arg;

   for(i = 0; i < numArgs; i++) {
      PROTECT(arg = VECTOR_ELT(serializeArgs, i));
      offset = LENGTH(arg);
      memcpy(argRawBytes + currentLength, RAW(arg), offset);
      currentLength += offset;
      UNPROTECT(1); // VECTOR_ELT(serializeArgs, i)
   }

   MPI_Scatterv(argRawBytes, rawByteCounts, rawByteDisplacements, MPI_BYTE,
      receiveBuffer, rawByteCounts[0], MPI_BYTE, 0, MPI_COMM_WORLD);
}

SEXP lapplyPiebaldMPI(SEXP functionName, SEXP serializeArgs, 
      SEXP serializeRemainder) {

   checkPiebaldInit();


   int numArgs = LENGTH(serializeArgs);
   size_t totalLength;
   int *argcounts = Calloc(readonly_nproc, int);
   int *rawByteDisplacements = Calloc(readonly_nproc, int);
   int *rawByteCounts = Calloc(readonly_nproc, int);
   unsigned char *argRawBytes = NULL, *receiveBuffer = NULL;

   SEXP arg;
   int supervisorWorkCount, supervisorByteCount;

   int command = LAPPLY;
   MPI_Bcast(&command, 1, MPI_INT, 0, MPI_COMM_WORLD);

   sendFunctionName(functionName);

   sendRemainder(serializeRemainder);

   supervisorWorkCount = numArgs / readonly_nproc;

   sendArgCounts(argcounts, supervisorWorkCount, numArgs);

   sendRawByteCounts(rawByteCounts, argcounts, serializeArgs, &totalLength);

   generateRawByteDisplacements(rawByteDisplacements, rawByteCounts);
   
   argRawBytes = Calloc(totalLength, unsigned char);
   receiveBuffer = Calloc(rawByteCounts[0], unsigned char);

   sendArgRawBytes(argRawBytes, rawByteDisplacements, rawByteCounts,
    receiveBuffer, serializeArgs);

   Free(rawByteDisplacements);
   Free(argRawBytes);  
   Free(receiveBuffer);
   Free(rawByteCounts);
   Free(argcounts);

   return(R_NilValue);
}


void lapplyWorkerPiebaldMPI() {
   int nameLength, remainderLength;
   int workCount, bytesCount;
   char *functionName;
   SEXP serializeRemainder;

   MPI_Bcast(&nameLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
   functionName = Calloc(nameLength, char);
   MPI_Bcast((void*) functionName, nameLength, MPI_CHAR, 0, MPI_COMM_WORLD);

   MPI_Bcast(&remainderLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
   PROTECT(serializeRemainder = allocVector(RAWSXP, remainderLength));
   MPI_Bcast((void*) RAW(serializeRemainder), remainderLength, MPI_BYTE, 0, MPI_COMM_WORLD);

   MPI_Scatter(NULL, 0, MPI_INT, &workCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

   MPI_Scatter(NULL, 0, MPI_INT, &bytesCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

   Rprintf("Work count %d, bytes count %d\n", workCount, bytesCount);

   UNPROTECT(1);
   Free(functionName);  
}