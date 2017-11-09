// This code is part of the project "Smaller and Faster: Parallel
// Processing of Compressed Graphs with Ligra+", presented at the IEEE
// Data Compression Conference, 2015.
// Copyright (c) 2015 Julian Shun, Laxman Dhulipala and Guy Blelloch
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

//include the code for the desired compression scheme
#ifndef PD 
#ifdef BYTE
#include "byte.h"
#elif defined NIBBLE
#include "nibble.h"
#else
#include "byteRLE.h"
#endif
#else //decode in parallel
#ifdef BYTE
#include "byte-pd.h"
#elif defined NIBBLE
#include "nibble-pd.h"
#else
#include "byteRLE-pd.h"
#endif
#endif

#include <sstream>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "parallel.h"
#include "utils.h"
#include "graph.h"
#include "parseCommandLine.h"
#include "IO.h"
using namespace std;

struct printAdjT {
  stringstream* ss;
  printAdjT(stringstream *_ss) : ss(_ss) {}
  bool srcTarg(uintE src, uintE target, uintT edgeNumber) {
    *ss << target << endl;
    return true;
  }
  bool srcTarg(uintE src, uintE target, intE weight, uintT edgeNumber) {
    *ss << target << endl;
    return true;
  }
};

struct printWghT {
  stringstream* ss;
  printWghT(stringstream *_ss) : ss(_ss) {}
  bool srcTarg(uintE src, uintE target, intE weight, uintT edgeNumber) {
    *ss << weight << endl;
    return true;
  }
};

void writeAdjGraph(graph<compressedSymmetricVertex> G, ofstream *of, bool weighted) {
  compressedSymmetricVertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    stringstream ss;
    uchar *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
    if(weighted)
      decodeWgh(printAdjT(&ss), nghArr, i, d);
    else
      decode(printAdjT(&ss), nghArr, i, d);
    *of << ss.str();
  }
  if(weighted) {
    cout << "writing weights..." << endl;
    for (long i = 0; i < G.n; i++) {
      stringstream ss;
      uchar *nghArr = V[i].getOutNeighbors();
      uintT d = V[i].getOutDegree();
      decodeWgh(printWghT(&ss), nghArr, i, d);
      *of << ss.str();    
    }
  }
}

//Converts binary compressed graph to text format. For weighted
//graphs, pass the "-w" flag. Note that if the original graph
//contained self-edges or duplicate edges, these would have been
//deleted by the encoder and will not be recreated by the decoder.
int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv,"[-w] <inFile> <outFile>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");
  graph<compressedSymmetricVertex> G = readCompressedGraph<compressedSymmetricVertex>(iFile,1,0);
  long n = G.n, m = G.m;

  ofstream out(oFile,ofstream::out);
  if(weighted) out << "WeightedAdjacencyGraph\n" << n << endl << m << endl;
  else out << "AdjacencyGraph\n" << n << endl << m << endl;
  cout<<"writing offsets..."<<endl;
  uintT* DegreesSum = newA(uintT,n);
  parallel_for(long i=0;i<n;i++) DegreesSum[i] = G.V[i].getOutDegree();
  sequence::plusScan(DegreesSum,DegreesSum,n);
  stringstream ss;
  setWorkers(1); //writing sequentially to file
  for(long i=0;i<n;i++) ss << DegreesSum[i] << endl;
  free(DegreesSum);
  out << ss.str();
  cout << "writing edges..."<<endl;
  writeAdjGraph(G,&out,weighted);
  out.close();
}
