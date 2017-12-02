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
  uintE** edges;
  F(uintE** _edges) : edges(_edges) {}

  inline bool srcTarg(uintE src, uintE target, uintT edgeNumber) {
    edges[src][edgeNumber] = target;
    return true;
  }
  inline bool srcTarg(uintE src, uintE target, intE weight, uintT edgeNumber) {
    edges[src][edgeNumber*2] = target;
    edges[src][edgeNumber*2+1] = target;
    return true;
  }
};

void decodeGraph(graph<compressedSymmetricVertex> G, bool weighted, uintE** edges) {
  compressedSymmetricVertex *V = G.V;
  parallel_for (long i = 0; i < G.n; i++) {
    uchar *nghArr = V[i].getOutNeighbors();
    uintT d = V[i].getOutDegree();
    if(weighted)
      decodeWgh(F(edges), nghArr, i, d);
    else
      decode(F(edges), nghArr, i, d);
  }
}

void copyGraph(graph<symmetricVertex> G, bool weighted, uintE** edges) {
  symmetricVertex *V = G.V;
  if(!weighted) {
    parallel_for(long i=0; i < G.n; i++) {
      for(long j=0; j < V[i].getOutDegree(); j++) {
	edges[i][j] = V[i].neighbors[j];
      }
    }
  } else {
    parallel_for(long i=0; i < G.n; i++) {
      for(long j=0; j < V[i].getOutDegree(); j++) {
	edges[i][2*j] = V[i].neighbors[2*j];
	edges[i][2*j+1] = V[i].neighbors[2*j+1];
      }
    }
  }
}

//Converts binary compressed graph to text format. For weighted
//graphs, pass the "-w" flag.
int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv,"[-w] <inFile>");
  char* iFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");
  long rounds = P.getOptionLongValue("-rounds",5);
  bool compressed = P.getOptionValue("-c");
  srand(0);
  if(compressed) {
    graph<compressedSymmetricVertex> G = readCompressedGraph<compressedSymmetricVertex>(iFile,1,0);
    uintE** edges = newA(uintE*,G.n);
    for(long i=0;i<G.n;i++) edges[i] = newA(uintE,G.V[i].getOutDegree());
    timer t;
    decodeGraph(G,weighted,edges);
    for(long i=0;i<rounds;i++) {
      t.start();
      decodeGraph(G,weighted,edges);
      t.reportTotal("decoding time");
    }
    uintE v = rand() % G.n;
    if(G.V[v].getOutDegree() > 0) cout << edges[v][rand() % G.V[v].getOutDegree()] << endl;
    else cout << "-1" << endl;
    for(long i=0;i<G.n;i++)
      free(edges[i]);
    free(edges);
    G.del();
  } else {
    graph<symmetricVertex> G =
      readGraph<symmetricVertex>(iFile,0,1,0,0); //symmetric graph
    uintE** edges = newA(uintE*,G.n);
    for(long i=0;i<G.n;i++) edges[i] = newA(uintE,G.V[i].getOutDegree());
    timer t;
    copyGraph(G,weighted,edges);
    for(long i=0;i<rounds;i++) {
      t.start();
      copyGraph(G,weighted,edges);
      t.reportTotal("decoding time");
    }
    uintE v = rand() % G.n;
    if(G.V[v].getOutDegree() > 0) cout << edges[v][rand() % G.V[v].getOutDegree()] << endl;
    else cout << "-1" << endl;
    for(long i=0;i<G.n;i++)
      free(edges[i]);
    free(edges);
    G.del();
  }
}
