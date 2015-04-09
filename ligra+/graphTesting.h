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
#ifndef GRAPHTESTING_H
#define GRAPHTESTING_H

#include "graph.h"

template<class F>
struct printT {
  bool srcTarg(F f, intE src, intE target, intT edgeNumber) {
    cout << target <<" ";
    return true;
  }
  bool srcTarg(F f, intE src, intE target, intE weight, intT edgeNumber) {
    cout << target <<" ";
    return true;
  }
};

struct printF {};

template <class vertex>
void printGraph(graph<vertex> G) {
  vertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    uchar *nghArr = V[i].getOutNeighbors();
    intT d = V[i].getOutDegree();
    cout << i << ": ";
#ifdef WEIGHTED
    decodeWgh(printT<printF>(), printF(), nghArr, i, d);
#else
    decode(printT<printF>(), printF(), nghArr, i, d);
#endif
    cout << endl;
  }
}

template<class F>
struct checkT {
  long n,m;
checkT(long nn, long mm) : n(nn), m(mm) {} 
  bool srcTarg(F f, intE src, intE target, intT edgeNumber) {
    if(target < 0 || target >= n) 
      {cout << "out of bounds: ("<< src << "," << 
	  target << ") "<< endl; abort(); } 
    return 1;
  }
  bool srcTarg(F f, intE src, intE target, intE weight, intT edgeNumber) {
    if(target < 0 || target >= n) 
      {cout << "out of bounds: ("<< src << ","<< 
	  target << ","<<weight << ") "<<endl; abort(); } 
    return 1;
  }
};

template <class vertex>
void checkGraph(graph<vertex> G, long n, long m) {
  vertex *V = G.V;
  for (long i = 0; i < G.n; i++) {
    uchar *nghArr = V[i].getOutNeighbors();
    intT d = V[i].getOutDegree();
#ifdef WEIGHTED
    decodeWgh(checkT<printF>(n,m), printF(), nghArr, i, d);
#else
    decode(checkT<printF>(n,m), printF(), nghArr, i, d);
#endif
  }
  for (long i = 0; i < G.n; i++) {
    uchar *nghArr = V[i].getInNeighbors();
    intT d = V[i].getInDegree();
#ifdef WEIGHTED
    decodeWgh(checkT<printF>(n,m), printF(), nghArr, i, d);
#else
    decode(checkT<printF>(n,m), printF(), nghArr, i, d);
#endif
  }
}

#endif
