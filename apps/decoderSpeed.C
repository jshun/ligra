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
#elif defined STREAMVBYTE
#include "streamvbyte.h"
#elif defined STREAMCASE
#include "streamvbyte_256.h"
#elif defined BP
#include "bitpacking.h"
#else
#include "byteRLE.h"
#endif
#else //decode in parallel
#ifdef BYTE
#include "byte-pd.h"
#elif defined NIBBLE
#include "nibble-pd.h"
#elif defined STREAMVBYTE
#include "streamvbyte.h"
#elif defined STREAMCASE
#include "streamvbyte_256.h"
#elif defined BP
#include "bitpacking.h"
#else
#include "byteRLE-pd.h"
#endif
#endif

#include <stdlib.h>
#include "parallel.h"
#include "utils.h"
#include "graph.h"
#include "parseCommandLine.h"
#include "IO.h"
#include "gettime.h"
using namespace std;

struct F {
  inline bool srcTarg(uintE src, uintE target, uintT edgeNumber) {
    return true;
  }
  inline bool srcTarg(uintE src, uintE target, intE weight, uintT edgeNumber) {
    return true;
  }
};

void decodeGraph(graph<compressedSymmetricVertex> G, bool weighted) {
  compressedSymmetricVertex *V = G.V;
  parallel_for (long i = 0; i < G.n; i++) {
    uchar *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
    if(weighted)
      decodeWgh(F(), nghArr, i, d);
    else
      decode(F(), nghArr, i, d);
  }
}

//Converts binary compressed graph to text format. For weighted
//graphs, pass the "-w" flag.
int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv,"[-w] <inFile>");
  char* iFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");
  graph<compressedSymmetricVertex> G = readCompressedGraph<compressedSymmetricVertex>(iFile,1,0);
  timer t;
  decodeGraph(G,weighted);
  t.start();
  decodeGraph(G,weighted);
  t.reportTotal("decoding time");
}
