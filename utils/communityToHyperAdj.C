// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"

//pass -w flag for weighted input
int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-w] <input hyperedge file> <output Ligra file>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  bool wgh = P.getOption("-w");
  if(!wgh) {
    hyperedgeArray<uintT> G = readHyperedges<uintT>(iFile);
    writeHypergraphToFile<uintT>(hypergraphFromHyperedges(G),oFile);
  } else {
    wghHyperedgeArray<uintT> G = readWghHyperedges<uintT>(iFile);
    writeWghHypergraphToFile<uintT>(wghHypergraphFromWghHyperedges(G),oFile);
  }
}
