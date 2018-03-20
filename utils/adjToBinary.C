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

// Converts a Ligra graph in adjacency graph format into binary format

#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"
#include <iostream>
#include <sstream>
using namespace benchIO;
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-w] <inFile> <idxFile> <adjFile> <configFile>");
  char* iFile = P.getArgument(3);
  char* idxFile = P.getArgument(2);
  char* adjFile = P.getArgument(1);
  char* configFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");

  ofstream idx(idxFile, ofstream::out | ios::binary);
  ofstream adj(adjFile, ofstream::out | ios::binary);
  ofstream config(configFile, ofstream::out);
  if(!weighted) {
    graph<uintT> G = readGraphFromFile<uintT>(iFile);
    config << G.n;
    uintT* In = G.allocatedInplace;
    uintT* offsets = In+2;
    uintT* edges = In+2+G.n;
    idx.write((char*)offsets,sizeof(uintT)*G.n);
    if(sizeof(uintE) != sizeof(uintT)) {
      uintE* E = newA(uintE,G.m);
      parallel_for(long i=0;i<G.m;i++) E[i] = edges[i];
      adj.write((char*)E,sizeof(uintE)*G.m);
      free(E);
    } else {
      adj.write((char*)edges,sizeof(uintT)*G.m);
    }
    G.del();
  } else {
    wghGraph<uintT> G = readWghGraphFromFile<uintT>(iFile);
    config << G.n;
    uintT* In = G.allocatedInplace;
    uintT* offsets = In+2;
    uintT* edges = In+2+G.n;
    idx.write((char*)offsets,sizeof(uintT)*G.n);
    if(sizeof(uintE) != sizeof(uintT)) {
      intE* E = newA(intE,2*G.m);
      parallel_for(long i=0;i<2*G.m;i++) E[i] = edges[i];
      adj.write((char*)E,sizeof(intE)*2*G.m);
      free(E);
    } else {
      adj.write((char*)edges,sizeof(uintT)*2*G.m);  //edges and weights
    }
    G.del();
  }
  config.close();
  idx.close();
  adj.close();
}
