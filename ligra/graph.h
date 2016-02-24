#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "vertex.h"
#include "compressedVertex.h"
#include "parallel.h"
using namespace std;

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

template <class vertex>
struct graph {
  vertex *V;
  long n;
  long m;
#ifndef WEIGHTED
  uintE* allocatedInplace, * inEdges;
#else
  intE* allocatedInplace, * inEdges;
#endif
  uintE* flags;
  bool transposed;
  graph(vertex* VV, long nn, long mm) 
  : V(VV), n(nn), m(mm), allocatedInplace(NULL), flags(NULL), transposed(false) {}
#ifndef WEIGHTED
  graph(vertex* VV, long nn, long mm, uintE* ai, uintE* _inEdges = NULL) 
#else
  graph(vertex* VV, long nn, long mm, intE* ai, intE* _inEdges = NULL) 
#endif
  : V(VV), n(nn), m(mm), allocatedInplace(ai), inEdges(_inEdges), flags(NULL), transposed(false) {}
  void del() {
    if (flags != NULL) free(flags);
    if (allocatedInplace == NULL) 
      for (long i=0; i < n; i++) V[i].del();
    else free(allocatedInplace);
    free(V);
    if(inEdges != NULL) free(inEdges);
  }
  void transpose() {
    if(sizeof(vertex) == sizeof(asymmetricVertex)) {
      parallel_for(long i=0;i<n;i++) {
	V[i].flipEdges();
      }
      transposed = !transposed;
    } 
  }
};
#endif
