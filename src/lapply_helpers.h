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

#ifndef _lapply_helpers_h
#define _lapply_helpers_h

#include "R.h"
#include <Rinternals.h>
#include <Rdefines.h>
#include <R_ext/Rdynload.h>

void sendFunctionName(SEXP functionName);
void sendRemainder(SEXP serializeRemainder);
void sendArgCounts(int *argcounts, int supervisorWorkCount, int numArgs);
void sendRawByteCounts(int *rawByteCounts, int *argcounts, SEXP serializeArgs, size_t *totalLength);
void generateRawByteDisplacements(int *rawByteDisplacements, int *rawByteCounts);
void sendArgDisplacements(int *argcounts, int *supervisorSizes, SEXP serializeArgs);
void sendArgRawBytes(unsigned char *argRawBytes, int *rawByteDisplacements, int *rawByteCounts,
    unsigned char *receiveBuffer, SEXP serializeArgs);

void evaluateLocalWork(SEXP functionName, SEXP serializeArgs, SEXP returnList, int count);

void receiveIncomingLengths(int *lengths);
void receiveIncomingSizes(int *lengths, int *sizes, int *displacements, int *argcounts, int *total);
void receiveIncomingData(unsigned char *buffer, int *lengths, int *displacements);
void processIncomingData(unsigned char *buffer, SEXP returnList, 
   int *sizes, int supervisorWorkCount, int numArgs);

#endif //_lapply_helpers_h
