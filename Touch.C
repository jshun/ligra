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
#include "ligra.h"
#include "parallel.h"
#include "parseCommandLine.h"
using namespace std;

template <class vertex>
void touch(graph<vertex> G) {
  vertex *V = G.V;
  {parallel_for (long i = 0; i < G.n; i++) {
    uintT d = V[i].getOutDegree();
    for(long j=0;j<d;j++)
      if(V[i].getOutNeighbor(j) == UINT_E_MAX) cout << "oops\n";
    }}
  {  parallel_for (long i = 0; i < G.n; i++) {
    uintT d = V[i].getInDegree();
    for(long j=0;j<d;j++)
      if(V[i].getInNeighbor(j) == UINT_E_MAX) cout << "oops\n";
    }}
}

int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  bool binary = P.getOptionValue("-b");
  long start = P.getOptionLongValue("-r",0);
  long rounds = P.getOptionLongValue("-rounds",3);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric,binary); //symmetric graph
    touch(G);
    for(int r=0;r<rounds;r++) {
      startTime();
      touch(G);
      nextTime("Touch graph");
    }
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary); //asymmetric graph
    touch(G);
    for(int r=0;r<rounds;r++) {
      startTime();
      touch(G);
      nextTime("Touch graph");
    }
    G.del();
  }
}
