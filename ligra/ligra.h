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
#ifndef LIGRA_H
#define LIGRA_H
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <algorithm>
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "vertexSubset.h"
#include "common.h"
#include "graph.h"
#include "IO.h"
#include "parseCommandLine.h"
#include "gettime.h"

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

using namespace std;

//*****START FRAMEWORK*****

struct nonMaxF{bool operator() (uintE &a) {return (a != UINT_E_MAX);}};

//options to edgeMap for different versions of dense edgeMap (default is DENSE)
enum options { DENSE, DENSE_FORWARD};

//remove duplicate integers in [0,...,n-1]
void remDuplicates(uintE* indices, uintE* flags, long m, long n) {
  //make flags for first time
  if(flags == NULL) {flags = newA(uintE,n); 
    {parallel_for(long i=0;i<n;i++) flags[i]=UINT_E_MAX;}}
  {parallel_for(uintE i=0;i<m;i++)
      if(indices[i] != UINT_E_MAX && flags[indices[i]] == UINT_E_MAX) 
	CAS(&flags[indices[i]],(uintE)UINT_E_MAX,i);
  }
  //reset flags
  {parallel_for(long i=0;i<m;i++){
      if(indices[i] != UINT_E_MAX){
	if(flags[indices[i]] == i){ //win
	  flags[indices[i]] = UINT_E_MAX; //reset
	}
	else indices[i] = UINT_E_MAX; //lost
      }
    }
  }
}

//*****EDGE FUNCTIONS*****
namespace ligra_uncompressed {
  template <class vertex>
  bool* edgeMapDense(graph<vertex> GA, bool* vertexSubset, Edge_F &f, bool parallel = 0) {
  long numVertices = GA.n;
  vertex *G = GA.V;
  bool* next = newA(bool,numVertices);
  {parallel_for (long i=0; i<numVertices; i++){
    next[i] = 0;
    if (f.cond(i)) { 
      uintE d = G[i].getInDegree();
      if(!parallel || d < 1000) {
  for(uintE j=0; j<d; j++){
    uintE ngh = G[i].getInNeighbor(j);
#ifndef WEIGHTED
    if (vertexSubset[ngh] && f.update(ngh,i))
#else
    if (vertexSubset[ngh] && f.update(ngh,i,G[i].getInWeight(j)))
#endif
      next[i] = 1;
    if(!f.cond(i)) break;
  }
      } else {
  {parallel_for(uintE j=0; j<d; j++){
    uintE ngh = G[i].getInNeighbor(j);
#ifndef WEIGHTED
    if (vertexSubset[ngh] && f.update(ngh,i))
#else
    if (vertexSubset[ngh] && f.update(ngh,i,G[i].getInWeight(j)))
#endif
      next[i] = 1;
    }}
      }
    }
    }}
  return next;
}

  template <class vertex>
  bool* edgeMapDenseForward(graph<vertex> GA, bool* vertexSubset, Edge_F &f) {
    long numVertices = GA.n;
    vertex *G = GA.V;
    bool* next = newA(bool,numVertices);
    {parallel_for(long i=0;i<numVertices;i++) next[i] = 0;}
    {parallel_for (long i=0; i<numVertices; i++){
      if (vertexSubset[i]) {
        uintE d = G[i].getOutDegree();
        if(d < 1000) {
    for(uintE j=0; j<d; j++){
      uintE ngh = G[i].getOutNeighbor(j);
#ifndef WEIGHTED
      if (f.cond(ngh) && f.updateAtomic(i,ngh))
#else 
      if (f.cond(ngh) && f.updateAtomic(i,ngh,G[i].getOutWeight(j))) 
#endif
        next[ngh] = 1;
    }
        }
        else {
    {parallel_for(uintE j=0; j<d; j++){
      uintE ngh = G[i].getOutNeighbor(j);
#ifndef WEIGHTED
      if (f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
        if (f.cond(ngh) && f.updateAtomic(i,ngh,G[i].getOutWeight(j)))
#endif
      next[ngh] = 1;
      }}
        }
      }
      }}
    return next;
  }

  template <class vertex>
  pair<long,uintE*> edgeMapSparse(vertex* frontierVertices, uintE* indices, 
          uintT* degrees, uintT m, Edge_F &f, 
          long remDups=0, uintE* flags=NULL) {
    uintT* offsets = degrees;
    long outEdgeCount = sequence::plusScan(offsets, degrees, m);
    uintE* outEdges = newA(uintE,outEdgeCount);
    {parallel_for (long i = 0; i < m; i++) {
        uintT v = indices[i], o = offsets[i];
      vertex vert = frontierVertices[i]; 
      uintE d = vert.getOutDegree();
      if(d < 1000) {
        for (uintE j=0; j < d; j++) {
    uintE ngh = vert.getOutNeighbor(j);
#ifndef WEIGHTED
    if(f.cond(ngh) && f.updateAtomic(v,ngh)) 
#else
    if(f.cond(ngh) && f.updateAtomic(v,ngh,vert.getOutWeight(j)))
#endif
      outEdges[o+j] = ngh;
    else outEdges[o+j] = UINT_E_MAX;
        } 
      } else {
        {parallel_for (uintE j=0; j < d; j++) {
    uintE ngh = vert.getOutNeighbor(j);
#ifndef WEIGHTED
    if(f.cond(ngh) && f.updateAtomic(v,ngh)) 
#else
    if(f.cond(ngh) && f.updateAtomic(v,ngh,vert.getOutWeight(j)))
#endif
      outEdges[o+j] = ngh;
    else outEdges[o+j] = UINT_E_MAX;
    }} 
      }
      }}
    uintE* nextIndices = newA(uintE, outEdgeCount);
    if(remDups) remDuplicates(outEdges,flags,outEdgeCount,remDups);
    // Filter out the empty slots (marked with -1)
    long nextM = sequence::filter(outEdges,nextIndices,outEdgeCount,nonMaxF());
    free(outEdges);
    return pair<long,uintE*>(nextM, nextIndices);
  }
}

namespace ligra_compressed {
  struct denseT {
    bool* nextArr, *vertexArr;
  denseT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
#ifndef WEIGHTED
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (vertexArr[target] && f.update(target, src)) nextArr[src] = 1;
      return f.cond(src);
    }
#else
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (vertexArr[target] && f.update(target, src, weight)) nextArr[src] = 1;
      return f.cond(src);
    }
#endif
  };

  template <class vertex>
    bool* edgeMapDense(graph<vertex> GA, bool* vertexSubset, Edge_F &f) {  
    long numVertices = GA.n;
    vertex *G = GA.V;
    bool* next = newA(bool,numVertices);
    parallel_for (long i=0; i<numVertices; i++){
      next[i] = 0;
      if (f.cond(i)) { 
        uchar *nghArr = G[i].getInNeighbors();
#ifdef WEIGHTED
        decodeWgh(denseT(next, vertexSubset), f, nghArr, i, G[i].getInDegree());
#else
        decode(denseT(next, vertexSubset), f, nghArr, i, G[i].getInDegree());
#endif
      }
    }
    return next;
  }

  struct denseForwardT {
    bool* nextArr, *vertexArr;
  denseForwardT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
#ifndef WEIGHTED
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(src,target)) nextArr[target] = 1;
      return true;
    }
#else
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(src,target, weight)) nextArr[target] = 1;
      return true;
    }
#endif
  };

  template <class vertex>
    bool* edgeMapDenseForward(graph<vertex> GA, bool* vertexSubset, Edge_F &f) {
    long numVertices = GA.n;
    vertex *G = GA.V;
    bool* next = newA(bool,numVertices);
    {parallel_for(long i=0;i<numVertices;i++) next[i] = 0;}
    {parallel_for (long i=0; i<numVertices; i++) {
        if(vertexSubset[i]) {
    uchar *nghArr = G[i].getOutNeighbors();
#ifdef WEIGHTED
    decodeWgh(denseForwardT(next, vertexSubset), f, nghArr, i, G[i].getOutDegree());
#else
    decode(denseForwardT(next, vertexSubset), f, nghArr, i, G[i].getOutDegree());
#endif
        }
    }}
    return next;
  }

  struct sparseT {
    uintT v, o;
    uintE *outEdges;
  sparseT(uintT vP, uintT oP, uintE *outEdgesP) : v(vP), o(oP), outEdges(outEdgesP) {}
#ifndef WEIGHTED
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(v, target))
        outEdges[o + edgeNumber] = target;
      else outEdges[o + edgeNumber] = UINT_E_MAX;
      return true; }
#else
    inline bool srcTarg(Edge_F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(v, target, weight))
        outEdges[o + edgeNumber] = target;
      else outEdges[o + edgeNumber] = UINT_E_MAX;
      return true; }
#endif
  };

  template <class vertex>
  pair<long,uintE*> edgeMapSparse(vertex* frontierVertices, uintE* indices, 
               uintT* degrees, long m, Edge_F &f,
          long remDups=0, uintE* flags=NULL) {
    uintT* offsets = degrees;
    long outEdgeCount = sequence::plusScan(offsets, degrees, m);
    uintE* outEdges = newA(uintE,outEdgeCount);
    // In parallel, for each edge in frontier vertices, get out-degree and check
    parallel_for (long i = 0; i < m; i++) {
      uintT v = indices[i], o = offsets[i];
      vertex vert = frontierVertices[i]; 
  //    intT d = vert.getOutDegree();
      uchar *nghArr = vert.getOutNeighbors();
      // Decode, with src = v, and degree d, applying sparseT
#ifdef WEIGHTED
      decodeWgh(sparseT(v, o, outEdges), f, nghArr, v, vert.getOutDegree());
#else
      decode(sparseT(v, o, outEdges), f, nghArr, v, vert.getOutDegree());
#endif
    }
    uintE* nextIndices = newA(uintE, outEdgeCount);
    if(remDups) remDuplicates(outEdges,flags,outEdgeCount,remDups);
    // Filter out the empty slots (marked with UINT_E_MAX)
    long nextM = sequence::filter(outEdges,nextIndices,outEdgeCount,nonMaxF());
    free(outEdges);
    return pair<long,uintE*>(nextM, nextIndices);
  }
}

// Routing tables for methods 

template <class vertex>
bool* edgeMapDenseForward(graph<vertex> GA, bool* vertexSubset, Edge_F &f) { }

template <>
bool* edgeMapDenseForward(graph<symmetricVertex> GA, bool* vertexSubset, Edge_F &f) {
  return ligra_uncompressed::edgeMapDenseForward(GA, vertexSubset, f);
}

template <>
bool* edgeMapDenseForward(graph<asymmetricVertex> GA, bool* vertexSubset, Edge_F &f) {
  return ligra_uncompressed::edgeMapDenseForward(GA, vertexSubset, f);
}

template <>
bool* edgeMapDenseForward(graph<compressedAsymmetricVertex> GA, bool* vertexSubset, Edge_F &f) {
  return ligra_compressed::edgeMapDenseForward(GA, vertexSubset, f);
}

template <>
bool* edgeMapDenseForward(graph<compressedSymmetricVertex> GA, bool* vertexSubset, Edge_F &f) {
  return ligra_compressed::edgeMapDenseForward(GA, vertexSubset, f);
}

template <class vertex>
bool* edgeMapDense(graph<vertex> GA, bool* vertexSubset, Edge_F &f, bool parallel = 0) { }

template <>
bool* edgeMapDense(graph<symmetricVertex> GA, bool* vertexSubset, Edge_F &f, bool parallel) {
  return ligra_uncompressed::edgeMapDense(GA, vertexSubset, f, parallel);
}

template <>
bool* edgeMapDense(graph<asymmetricVertex> GA, bool* vertexSubset, Edge_F &f, bool parallel) {
  return ligra_uncompressed::edgeMapDense(GA, vertexSubset, f, parallel);
}

template <>
bool* edgeMapDense(graph<compressedAsymmetricVertex> GA, bool* vertexSubset, Edge_F &f, bool parallel) {
  return ligra_compressed::edgeMapDense(GA, vertexSubset, f);
}

template <>
bool* edgeMapDense(graph<compressedSymmetricVertex> GA, bool* vertexSubset, Edge_F &f, bool parallel) {
  return ligra_compressed::edgeMapDense(GA, vertexSubset, f);
}


template <class vertex>
pair<long,uintE*> edgeMapSparse(vertex* frontierVertices, uintE* indices, 
        uintT* degrees, uintT m, Edge_F &f, 
        long remDups=0, uintE* flags=NULL) { }

template <>
pair<long,uintE*> edgeMapSparse(symmetricVertex* frontierVertices, uintE* indices, 
        uintT* degrees, uintT m, Edge_F &f, long remDups, uintE* flags) {
  return ligra_uncompressed::edgeMapSparse(frontierVertices, indices, degrees, m, f, remDups, flags);
}

template <>
pair<long,uintE*> edgeMapSparse(asymmetricVertex* frontierVertices, uintE* indices, 
        uintT* degrees, uintT m, Edge_F &f, long remDups, uintE* flags) {
  return ligra_uncompressed::edgeMapSparse(frontierVertices, indices, degrees, m, f, remDups, flags);
}

template <>
pair<long,uintE*> edgeMapSparse(compressedAsymmetricVertex* frontierVertices, uintE* indices, 
        uintT* degrees, uintT m, Edge_F &f, long remDups, uintE* flags) {
  return ligra_compressed::edgeMapSparse(frontierVertices, indices, degrees, m, f, remDups, flags);
}

template <>
pair<long,uintE*> edgeMapSparse(compressedSymmetricVertex* frontierVertices, uintE* indices, 
        uintT* degrees, uintT m, Edge_F &f, long remDups, uintE* flags) {
  return ligra_compressed::edgeMapSparse(frontierVertices, indices, degrees, m, f, remDups, flags);
}



// decides on sparse or dense base on number of nonzeros in the active vertices
template <class vertex>
vertexSubset edgeMap(graph<vertex> GA, vertexSubset &V, Edge_F &f, intT threshold = -1, 
		 char option=DENSE, bool remDups=false) {
  long numVertices = GA.n, numEdges = GA.m;
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = GA.V;
  long m = V.numNonzeros();
  if (numVertices != V.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }
  // used to generate nonzero indices to get degrees
  uintT* degrees = newA(uintT, m);
  vertex* frontierVertices;
  V.toSparse();
  frontierVertices = newA(vertex,m);
  {parallel_for (long i=0; i < m; i++){
    vertex v = G[V.s[i]];
    degrees[i] = v.getOutDegree();
    frontierVertices[i] = v;
    }}
  uintT outDegrees = sequence::plusReduce(degrees, m);
  if (outDegrees == 0) return vertexSubset(numVertices);
  if (m + outDegrees > threshold) { 
    V.toDense();
    free(degrees);
    free(frontierVertices);
    bool* R = (option == DENSE_FORWARD) ? 
      edgeMapDenseForward(GA,V.d,f) : 
      edgeMapDense(GA, V.d, f, option);
    vertexSubset v1 = vertexSubset(numVertices, R);
    //cout << "size (D) = " << v1.m << endl;
    return v1;
  } else { 
    pair<long,uintE*> R = 
      remDups ? 
      edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), f, 
		    numVertices, GA.flags) :
      edgeMapSparse(frontierVertices, V.s, degrees, V.numNonzeros(), f);
    //cout << "size (S) = " << R.first << endl;
    free(degrees);
    free(frontierVertices);
    return vertexSubset(numVertices, R.first, R.second);
  }
}

//*****VERTEX FUNCTIONS*****

//Note: this is the optimized version of vertexMap which does not
//perform a filter
template <class F>
void vertexMap(vertexSubset V, F add) {
  long n = V.numRows(), m = V.numNonzeros();
  if(V.isDense) {
    {parallel_for(long i=0;i<n;i++)
	if(V.d[i]) add(i);}
  } else {
    {parallel_for(long i=0;i<m;i++)
	add(V.s[i]);}
  }
}

//Note: this is the version of vertexMap in which only a subset of the
//input vertexSubset is returned
template <class F>
vertexSubset vertexFilter(vertexSubset V, F filter) {
  long n = V.numRows(), m = V.numNonzeros();
  V.toDense();
  bool* d_out = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) d_out[i] = 0;}
  {parallel_for(long i=0;i<n;i++)
      if(V.d[i]) d_out[i] = filter(i);}
  return vertexSubset(n,d_out);
}

//cond function that always returns true
inline bool cond_true (intT d) { return 1; }

template<class vertex>
void Compute(graph<vertex>&, commandLine);

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  bool compressed = P.getOptionValue("-c");
  bool binary = P.getOptionValue("-b");
  long rounds = P.getOptionLongValue("-rounds",3);
  if (compressed) {
    if (symmetric) {
      graph<compressedSymmetricVertex> G =
        readCompressedGraph<compressedSymmetricVertex>(iFile,symmetric); //symmetric graph
      Compute(G,P);
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
      }
      G.del();
    } else {
      graph<compressedAsymmetricVertex> G =
        readCompressedGraph<compressedAsymmetricVertex>(iFile,symmetric); //asymmetric graph
      Compute(G,P);
      if(G.transposed) G.transpose();
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
        if(G.transposed) G.transpose();
      }
      G.del();
    }
  } else {
    if (symmetric) {
      graph<symmetricVertex> G =
        readGraph<symmetricVertex>(iFile,compressed,symmetric,binary); //symmetric graph
      Compute(G,P);
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
      }
      G.del();
    } else {
      graph<asymmetricVertex> G =
        readGraph<asymmetricVertex>(iFile,compressed,symmetric,binary); //asymmetric graph
      Compute(G,P);
      if(G.transposed) G.transpose();
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
        if(G.transposed) G.transpose();
      }
      G.del();
    }
  }
}
#endif
