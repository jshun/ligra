#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "parallel.h"

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

struct symmetricVertex {
  uchar* neighbors;
  uintT degree;
  uchar* getInNeighbors() { return neighbors; }
  uchar* getOutNeighbors() { return neighbors; }
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInNeighbors(uchar* _i) { neighbors = _i; }
  void setOutNeighbors(uchar* _i) { neighbors = _i; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
};

struct asymmetricVertex {
  uchar* inNeighbors;
  uchar* outNeighbors;
  uintT outDegree;
  uintT inDegree;
  uchar* getInNeighbors() { return inNeighbors; }
  uchar* getOutNeighbors() { return outNeighbors; }
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInNeighbors(uchar* _i) { inNeighbors = _i; }
  void setOutNeighbors(uchar* _i) { outNeighbors = _i; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); 
    swap(inDegree,outDegree); }
};

template <class vertex>
struct graph {
  vertex *V;
  long n;
  long m;
  uintT* inOffsets, *outOffsets;
  uchar* inEdges, *outEdges;
  uintE* flags;
  char* s;
  bool transposed;
graph(uintT* _inOffsets, uintT* _outOffsets, uchar* _inEdges, uchar* _outEdges, long nn, long mm, uintE* inDegrees, uintE* outDegrees, char* _s) 
: inOffsets(_inOffsets), outOffsets(_outOffsets), inEdges(_inEdges), outEdges(_outEdges), n(nn), m(mm), s(_s), flags(NULL), transposed(false) {
  V = newA(vertex,n);
  parallel_for(long i=0;i<n;i++) {
    long o = outOffsets[i];
    uintT d = outDegrees[i];
    V[i].setOutDegree(d);
    V[i].setOutNeighbors(outEdges+o);
  }

  if(sizeof(vertex) == sizeof(asymmetricVertex)){
    parallel_for(long i=0;i<n;i++) {
      long o = inOffsets[i];
      uintT d = inDegrees[i];
      V[i].setInDegree(d);
      V[i].setInNeighbors(inEdges+o);
    }
  }
}

  void del() {
    free(s);
    free(V);
    if(flags != NULL) free(flags);
  }
  void transpose() {
    if(sizeof(vertex) == sizeof(asymmetricVertex)) {
      parallel_for(long i=0;i<n;i++)
	V[i].flipEdges();
      transposed = !transposed;
    }
  }
};
#endif
