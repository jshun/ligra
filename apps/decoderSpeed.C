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
#elif defined BPSIMD
#include "bitpacking_vec.h"
#elif defined VARINTGB
#include "varintGB.h"
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
#elif defined BPSIMD
#include "bitpacking_vec.h"
#elif defined VARINTGB
#include "varintGB.h"
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
  uintE* edges;
  uintT* offsets;
  F(uintE* _edges, uintT* _offsets) : edges(_edges), offsets(_offsets) {}
  inline bool srcTarg(uintE src, uintE target, uintT edgeNumber) {
    edges[offsets[src]+edgeNumber] = target;
    return true;
  }
};

void decodeGraph(graph<compressedSymmetricVertex> G, uintE* edges, uintT* offsets) {
  compressedSymmetricVertex *V = G.V;
  parallel_for (long i = 0; i < G.n; i++) {
	cout << "in loop " << G.n <<  endl;
    uchar *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
    decode(F(edges,offsets), nghArr, i, d);
  }
}

void copyGraph(graph<symmetricVertex> G, uintE* edges, uintT* offsets) {
  symmetricVertex *V = G.V;
  parallel_for(long i=0; i < G.n; i++) {
    for(long j=0; j < V[i].getOutDegree(); j++) {
      edges[offsets[i]+j] = V[i].neighbors[j];
    }
  }
}

//Tests the speed of decoding the entire graph. Currently works for
//unweighted graphs. For compressed graphs, pass the "-c" flag.
int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv,"[-w] <inFile>");
  char* iFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");
  long rounds = P.getOptionLongValue("-rounds",5);
  bool compressed = P.getOptionValue("-c");
  bool binary = P.getOptionValue("-b");
  srand(0);
  if(compressed) {
    graph<compressedSymmetricVertex> G = readCompressedGraph<compressedSymmetricVertex>(iFile,1,0);
    uintE* edges = newA(uintE,G.m);
    uintT* offsets = newA(uintT,G.n);
    parallel_for(long i=0;i<G.n;i++) offsets[i] = G.V[i].getOutDegree(); 
    sequence::plusScan(offsets, offsets, G.n);
    timer t;
    decodeGraph(G,edges,offsets);
    for(long i=0;i<rounds;i++) {
      t.start();
      decodeGraph(G,edges,offsets);
      t.reportTotal("decoding time");
    }
    cout << edges[rand() % G.m] << endl;
    free(offsets);
    free(edges);
    G.del();
  } else {
    graph<symmetricVertex> G =
      readGraph<symmetricVertex>(iFile,0,1,binary,0); //symmetric graph
    uintE* edges = newA(uintE,G.m);
    uintT* offsets = newA(uintT,G.n);
    parallel_for(long i=0;i<G.n;i++) offsets[i] = G.V[i].getOutDegree(); 
    sequence::plusScan(offsets, offsets, G.n);
    timer t;
    copyGraph(G,edges,offsets);
    for(long i=0;i<rounds;i++) {
      t.start();
      copyGraph(G,edges,offsets);
      t.reportTotal("decoding time");
    }
    cout << edges[rand() % G.m] << endl;
    free(offsets);
    free(edges);
    G.del();
  }
}
