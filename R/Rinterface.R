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

pbInit <- function() {
   invisible(.Call("initPiebaldMPI", PACKAGE = "PiebaldMPI"))
   if(getRank() > 0) {
      quit(save = "no")
   }
}

pbFinalize <- function() {
   invisible(.Call("finalizePiebaldMPI", PACKAGE = "PiebaldMPI"))
}

getRank <- function() {
   return(.Call("getrankPiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbSize <- function() {
   return(.Call("getsizePiebaldMPI", PACKAGE = "PiebaldMPI"))
}

pbLapply <- function(X, FUN, ...) {
   rank <- getRank()
   nproc <- pbSize()
   if (rank > 0 || nproc < 2) {
      return(lapply(X, FUN, ...))
   }
   argLength <- as.integer(length(X))
   serializeArgs <- serializeInput(X, nproc) 
   serializeFun <- serialize(FUN, NULL)
   serializeRemainder <- serialize(list(...), connection = NULL)
   results <- .Call("lapplyPiebaldMPI", serializeFun, serializeArgs, 
      serializeRemainder, argLength, PACKAGE = "PiebaldMPI")
   return(results)
}
