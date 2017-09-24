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
#include "vertex.h"
#include "compressedVertex.h"
#include "vertexSubset.h"
#include "graph.h"
#include "IO.h"
#include "parseCommandLine.h"
#include "gettime.h"
#include "index_map.h"
#include "edgeMap_utils.h"
using namespace std;

//*****START FRAMEWORK*****

//options to edgeMap for different versions of dense edgeMap (default is DENSE)
enum options { DENSE, DENSE_FORWARD };

typedef uint32_t flags;
const flags output = 1;
const flags no_output = 2;
const flags pack_edges = 4;
const flags sparse_no_filter = 8;

template <class data, class vertex, class VS, class F>
tuple<bool, data>* edgeMapDense(graph<vertex> GA, VS& vertexSubset, F &f, bool parallel = 0) {
  using D = tuple<bool, data>;
  long numVertices = GA.n;
  vertex *G = GA.V;
  D* next = newA(D, numVertices);
  auto g = get_emdense_gen<data>(next);
  parallel_for (long v=0; v<numVertices; v++) {
    std::get<0>(next[v]) = 0;
    if (f.cond(v)) {
      G[v].decodeInNghBreakEarly(v, vertexSubset, f, g, parallel);
    }
  }
  return next;
}

template <class data, class vertex, class VS, class F>
tuple<bool, data>* edgeMapDenseForward(graph<vertex> GA, VS& vertexSubset, F &f) {
  using D = tuple<bool, data>;
  long n = GA.n;
  vertex *G = GA.V;
  D* next = newA(D, n);
  auto g = get_emdense_forward_gen<data>(next);
  parallel_for(long i=0;i<n;i++) { std::get<0>(next[i]) = 0; }
  parallel_for (long i=0; i<n; i++) {
    if (vertexSubset.isIn(i)) {
      G[i].decodeOutNgh(i, f, g);
    }
  }
  return next;
}

template <class data, class vertex, class vs, class F>
pair<size_t, tuple<uintE, data>*> edgeMapSparse(vertex* frontierVertices, vs &indices,
        uintT* degrees, uintT m, F &f) {
  using S = tuple<uintE, data>;
  uintT* offsets = degrees;
  long outEdgeCount = sequence::plusScan(offsets, offsets, m);
  S* outEdges = newA(S, outEdgeCount);

  auto g = get_emsparse_gen<data>(outEdges);

  parallel_for (size_t i = 0; i < m; i++) {
    uintT v = indices.vtx(i), o = offsets[i];
    vertex vert = frontierVertices[i];
    vert.decodeOutNghSparse(v, o, f, g);
  }
  S* nextIndices = newA(S, outEdgeCount);

  auto p = [] (tuple<uintE, data>& v) { return std::get<0>(v) != UINT_E_MAX; };
  size_t nextM = pbbs::filterf(outEdges, nextIndices, outEdgeCount, p);
  free(outEdges);
  return make_pair(nextM, nextIndices);
}

// Decides on sparse or dense base on number of nonzeros in the active vertices.
template <class data, class vertex, class VS, class F>
vertexSubsetData<data> edgeMapData(const graph<vertex>& GA, VS &vs, F f,
    intT threshold = -1, char option=DENSE, const flags& fl=output) {
  long numVertices = GA.n, numEdges = GA.m, m = vs.numNonzeros();
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = GA.V;
  if (numVertices != vs.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }
  if (vs.size() == 0) return vertexSubsetData<data>(numVertices);
  vs.toSparse();
  uintT* degrees = newA(uintT, m);
  vertex* frontierVertices = newA(vertex,m);
  {parallel_for (size_t i=0; i < m; i++) {
    uintE v_id = vs.vtx(i);
    vertex v = G[v_id];
    degrees[i] = v.getOutDegree();
    frontierVertices[i] = v;
  }}

  uintT outDegrees = sequence::plusReduce(degrees, m);
  if (outDegrees == 0) return vertexSubsetData<data>(numVertices);
  if (m + outDegrees > threshold) {
    vs.toDense();
    free(degrees); free(frontierVertices);
    tuple<bool, data>* R = (option == DENSE_FORWARD) ?
      edgeMapDenseForward<data, vertex, VS, F>(GA, vs, f) :
      edgeMapDense<data, vertex, VS, F>(GA, vs, f, option);
    vertexSubsetData<data> v1 = vertexSubsetData<data>(numVertices, R);
    return v1;
  } else {
    pair<size_t, tuple<uintE, data>*> R =
      (fl & sparse_no_filter) ?
      edgeMapSparse_no_filter<data, vertex, VS, F>(frontierVertices, vs, degrees, vs.numNonzeros(), f) :
      edgeMapSparse<data, vertex, VS, F>(frontierVertices, vs, degrees, vs.numNonzeros(), f);
    free(degrees); free(frontierVertices);
    return vertexSubsetData<data>(numVertices, R.first, R.second);
  }
}

// Version of edgeMap which produces no output.
template <class outdata, class vertex, class VS, class F>
vertexSubsetData<outdata> edgeMapNoOutput(graph<vertex> GA, VS &vs, F f,
    intT threshold = -1, char option=DENSE, const flags& fl=output) {
  long numVertices = GA.n, numEdges = GA.m, m = vs.numNonzeros();
  if(threshold == -1) threshold = numEdges/20;
  vertex *V = GA.V;
  if (numVertices != vs.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }
  if (vs.size() == 0) {
    return vertexSubsetData<outdata>(numVertices);
  }

  vs.toSparse();
  vertex* frontierVertices = newA(vertex,m);
  {parallel_for (size_t i=0; i < m; i++) {
    uintE v_id = vs.vtx(i);
    vertex v = V[v_id];
    frontierVertices[i] = v;
  }}

  edgeMapSparseNoOutput<outdata, vertex, VS, F>(frontierVertices, vs, vs.numNonzeros(), f);
  free(frontierVertices);
  return vertexSubsetData<outdata>(numVertices);
}

// Main edgeMap function, which determines which implementation to call based on
// the flags provided by the user.
template <class vertex, class VS, class F>
vertexSubset edgeMap(graph<vertex> GA, VS& vs, F f,
    intT threshold = -1, char option=DENSE, const flags& fl=output) {
  if (fl & no_output) {
    return edgeMapNoOutput<pbbs::empty>(GA, vs, f, threshold, option, fl);
  } else {
    return edgeMapData<pbbs::empty>(GA, vs, f, threshold, option, fl);
  }
}

// Packs out the adjacency lists of all vertex in vs. A neighbor, ngh, is kept
// in the new adjacency list if p(ngh) is true.
// Weighted graphs are not yet supported, but this should be easy to do.
template <class vertex, class P>
vertexSubsetData<uintE> packEdges(graph<vertex>& GA, vertexSubset& vs, P& p, const flags& fl=output) {
  using S = tuple<uintE, uintE>;
  vs.toSparse();
  vertex* G = GA.V; long m = vs.numNonzeros(); long n = vs.numRows();
  if (vs.size() == 0) {
    return vertexSubsetData<uintE>(n);
  }
  auto degrees = array_imap<uintT>(m);
  granular_for(i, 0, m, (m > 2000), {
    uintE v = vs.vtx(i);
    degrees[i] = G[v].getOutDegree();
  });
  long outEdgeCount = pbbs::scan_add(degrees, degrees);
  S* outV;
  if (fl & output) {
    outV = newA(S, vs.size());
  }

  bool* bits = newA(bool, outEdgeCount);
  uintE* tmp1 = newA(uintE, outEdgeCount);
  uintE* tmp2 = newA(uintE, outEdgeCount);
  if (fl & output) {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t offset = degrees[i];
      auto bitsOff = &(bits[offset]); auto tmp1Off = &(tmp1[offset]);
      auto tmp2Off = &(tmp2[offset]);
      size_t ct = G[v].packOutNgh(v, p, bitsOff, tmp1Off, tmp2Off);
      outV[i] = make_tuple(v, ct);
    }
  } else {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t offset = degrees[i];
      auto bitsOff = &(bits[offset]); auto tmp1Off = &(tmp1[offset]);
      auto tmp2Off = &(tmp2[offset]);
      size_t ct = G[v].packOutNgh(v, p, bitsOff, tmp1Off, tmp2Off);
    }
  }
  free(bits); free(tmp1); free(tmp2);
  if (fl & output) {
    return vertexSubsetData<uintE>(n, m, outV);
  } else {
    return vertexSubsetData<uintE>(n);
  }
}

template <class vertex, class P>
vertexSubsetData<uintE> edgeMapFilter(graph<vertex>& GA, vertexSubset& vs, P& p, const flags& fl=output) {
  vs.toSparse();
  if (fl & pack_edges) {
    return packEdges<vertex, P>(GA, vs, p, fl);
  }
  vertex* G = GA.V; long m = vs.numNonzeros(); long n = vs.numRows();
  using S = tuple<uintE, uintE>;
  if (vs.size() == 0) {
    return vertexSubsetData<uintE>(n);
  }
  S* outV;
  if (fl & output) {
    outV = newA(S, vs.size());
  }
  if (fl & output) {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t ct = G[v].countOutNgh(v, p);
      outV[i] = make_tuple(v, ct);
    }
  } else {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t ct = G[v].countOutNgh(v, p);
    }
  }
  if (fl & output) {
    return vertexSubsetData<uintE>(n, m, outV);
  } else {
    return vertexSubsetData<uintE>(n);
  }
}



//*****VERTEX FUNCTIONS*****

template <class F, class VS, typename std::enable_if<
  !std::is_same<VS, vertexSubset>::value, int>::type=0 >
void vertexMap(VS& V, F f) {
  size_t n = V.numRows(), m = V.numNonzeros();
  if(V.dense()) {
    parallel_for(long i=0;i<n;i++) {
      if(V.isIn(i)) {
        f(i, V.ithData(i));
      }
    }
  } else {
    parallel_for(long i=0;i<m;i++) {
      f(V.vtx(i), V.vtxData(i));
    }
  }
}

template <class VS, class F, typename std::enable_if<
  std::is_same<VS, vertexSubset>::value, int>::type=0 >
void vertexMap(VS& V, F f) {
  size_t n = V.numRows(), m = V.numNonzeros();
  if(V.dense()) {
    parallel_for(long i=0;i<n;i++) {
      if(V.isIn(i)) {
        f(i);
      }
    }
  } else {
    parallel_for(long i=0;i<m;i++) {
      f(V.vtx(i));
    }
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

template <class F>
vertexSubset vertexFilter2(vertexSubset V, F filter) {
  long n = V.numRows(), m = V.numNonzeros();
  if (m == 0) {
    return vertexSubset(n);
  }
  bool* bits = newA(bool, m);
  V.toSparse();
  {parallel_for(size_t i=0; i<m; i++) {
    uintE v = V.vtx(i);
    bits[i] = filter(v);
  }}
  auto v_imap = make_in_imap<uintE>(m, [&] (size_t i) { return V.vtx(i); });
  auto bits_m = make_in_imap<bool>(m, [&] (size_t i) { return bits[i]; });
  auto out = pbbs::pack(v_imap, bits_m);
  out.alloc = false;
  free(bits);
  return vertexSubset(n, out.size(), out.s);
}

template <class data, class F>
vertexSubset vertexFilter2(vertexSubsetData<data> V, F filter) {
  long n = V.numRows(), m = V.numNonzeros();
  if (m == 0) {
    return vertexSubset(n);
  }
  bool* bits = newA(bool, m);
  V.toSparse();
  parallel_for(size_t i=0; i<m; i++) {
    auto t = V.vtxAndData(i);
    bits[i] = filter(std::get<0>(t), std::get<1>(t));
  }
  auto v_imap = make_in_imap<uintE>(m, [&] (size_t i) { return V.vtx(i); });
  auto bits_m = make_in_imap<bool>(m, [&] (size_t i) { return bits[i]; });
  auto out = pbbs::pack(v_imap, bits_m);
  out.alloc = false;
  free(bits);
  return vertexSubset(n, out.size(), out.s);
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
  bool mmap = P.getOptionValue("-m");
  //cout << "mmap = " << mmap << endl;
  long rounds = P.getOptionLongValue("-rounds",3);
  if (compressed) {
    if (symmetric) {
      graph<compressedSymmetricVertex> G =
        readCompressedGraph<compressedSymmetricVertex>(iFile,symmetric,mmap); //symmetric graph
      Compute(G,P);
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
      }
      G.del();
    } else {
      graph<compressedAsymmetricVertex> G =
        readCompressedGraph<compressedAsymmetricVertex>(iFile,symmetric,mmap); //asymmetric graph
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
        readGraph<symmetricVertex>(iFile,compressed,symmetric,binary,mmap); //symmetric graph
      Compute(G,P);
      for(int r=0;r<rounds;r++) {
        startTime();
        Compute(G,P);
        nextTime("Running time");
      }
      G.del();
    } else {
      graph<asymmetricVertex> G =
        readGraph<asymmetricVertex>(iFile,compressed,symmetric,binary,mmap); //asymmetric graph
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
