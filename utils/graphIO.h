// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and the PBBS team
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
#ifndef _BENCH_GRAPH_IO
#define _BENCH_GRAPH_IO

#include <iostream>
#include <stdint.h>
#include <cstring>
#include "parallel.h"
#include "blockRadixSort.h"
#include "quickSort.h"
using namespace std;

// **************************************************************
//    EDGE ARRAY REPRESENTATION
// **************************************************************

template <class intT>
struct edge {
  intT u;
  intT v;
  edge(intT f, intT s) : u(f), v(s) {}
};

template <class intT>
struct edgeArray {
  edge<intT>* E;
  intT numRows;
  intT numCols;
  intT nonZeros;
  void del() {free(E);}
  edgeArray(edge<intT> *EE, intT r, intT c, intT nz) :
    E(EE), numRows(r), numCols(c), nonZeros(nz) {}
  edgeArray() {}
};

template <class intT>
struct hyperedgeArray {
  edge<intT>* VE;
  edge<intT>* HE;
  long nv,nh,mv,mh;
  void del() {free(VE);free(HE);}
hyperedgeArray(edge<intT>* _VE, edge<intT>* _HE, long _nv, long _nh, long _mv, long _mh) :
  VE(_VE), HE(_HE), nv(_nv), nh(_nh), mv(_mv), mh(_mh) {}
  hyperedgeArray() {}
};

template <class intT>
struct wghEdge {
  intT u;
  intT v;
  intT w;
wghEdge(intT f, intT s, intT t) : u(f), v(s), w(t) {}
};

template <class intT>
struct wghEdgeArray {
  wghEdge<intT>* E;
  intT numRows;
  intT numCols;
  intT nonZeros;
  void del() {free(E);}
  wghEdgeArray(wghEdge<intT> *EE, intT r, intT c, intT nz) :
    E(EE), numRows(r), numCols(c), nonZeros(nz) {}
  wghEdgeArray() {}
};

template <class intT>
struct wghHyperedgeArray {
  wghEdge<intT>* VE;
  wghEdge<intT>* HE;
  long nv,nh,mv,mh;
  void del() {free(VE);free(HE);}
wghHyperedgeArray(wghEdge<intT>* _VE, wghEdge<intT>* _HE, long _nv, long _nh, long _mv, long _mh) :
  VE(_VE), HE(_HE), nv(_nv), nh(_nh), mv(_mv), mh(_mh) {}
  wghHyperedgeArray() {}
};

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

template <class intT>
struct vertex {
  intT* Neighbors;
  intT degree;
  void del() {free(Neighbors);}
  vertex(intT* N, intT d) : Neighbors(N), degree(d) {}
};

template <class intT>
struct wghVertex {
  intT* Neighbors;
  intT degree;
  intT* nghWeights;
  void del() {free(Neighbors); free(nghWeights);}
wghVertex(intT* N, intT* W, intT d) : Neighbors(N), nghWeights(W), degree(d) {}
};

template <class intT>
struct graph {
  vertex<intT> *V;
  intT n;
  intT m;
  intT* allocatedInplace;
  graph(vertex<intT>* VV, intT nn, uintT mm) 
    : V(VV), n(nn), m(mm), allocatedInplace(NULL) {}
  graph(vertex<intT>* VV, intT nn, uintT mm, intT* ai) 
    : V(VV), n(nn), m(mm), allocatedInplace(ai) {}
  intT* vertices() { return allocatedInplace+2; }
  intT* edges() { return allocatedInplace+2+n; }
  graph copy() {
    vertex<intT>* VN = newA(vertex<intT>,n);
    intT* _allocatedInplace = newA(intT,n+m+2);
    _allocatedInplace[0] = n;
    _allocatedInplace[1] = m;
    intT* Edges = _allocatedInplace+n+2;
    intT k = 0;
    for (intT i=0; i < n; i++) {
      _allocatedInplace[i+2] = allocatedInplace[i+2];
      VN[i] = V[i];
      VN[i].Neighbors = Edges + k;
      for (intT j =0; j < V[i].degree; j++) 
	Edges[k++] = V[i].Neighbors[j];
    }
    return graph(VN, n, m, _allocatedInplace);
  } 
  void del() {
    if (allocatedInplace == NULL) 
      for (intT i=0; i < n; i++) V[i].del();
    else free(allocatedInplace);
    free(V);
  }
};

template <class intT>
struct hypergraph {
  vertex<intT> *V;
  vertex<intT> *H;
  long nv;
  long mv;
  long nh;
  long mh;
  intT* edgesV, *edgesH;
  intT* allocatedInplace;

hypergraph(vertex<intT>* _V, vertex<intT>* _H, long _nv, long _mv, long _nh, long _mh, intT* _edgesV, intT* _edgesH) : V(_V), H(_H), nv(_nv), mv(_mv), nh(_nh), mh(_mh), edgesV(_edgesV), edgesH(_edgesH), allocatedInplace(0) {}
hypergraph(vertex<intT>* _V, vertex<intT>* _H, long _nv, long _mv, long _nh, long _mh, intT* ai) : V(_V), H(_H), nv(_nv), mv(_mv), nh(_nh), mh(_mh), allocatedInplace(ai) {}
  void del() {
    free(H); free(V);
    if(allocatedInplace) free(allocatedInplace);
    else {free(edgesV); free(edgesH);}
  }
};

template <class intT>
struct wghGraph {
  wghVertex<intT> *V;
  intT n;
  uintT m;
  intT* allocatedInplace;
  intT* weights;
wghGraph(wghVertex<intT>* VV, intT nn, uintT mm) 
    : V(VV), n(nn), m(mm), allocatedInplace(NULL) {}
wghGraph(wghVertex<intT>* VV, intT nn, uintT mm, intT* ai, intT* _weights) 
: V(VV), n(nn), m(mm), allocatedInplace(ai), weights(_weights) {}
  wghGraph copy() {
    wghVertex<intT>* VN = newA(wghVertex<intT>,n);
    intT* Edges = newA(intT,m);
    intT* Weights = newA(intT,m);
    intT k = 0;
    for (intT i=0; i < n; i++) {
      VN[i] = V[i];
      VN[i].Neighbors = Edges + k;
      VN[i].nghWeights = Weights + k;
      for (intT j =0; j < V[i].degree; j++){ 
	Edges[k] = V[i].Neighbors[j];
	Weights[k++] = V[i].nghWeights[j];
      }
    }
    return wghGraph(VN, n, m, Edges, Weights);
  } 
  void del() {
    if (allocatedInplace == NULL) 
      for (intT i=0; i < n; i++) V[i].del();
    else { free(allocatedInplace); }
    free(V);
  }
};

template <class intT>
struct wghHypergraph {
  wghVertex<intT> *V;
  wghVertex<intT> *H;
  long nv;
  long mv;
  long nh;
  long mh;
  intT* edgesV, *edgesH;
  intT* allocatedInplace;
wghHypergraph(wghVertex<intT>* _V, wghVertex<intT>* _H, long _nv, long _mv, long _nh, long _mh, intT* _edgesV, intT* _edgesH) : V(_V), H(_H), nv(_nv), mv(_mv), nh(_nh), mh(_mh), edgesV(_edgesV), edgesH(_edgesH), allocatedInplace(0) {}
wghHypergraph(wghVertex<intT>* _V, wghVertex<intT>* _H, long _nv, long _mv, long _nh, long _mh, intT* ai) : V(_V), H(_H), nv(_nv), mv(_mv), nh(_nh), mh(_mh), allocatedInplace(ai) {}
  void del() {
    free(H); free(V);
    if(allocatedInplace) free(allocatedInplace);
    else {free(edgesV); free(edgesH);} 
  }
};

// **************************************************************
//    GRAPH UTILITIES
// **************************************************************
struct edgeCmp {
  bool operator() (edge<uintT> e1, edge<uintT> e2) {
    return ((e1.u < e2.u) ? 1 : ((e1.u > e2.u) ? 0 : (e1.v < e2.v)));
  }
};

struct edgeCmpWgh {
  bool operator() (wghEdge<uintT> e1, wghEdge<uintT> e2) {
    return ((e1.u < e2.u) ? 1 : ((e1.u > e2.u) ? 0 : (e1.v < e2.v)));
  }
};

template <class intT>
edgeArray<intT> remDuplicates(edgeArray<intT> A) {
  intT m = A.nonZeros;
  edge<intT> * E = newA(edge<intT>,m);
  {parallel_for(intT i=0;i<m;i++) {E[i].u = A.E[i].u; E[i].v = A.E[i].v;}}
  quickSort(E,m,edgeCmp());

  intT* flags = newA(intT,m);
  flags[0] = 1;
  {parallel_for(intT i=1;i<m;i++) {
    if((E[i].u != E[i-1].u) || (E[i].v != E[i-1].v)) flags[i] = 1;
    else flags[i] = 0;
    }}

  intT mm = sequence::plusScan(flags,flags,m);
  edge<intT>* F = newA(edge<intT>,mm);
  F[mm-1] = E[m-1];
  {parallel_for(intT i=0;i<m-1;i++) {
    if(flags[i] != flags[i+1]) F[flags[i]] = E[i];
    }}
  free(flags);
  return edgeArray<intT>(F,A.numRows,A.numCols,mm);
}

template <class intT>
wghEdgeArray<intT> remDuplicates(wghEdgeArray<intT> A) {
  intT m = A.nonZeros;
  wghEdge<intT> * E = newA(wghEdge<intT>,m);
  {parallel_for(intT i=0;i<m;i++) {E[i].u = A.E[i].u; E[i].v = A.E[i].v; E[i].w = A.E[i].w;}}
  quickSort(E,m,edgeCmpWgh());

  intT* flags = newA(intT,m);
  flags[0] = 1;
  {parallel_for(intT i=1;i<m;i++) {
    if((E[i].u != E[i-1].u) || (E[i].v != E[i-1].v)) flags[i] = 1;
    else flags[i] = 0;
    }}

  intT mm = sequence::plusScan(flags,flags,m);
  wghEdge<intT>* F = newA(wghEdge<intT>,mm);
  F[mm-1] = E[m-1];
  {parallel_for(intT i=0;i<m-1;i++) {
    if(flags[i] != flags[i+1]) F[flags[i]] = E[i];
    }}
  free(flags);
  return wghEdgeArray<intT>(F,A.numRows,A.numCols,mm);
}


template <class intT>
struct nEQF {bool operator() (edge<intT> e) {return (e.u != e.v);}};

template <class intT>
struct nEQFWgh {bool operator() (wghEdge<intT> e) {return (e.u != e.v);}};

template <class intT>
edgeArray<intT> makeSymmetric(edgeArray<intT> A) {
  intT m = A.nonZeros;
  edge<intT> *E = A.E;
  edge<intT> *F = newA(edge<intT>,2*m);
  intT mm = sequence::filter(E,F,m,nEQF<intT>());

  parallel_for (intT i=0; i < mm; i++) {
    F[i+mm].u = F[i].v;
    F[i+mm].v = F[i].u;
  }

  edgeArray<intT> R = remDuplicates(edgeArray<intT>(F,A.numRows,A.numCols,2*mm));
  free(F);
  
  return R;
}

template <class intT>
wghEdgeArray<intT> makeSymmetric(wghEdgeArray<intT> A) {
  intT m = A.nonZeros;
  wghEdge<intT> *E = A.E;
  wghEdge<intT> *F = newA(wghEdge<intT>,2*m);
  intT mm = sequence::filter(E,F,m,nEQFWgh<intT>());

  parallel_for (intT i=0; i < mm; i++) {
    F[i+mm].u = F[i].v;
    F[i+mm].v = F[i].u;
    F[i+mm].w = F[i].w;
  }

  wghEdgeArray<intT> R = remDuplicates(wghEdgeArray<intT>(F,A.numRows,A.numCols,2*mm));
  free(F);
  
  return R;
}

template <class intT>
struct getuF {intT operator() (edge<intT> e) {return e.u;} };

template <class intT>
struct getuFWgh {intT operator() (wghEdge<intT> e) {return e.u;} };

template <class intT>
graph<intT> graphFromEdges(edgeArray<intT> EA, bool makeSym) {
  edgeArray<intT> A;
  if (makeSym) A = makeSymmetric<intT>(EA);
  else {  // should have copy constructor
    edge<intT> *E = newA(edge<intT>,EA.nonZeros);
    parallel_for (intT i=0; i < EA.nonZeros; i++) E[i] = EA.E[i];
    A = edgeArray<intT>(E,EA.numRows,EA.numCols,EA.nonZeros);
  }
  intT m = A.nonZeros;
  intT n = max<intT>(A.numCols,A.numRows);
  intT* offsets = newA(intT,n);
  intSort::iSort(A.E,offsets,m,n,getuF<intT>());
  intT *X = newA(intT,m);
  vertex<intT> *v = newA(vertex<intT>,n);
  parallel_for (intT i=0; i < n; i++) {
    intT o = offsets[i];
    intT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].degree = l;
    v[i].Neighbors = X+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = A.E[o+j].v;
    }
  }
  A.del();
  free(offsets);
  return graph<intT>(v,n,m,X);
}
 
template <class intT>
hypergraph<intT> hypergraphFromHyperedges(hyperedgeArray<intT> EA) {
  long nv = EA.nv, nh = EA.nh, mv = EA.mv, mh = EA.mh;
  intT* offsetsV = newA(intT,nv);
  edge<intT>* E = newA(edge<intT>,mv);
  parallel_for(long i=0;i<mv;i++) E[i] = EA.VE[i];
  intSort::iSort(E,offsetsV,mv,nv,getuF<intT>());
  intT *edgesV = newA(intT,mv);
  vertex<intT> *v = newA(vertex<intT>,nv);
  parallel_for (intT i=0; i < nv; i++) {
    intT o = offsetsV[i];
    intT l = ((i == nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
    v[i].degree = l;
    v[i].Neighbors = edgesV+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = E[o+j].v;
    }
  }
  free(offsetsV);

  intT* offsetsH = newA(intT,nh);
  parallel_for(long i=0;i<mh;i++) E[i] = EA.HE[i];
  intSort::iSort(E,offsetsH,mh,nh,getuF<intT>()); //don't need?
  intT *edgesH = newA(intT,mh);
  vertex<intT> *h = newA(vertex<intT>,nh);
  parallel_for (intT i=0; i<nh;i++) {
    intT o = offsetsH[i];
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    h[i].degree = l;
    h[i].Neighbors = edgesH+o;
    for (intT j=0; j < l; j++) {
      h[i].Neighbors[j] = E[o+j].v;
    }
  }
  free(offsetsH);
  free(E);
  return hypergraph<intT>(v,h,nv,mv,nh,mh,edgesV,edgesH);
}

template <class intT>
wghGraph<intT> wghGraphFromWghEdges(wghEdgeArray<intT> EA, bool makeSym) {
  wghEdgeArray<intT> A;
  if (makeSym) A = makeSymmetric<intT>(EA);
  else {  // should have copy constructor
    wghEdge<intT> *E = newA(wghEdge<intT>,EA.nonZeros);
    parallel_for (intT i=0; i < EA.nonZeros; i++) E[i] = EA.E[i];
    A = wghEdgeArray<intT>(E,EA.numRows,EA.numCols,EA.nonZeros);
  }
  intT m = A.nonZeros;
  intT n = max<intT>(A.numCols,A.numRows);
  intT* offsets = newA(intT,n);
  intSort::iSort(A.E,offsets,m,n,getuFWgh<intT>());
  intT *X = newA(intT,2*m);
  intT *Weights = X+m;
  wghVertex<intT> *v = newA(wghVertex<intT>,n);
  parallel_for (intT i=0; i < n; i++) {
    intT o = offsets[i];
    intT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].degree = l;
    v[i].Neighbors = X+o;
    v[i].nghWeights = Weights+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = A.E[o+j].v;
      v[i].nghWeights[j] = A.E[o+j].w;
    }
  }
  A.del();
  free(offsets);
  return wghGraph<intT>(v,n,m,X,Weights);
}

template <class intT>
wghHypergraph<intT> wghHypergraphFromWghHyperedges(wghHyperedgeArray<intT> EA) {
  long nv = EA.nv, nh = EA.nh, mv = EA.mv, mh = EA.mh;
  intT* offsetsV = newA(intT,nv);
  wghEdge<intT>* E = newA(wghEdge<intT>,mv);
  parallel_for(long i=0;i<mv;i++) E[i] = EA.VE[i];
  intSort::iSort(E,offsetsV,mv,nv,getuFWgh<intT>());
  intT *edgesV = newA(intT,mv*2);
  intT *WeightsV = edgesV + mv;
  wghVertex<intT> *v = newA(wghVertex<intT>,nv);
  parallel_for (intT i=0; i < nv; i++) {
    intT o = offsetsV[i];
    intT l = ((i == nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
    v[i].degree = l;
    v[i].Neighbors = edgesV+o;
    v[i].nghWeights = WeightsV+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = E[o+j].v;
      v[i].nghWeights[j] = E[o+j].w;
    }
  }
  free(offsetsV);

  intT* offsetsH = newA(intT,nh);
  parallel_for(long i=0;i<mh;i++) E[i] = EA.HE[i];
  intSort::iSort(E,offsetsH,mh,nh,getuFWgh<intT>()); //don't need?
  intT *edgesH = newA(intT,mh*2);
  intT *WeightsH = edgesH + mh;
  wghVertex<intT> *h = newA(wghVertex<intT>,nh);
  parallel_for (intT i=0; i<nh;i++) {
    intT o = offsetsH[i];
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    h[i].degree = l;
    h[i].Neighbors = edgesH+o;
    h[i].nghWeights = WeightsH+o;
    for (intT j=0; j < l; j++) {
      h[i].Neighbors[j] = E[o+j].v;
      h[i].nghWeights[j] = E[o+j].w;
    }
  }
  free(offsetsH);
  free(E);
  return wghHypergraph<intT>(v,h,nv,mv,nh,mh,edgesV,edgesH);
}

// **************************************************************
//    BASIC I/O
// **************************************************************
namespace benchIO {
  using namespace std;

  // A structure that keeps a sequence of strings all allocated from
  // the same block of memory
  struct words {
    long n; // total number of characters
    char* Chars;  // array storing all strings
    long m; // number of substrings
    char** Strings; // pointers to strings (all should be null terminated)
    words() {}
    words(char* C, long nn, char** S, long mm)
      : Chars(C), n(nn), Strings(S), m(mm) {}
    void del() {free(Chars); free(Strings);}
  };

  struct lines {
    long n; // total number of characters
    char* Chars;  // array storing all strings
    long m; // number of substrings
    char** Strings; // pointers to strings (all should be null terminated)
    long numLines;
    intT* Lines; //offsets of beginning of lines
    lines() {}
  lines(char* C, long nn, char** S, long mm, intT* _Lines, long _numLines)
  : Chars(C), n(nn), Strings(S), m(mm), Lines(_Lines), numLines(_numLines) {}
    void del() {free(Chars); free(Strings); free(Lines);}
  };

  inline bool isSpace(char c) {
    switch (c)  {
    case '\r': 
    case '\t': 
    case '\n': 
    case 0:
    case ' ' : return true;
    default : return false;
    }
  }

  struct toLong { long operator() (bool v) {return (long) v;} };

  // parallel code for converting a string to words
  words stringToWords(char *Str, long n) {
    parallel_for (long i=0; i < n; i++) 
      if (isSpace(Str[i])) Str[i] = 0; 

    // mark start of words
    bool *FL = newA(bool,n);
    FL[0] = Str[0];
    parallel_for (long i=1; i < n; i++) FL[i] = Str[i] && !Str[i-1];
    
    // offset for each start of word
    _seq<long> Off = sequence::packIndex<long>(FL, n);
    long m = Off.n;
    long *offsets = Off.A;

    // pointer to each start of word
    char **SA = newA(char*, m);
    parallel_for (long j=0; j < m; j++) SA[j] = Str+offsets[j];

    free(offsets); free(FL);
    return words(Str,n,SA,m);
  }

  lines stringToLinesOfWords(char *Str, long n) {

    // mark start of words
    bool *FL = newA(bool,n);
    FL[0] = Str[0];
    parallel_for (long i=1; i < n; i++) FL[i] = !isSpace(Str[i]) && isSpace(Str[i-1]); //!Str[i-1];
    
    // offset for each start of word
    _seq<long> Off = sequence::packIndex<long>(FL, n);
    long m = Off.n;
    long *offsets = Off.A;

    // pointer to each start of word
    char **SA = newA(char*, m);
    parallel_for (long j=0; j < m; j++) SA[j] = Str+offsets[j];

    //mark start of lines
    FL[0] = 1;
    parallel_for(long j=1;j<m;j++) {
      FL[j] = !isSpace(Str[offsets[j]]) && (Str[offsets[j]-1] == '\n' || Str[offsets[j]-1] == '\r');} //check if new line
    _seq<intT> LinesSeq = sequence::packIndex<intT>(FL, m);
    long numLines = LinesSeq.n;
    intT* Lines = LinesSeq.A;

    parallel_for (long i=0; i < n; i++) 
      if (isSpace(Str[i])) Str[i] = 0; 

    free(offsets); free(FL);
    return lines(Str,n,SA,m,Lines,numLines);
  }

  int writeStringToFile(char* S, long n, char* fileName) {
    ofstream file (fileName, ios::out | ios::binary);
    if (!file.is_open()) {
      std::cout << "Unable to open file: " << fileName << std::endl;
      return 1;
    }
    file.write(S, n);
    file.close();
    return 0;
  }

  inline int xToStringLen(long a) { return 21;}
  inline void xToString(char* s, long a) { sprintf(s,"%ld",a);}

  inline int xToStringLen(unsigned long a) { return 21;}
  inline void xToString(char* s, unsigned long a) { sprintf(s,"%lu",a);}

  inline uint xToStringLen(uint a) { return 12;}
  inline void xToString(char* s, uint a) { sprintf(s,"%u",a);}

  inline int xToStringLen(int a) { return 12;}
  inline void xToString(char* s, int a) { sprintf(s,"%d",a);}

  inline int xToStringLen(double a) { return 18;}
  inline void xToString(char* s, double a) { sprintf(s,"%.11le", a);}

  inline int xToStringLen(char* a) { return strlen(a)+1;}
  inline void xToString(char* s, char* a) { sprintf(s,"%s",a);}

  template <class A, class B>
  inline int xToStringLen(pair<A,B> a) { 
    return xToStringLen(a.first) + xToStringLen(a.second) + 1;
  }
  template <class A, class B>
  inline void xToString(char* s, pair<A,B> a) { 
    int l = xToStringLen(a.first);
    xToString(s,a.first);
    s[l] = ' ';
    xToString(s+l+1,a.second);
  }

  struct notZero { bool operator() (char A) {return A > 0;}};

  template <class T>
  _seq<char> arrayToString(T* A, long n) {
    long* L = newA(long,n);
    {parallel_for(long i=0; i < n; i++) L[i] = xToStringLen(A[i])+1;}
    long m = sequence::scan(L,L,n,addF<long>(),(long) 0);
    char* B = newA(char,m);
    parallel_for(long j=0; j < m; j++) 
      B[j] = 0;
    parallel_for(long i=0; i < n-1; i++) {
      xToString(B + L[i],A[i]);
      B[L[i+1] - 1] = '\n';
    }
    xToString(B + L[n-1],A[n-1]);
    B[m-1] = '\n';
    free(L);
    char* C = newA(char,m+1);
    long mm = sequence::filter(B,C,m,notZero());
    C[mm] = 0;
    free(B);
    return _seq<char>(C,mm);
  }

  template <class T>
  void writeArrayToStream(ofstream& os, T* A, long n) {
    long BSIZE = 1000000;
    long offset = 0;
    while (offset < n) {
      // Generates a string for a sequence of size at most BSIZE
      // and then wrties it to the output stream
      _seq<char> S = arrayToString(A+offset,min(BSIZE,n-offset));
      os.write(S.A, S.n);
      S.del();
      offset += BSIZE;
    }    
  }

  template <class T>
    int writeArrayToFile(string header, T* A, long n, char* fileName) {
    ofstream file (fileName, ios::out | ios::binary);
    if (!file.is_open()) {
      std::cout << "Unable to open file: " << fileName << std::endl;
      return 1;
    }
    file << header << endl;
    writeArrayToStream(file, A, n);
    file.close();
    return 0;
  }

  _seq<char> readStringFromFile(char *fileName) {
    ifstream file (fileName, ios::in | ios::binary | ios::ate);
    if (!file.is_open()) {
      std::cout << "Unable to open file: " << fileName << std::endl;
      abort();
    }
    long end = file.tellg();
    file.seekg (0, ios::beg);
    long n = end - file.tellg();
    char* bytes = newA(char,n+1);
    file.read (bytes,n);
    file.close();
    return _seq<char>(bytes,n);
  }
};


// **************************************************************
//    GRAPH I/O
// **************************************************************
using namespace benchIO;

template <class intT>
int xToStringLen(edge<intT> a) { 
  return xToStringLen(a.u) + xToStringLen(a.v) + 1;
}

template <class intT>
void xToString(char* s, edge<intT> a) { 
  int l = xToStringLen(a.u);
  xToString(s, a.u);
  s[l] = ' ';
  xToString(s+l+1, a.v);
}

namespace benchIO {
  using namespace std;

  string AdjGraphHeader = "AdjacencyGraph";
  string WghAdjGraphHeader = "WeightedAdjacencyGraph";
  string AdjHypergraphHeader = "AdjacencyHypergraph";
  string WghAdjHypergraphHeader = "WeightedAdjacencyHypergraph";

  template <class intT>
  int writeGraphToFile(graph<intT> G, char* fname) {
    long m = G.m;
    long n = G.n;
    long totalLen = 2 + n + m;
    intT *Out = newA(intT, totalLen);
    Out[0] = n;
    Out[1] = m;
    parallel_for (long i=0; i < n; i++) {
      Out[i+2] = G.V[i].degree;
    }
    long total = sequence::scan(Out+2,Out+2,n,addF<intT>(),(intT)0);
    for (long i=0; i < n; i++) {
      intT *O = Out + (2 + n + Out[i+2]);
      vertex<intT> v = G.V[i];
      for (long j = 0; j < v.degree; j++) 
	O[j] = v.Neighbors[j];
    }
    int r = writeArrayToFile(AdjGraphHeader, Out, totalLen, fname);
    free(Out);
    return r;
  }

  template <class intT>
  int writeHypergraphToFile(hypergraph<intT> G, char* fname) {
    long nv = G.nv, mv = G.mv, nh = G.nh, mh = G.mh;
    long totalLen = 4 + nv + mv + nh + mh;
    intT *Out = newA(intT, totalLen);
    Out[0] = nv; Out[1] = mv; Out[2] = nh; Out[3] = mh;
    parallel_for(long i=0;i<nv;i++) Out[4+i] = G.V[i].degree;
    sequence::scan(4+Out,4+Out,nv,addF<intT>(),(intT)0);
    for(long i=0;i<nv;i++) {
      intT *O = Out + 4 + nv + Out[4+i];
      vertex<intT> v = G.V[i];
      for(long j=0;j < v.degree; j++) O[j] = v.Neighbors[j];
    }
    parallel_for(long i=0;i<nh;i++) Out[nv+mv+4+i] = G.H[i].degree;
    sequence::scan(nv+mv+4+Out,nv+mv+4+Out,nh,addF<intT>(),(intT)0);
    for(long i=0;i<nh;i++) {
      intT *O = Out + 4 + nv + mv + nh + Out[nv+mv+4+i];
      vertex<intT> h = G.H[i];
      for(long j=0;j < h.degree; j++) O[j] = h.Neighbors[j];
    }
    int r = writeArrayToFile(AdjHypergraphHeader,Out,totalLen,fname);
    free(Out);
    return r;
  }

  template <class intT>
  int writeWghGraphToFile(wghGraph<intT> G, char* fname) {
    long m = G.m;
    long n = G.n;
    long totalLen = 2 + n + m*2;
    intT *Out = newA(intT, totalLen);
    Out[0] = n;
    Out[1] = m;
    parallel_for (long i=0; i < n; i++) {
      Out[i+2] = G.V[i].degree;
    }
    long total = sequence::scan(Out+2,Out+2,n,addF<intT>(),(intT)0);
    for (long i=0; i < n; i++) {
      intT *O = Out + (2 + n + Out[i+2]);
      wghVertex<intT> v = G.V[i];
      for (long j = 0; j < v.degree; j++) {
	O[j] = v.Neighbors[j];
	O[j+m] = v.nghWeights[j];
      }
    }
    int r = writeArrayToFile(WghAdjGraphHeader, Out, totalLen, fname);
    free(Out);
    return r;
  }

  template <class intT>
  int writeWghHypergraphToFile(wghHypergraph<intT> G, char* fname) {
    long nv = G.nv, mv = G.mv, nh = G.nh, mh = G.mh;
    long totalLen = 4 + nv + mv*2 + nh + mh*2;
    intT *Out = newA(intT, totalLen);
    Out[0] = nv; Out[1] = mv; Out[2] = nh; Out[3] = mh;
    parallel_for(long i=0;i<nv;i++) Out[4+i] = G.V[i].degree;
    sequence::scan(4+Out,4+Out,nv,addF<intT>(),(intT)0);
    for(long i=0;i<nv;i++) {
      intT *O = Out + 4 + nv + Out[4+i];
      wghVertex<intT> v = G.V[i];
      for(long j=0;j < v.degree; j++) {
	O[j] = v.Neighbors[j];
	O[j+mv] = v.nghWeights[j];
      }
    }
    parallel_for(long i=0;i<nh;i++) Out[nv+2*mv+4+i] = G.H[i].degree;
    sequence::scan(nv+2*mv+4+Out,nv+2*mv+4+Out,nh,addF<intT>(),(intT)0);
    for(long i=0;i<nh;i++) {
      intT *O = Out + 4 + nv + 2*mv + nh + Out[nv+2*mv+4+i];
      wghVertex<intT> h = G.H[i];
      for(long j=0;j < h.degree; j++) {
	O[j] = h.Neighbors[j];
	O[j+mh] = h.nghWeights[j];
      }
    }
    int r = writeArrayToFile(WghAdjHypergraphHeader,Out,totalLen,fname);
    free(Out);
    return r;
  }

  template <class intT>
  edgeArray<intT> readSNAP(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '#') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '#') break; 
    }
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();

    words W = stringToWords(S2, S.n-k);
    long n = W.m/2;
    edge<intT> *E = newA(edge<intT>,n);
    {parallel_for(long i=0; i < n; i++)
      E[i] = edge<intT>(atol(W.Strings[2*i]), 
		  atol(W.Strings[2*i + 1]));}
    W.del();

    long maxR = 0;
    long maxC = 0;
    for (long i=0; i < n; i++) {
      maxR = max<intT>(maxR, E[i].u);
      maxC = max<intT>(maxC, E[i].v);
    }
    long maxrc = max<intT>(maxR,maxC) + 1;
    return edgeArray<intT>(E, maxrc, maxrc, n);
  }

  template <class intT>
  wghEdgeArray<intT> readWghSNAP(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '#') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '#') break; 
    }
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();

    words W = stringToWords(S2, S.n-k);
    long n = W.m/3;
    wghEdge<intT> *E = newA(wghEdge<intT>,n);
    {parallel_for(long i=0; i < n; i++)
      E[i] = wghEdge<intT>(atol(W.Strings[3*i]), 
			atol(W.Strings[3*i + 1]),
			atol(W.Strings[3*i + 2]));}
    W.del();

    long maxR = 0;
    long maxC = 0;
    for (long i=0; i < n; i++) {
      maxR = max<intT>(maxR, E[i].u);
      maxC = max<intT>(maxC, E[i].v);
    }
    long maxrc = max<intT>(maxR,maxC) + 1;
    return wghEdgeArray<intT>(E, maxrc, maxrc, n);
  }

  //SNAP format
  template <class intT>
  hyperedgeArray<intT> readHyperedges(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '#') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '#') break; 
    }
    
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();
    
    lines W = stringToLinesOfWords(S2, S.n-k);
    long nh = W.numLines;
    long m = W.m;
    edge<intT> *VE = newA(edge<intT>,m);
    edge<intT> *HE = newA(edge<intT>,m);
    
    {parallel_for(long i=0;i<nh;i++) {
	long offset = W.Lines[i];
	long degree = (i == nh-1) ? m-W.Lines[i] : W.Lines[i+1]-W.Lines[i];
	for(long j=0;j<degree;j++) {
	  VE[offset+j] = edge<intT>(atol(W.Strings[offset+j]),i);
	  HE[offset+j] = edge<intT>(i,atol(W.Strings[offset+j]));
	}
      }} 
    W.del();

    long maxID = 0;
    for (long i=0; i < m; i++) {
      maxID = max<intT>(maxID, VE[i].u);
    }
    maxID++;
    //compress IDs into contiguous range
    intT* IDs = newA(intT,maxID);
    {parallel_for(long i=0;i<maxID;i++) IDs[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDs[VE[i].u]) CAS(&IDs[VE[i].u],(intT)0,(intT)1);}
    long nv = sequence::plusScan(IDs,IDs,maxID);

    {parallel_for(long i=0;i<m;i++) {
	VE[i].u = IDs[VE[i].u];
	HE[i].v = IDs[HE[i].v];
      }}
    free(IDs);
    return hyperedgeArray<intT>(VE,HE,nv,nh,m,m);
  }

  //SNAP format
  //one hyperedge per line followed by weight on a separate line
  template <class intT>
    wghHyperedgeArray<intT> readWghHyperedges(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '#') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '#') break; 
    }
    
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();
    
    lines W = stringToLinesOfWords(S2, S.n-k);
    long n = W.numLines/2;
    long m = W.m-n;
    wghEdge<intT> *VE = newA(wghEdge<intT>,m);
    wghEdge<intT> *HE = newA(wghEdge<intT>,m);
    {parallel_for(long i=0;i<n;i++) {
	long offset = W.Lines[2*i];
	long degree = W.Lines[2*i+1]-W.Lines[2*i];
	for(long j=0;j<degree;j++) {
	  VE[offset+j-i] = wghEdge<intT>(atol(W.Strings[offset+j]),i,atol(W.Strings[W.Lines[2*i+1]]));
	  HE[offset+j-i] = wghEdge<intT>(i,atol(W.Strings[offset+j]),atol(W.Strings[W.Lines[2*i+1]]));
	}
      }} 
    W.del();

    long maxID = 0;
    for (long i=0; i < m; i++) {
      maxID = max<intT>(maxID, VE[i].u);
    }
    maxID++;
    //compress IDs into contiguous range
    intT* IDs = newA(intT,maxID);
    {parallel_for(long i=0;i<maxID;i++) IDs[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDs[VE[i].u]) CAS(&IDs[VE[i].u],(intT)0,(intT)1);}
    long nv = sequence::plusScan(IDs,IDs,maxID);

    {parallel_for(long i=0;i<m;i++) {
	VE[i].u = IDs[VE[i].u];
	HE[i].v = IDs[HE[i].v];
      }}
    free(IDs);
    return wghHyperedgeArray<intT>(VE,HE,nv,n,m,m);
  }

  //KONECT format
  template <class intT>
  hyperedgeArray<intT> readKONECT(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '%') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '%') break; 
    }
    
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();
    
    words W = stringToWords(S2, S.n-k);
    long m = W.m/2;
    edge<intT> *VE = newA(edge<intT>,m);
    edge<intT> *HE = newA(edge<intT>,m);

    {parallel_for(long i=0; i < m; i++) {
	VE[i] = edge<intT>(atol(W.Strings[2*i]), 
			  atol(W.Strings[2*i + 1]));
	HE[i] = edge<intT>(atol(W.Strings[2*i+1]),atol(W.Strings[2*i]));
      }}
    W.del();

    long maxV = 0, maxH = 0;
    for (long i=0; i < m; i++) {
      maxV = max<intT>(maxV,VE[i].u);
      maxH = max<intT>(maxH,HE[i].u);
    }
    maxV++; maxH++;
    //compress IDs into contiguous range
    intT* IDsV = newA(intT,maxV);
    {parallel_for(long i=0;i<maxV;i++) IDsV[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDsV[VE[i].u]) CAS(&IDsV[VE[i].u],(intT)0,(intT)1);}
    long nv = sequence::plusScan(IDsV,IDsV,maxV);

    intT* IDsH = newA(intT,maxH);
    {parallel_for(long i=0;i<maxH;i++) IDsH[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDsH[VE[i].v]) CAS(&IDsH[VE[i].v],(intT)0,(intT)1);}
    long nh = sequence::plusScan(IDsH,IDsH,maxH);

    {parallel_for(long i=0;i<m;i++) {
    	VE[i].u = IDsV[VE[i].u];
	VE[i].v = IDsH[VE[i].v];
	HE[i].u = IDsH[HE[i].u];
	HE[i].v = IDsV[HE[i].v];
      }}
    free(IDsV);
    free(IDsH);
    return hyperedgeArray<intT>(VE,HE,nv,nh,m,m);
  }

  //KONECT format
  template <class intT>
    wghHyperedgeArray<intT> readWghKONECT(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    char* S2 = newA(char,S.n);
    //ignore starting lines with '#' and find where to start in file 
    long k=0;
    while(1) {
      if(S.A[k] == '%') {
	while(S.A[k++] != '\n') continue;
      }
      if(k >= S.n || S.A[k] != '%') break; 
    }
    
    parallel_for(long i=0;i<S.n-k;i++) S2[i] = S.A[k+i];
    S.del();
    
    words W = stringToWords(S2, S.n-k);
    long m = W.m/4;
    wghEdge<intT> *VE = newA(wghEdge<intT>,m);
    wghEdge<intT> *HE = newA(wghEdge<intT>,m);

    {parallel_for(long i=0; i < m; i++) {
	VE[i] = wghEdge<intT>(atol(W.Strings[4*i]), 
			   atol(W.Strings[4*i+1]),
			   atol(W.Strings[4*i+2]));
	HE[i] = wghEdge<intT>(atol(W.Strings[4*i+1]),
			   atol(W.Strings[4*i]),
			   atol(W.Strings[4*i+2]));
      }}
    W.del();

    long maxV = 0, maxH = 0;
    for (long i=0; i < m; i++) {
      maxV = max<intT>(maxV,VE[i].u);
      maxH = max<intT>(maxH,HE[i].u);
    }
    maxV++; maxH++;
    //compress IDs into contiguous range
    intT* IDsV = newA(intT,maxV);
    {parallel_for(long i=0;i<maxV;i++) IDsV[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDsV[VE[i].u]) CAS(&IDsV[VE[i].u],(intT)0,(intT)1);}
    long nv = sequence::plusScan(IDsV,IDsV,maxV);

    intT* IDsH = newA(intT,maxH);
    {parallel_for(long i=0;i<maxH;i++) IDsH[i]=0;}
    {parallel_for(long i=0; i < m; i++) if(!IDsH[VE[i].v]) CAS(&IDsH[VE[i].v],(intT)0,(intT)1);}
    long nh = sequence::plusScan(IDsH,IDsH,maxH);

    {parallel_for(long i=0;i<m;i++) {
    	VE[i].u = IDsV[VE[i].u];
	VE[i].v = IDsH[VE[i].v];
	HE[i].u = IDsH[HE[i].u];
	HE[i].v = IDsV[HE[i].v];
      }}
    free(IDsV);
    free(IDsH);
    return wghHyperedgeArray<intT>(VE,HE,nv,nh,m,m);
  }

  template <class intT>
  graph<intT> readGraphFromFile(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != AdjGraphHeader) {
      cout << "Bad input file: missing header: " << AdjGraphHeader << endl;
      abort();
    }

    long len = W.m -1;
    uintT * In = newA(uintT, len);
    {parallel_for(long i=0; i < len; i++) In[i] = atol(W.Strings[i + 1]);}
    W.del();
    
    long n = In[0];
    long m = In[1];

    if (len != n + m + 2) {
      cout << "Bad input file: length = "<<len<< " n+m+2 = " << n+m+2 << endl;
      abort();
    }
    vertex<intT> *v = newA(vertex<intT>,n);
    uintT* offsets = In+2;
    uintT* edges = In+2+n;

    parallel_for (uintT i=0; i < n; i++) {
      uintT o = offsets[i];
      uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
      v[i].degree = l;
      v[i].Neighbors = (intT*)(edges+o);
    }
    return graph<intT>(v,(intT)n,(uintT)m,(intT*)In);
  }

  template <class intT>
    wghGraph<intT> readWghGraphFromFile(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != WghAdjGraphHeader) {
      cout << "Bad input file" << endl;
      abort();
    }
    
    long len = W.m -1;
    intT * In = newA(intT, len);
    {parallel_for(long i=0; i < len; i++) In[i] = atol(W.Strings[i + 1]);}
    W.del();
    
    long n = In[0];
    long m = In[1];

    if (len != n + 2*m + 2) {
      cout << "Bad input file" << endl;
      abort();
    }
    wghVertex<intT> *v = newA(wghVertex<intT>,n);
    uintT* offsets = (uintT*)In+2;
    uintT* edges = (uintT*)In+2+n;
    intT* weights = In+2+n+m;
    parallel_for (uintT i=0; i < n; i++) {
      uintT o = offsets[i];
      uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
      v[i].degree = l;
      v[i].Neighbors = (intT*)(edges+o);
      v[i].nghWeights = (weights+o);
    }
    return wghGraph<intT>(v,(intT)n,(uintT)m,(intT*)In,weights);
  }

  template <class intT>
  hypergraph<intT> readHypergraphFromFile(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != AdjHypergraphHeader) {
      cout << "Bad input file: missing header: " << AdjHypergraphHeader << endl;
      abort();
    }

    long len = W.m -1;
    uintT * In = newA(uintT, len);
    {parallel_for(long i=0; i < len; i++) In[i] = atol(W.Strings[i + 1]);}
    W.del();
    
    long nv = In[0];
    long mv = In[1];
    long nh = In[2];
    long mh = In[3];
    
    if (len != nv + mv + nh + mh + 4) {
      cout << "Bad input file: length = "<<len<< " nv+mv+nh+mh+4 = " << nv+mv+nh+mh+4 << endl;
      abort();
    }
    vertex<intT> *v = newA(vertex<intT>,nv);
    vertex<intT> *h = newA(vertex<intT>,nh);
    uintT* offsetsV = In+4;
    uintT* edgesV = In+4+nv;
    uintT* offsetsH = In+4+nv+mv;
    uintT* edgesH = In+4+nv+mv+nh;

    parallel_for (uintT i=0; i < nv; i++) {
      uintT o = offsetsV[i];
      uintT l = ((i == nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
      v[i].degree = l;
      v[i].Neighbors = (intT*)(edgesV+o);
    }
    parallel_for (uintT i=0; i < nh; i++) {
      uintT o = offsetsH[i];
      uintT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
      h[i].degree = l;
      h[i].Neighbors = (intT*)(edgesH+o);
    }

    return hypergraph<intT>(v,h,nv,mv,nh,mh,(intT*)In);
  }

  template <class intT>
    wghHypergraph<intT> readWghHypergraphFromFile(char* fname) {
    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != WghAdjHypergraphHeader) {
      cout << "Bad input file: missing header: " << WghAdjHypergraphHeader << endl;
      abort();
    }
    
    long len = W.m -1;
    intT * In = newA(intT, len);
    {parallel_for(long i=0; i < len; i++) In[i] = atol(W.Strings[i + 1]);}
    W.del();
    
    long nv = In[0];
    long mv = In[1];
    long nh = In[2];
    long mh = In[3];
    
    if (len != nv + 2*mv + nh + 2*mh + 4) {
      cout << "Bad input file" << endl;
      abort();
    }
    wghVertex<intT> *v = newA(wghVertex<intT>,nv);
    wghVertex<intT> *h = newA(wghVertex<intT>,nh);
    uintT* offsetsV = (uintT*)In+4;
    uintT* edgesV = (uintT*)In+4+nv;
    intT* weightsV = In+4+nv+mv;
    uintT* offsetsH = (uintT*)In+4+nv+2*mv;
    uintT* edgesH = (uintT*)In+4+nv+2*mv+nh;
    intT* weightsH = In+4+nv+2*mv+nh+mh;
    
    parallel_for (uintT i=0; i < nv; i++) {
      uintT o = offsetsV[i];
      uintT l = ((i == nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
      v[i].degree = l;
      v[i].Neighbors = (intT*)(edgesV+o);
      v[i].nghWeights = (weightsV+o);
    }

    parallel_for (uintT i=0; i < nh; i++) {
      uintT o = offsetsH[i];
      uintT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
      h[i].degree = l;
      h[i].Neighbors = (intT*)(edgesH+o);
      h[i].nghWeights = (weightsH+o);
    }

    return wghHypergraph<intT>(v,h,nv,mv,nh,mh,In);
  }

};

#endif // _BENCH_GRAPH_IO
