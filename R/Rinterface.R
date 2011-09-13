#
#   Copyright 2011 The OpenMx Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#        http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

pbmpi_init <- function() {
   invisible(.Call("initPiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbmpi_finalize <- function() {
   invisible(.Call("finalizePiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbmpi_getrank <- function() {
   return(.Call("getrankPiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbmpi_getsize <- function() {
   return(.Call("getsizePiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbmpi_lapply <- function(X, FUN, ...) {
   rank <- pbmpi_getrank()
   nproc <- pbmpi_getsize()
   if (rank > 0 || nproc < 2) {
      return(lapply(X, FUN, ...))
   }
   serializeArgs <- lapply(X, serialize, connection = NULL)
   functionName <- as.character(match.call()$FUN)
   serializeRemainder <- serialize(list(...), connection = NULL)

   return(.Call("lapplyPiebaldMPI", functionName, serializeArgs, 
      serializeRemainder, PACKAGE = "PiebaldMPI"))

}