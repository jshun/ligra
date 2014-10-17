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

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "parallel.h"
#include "quickSort.h"
using namespace std;

typedef pair<uintE,uintE> intPair;
typedef pair<uintE, pair<uintE,intE> > intTriple;

template <class E>
struct pairFirstCmp {
  bool operator() (pair<intE,E> a, pair<intE,E> b) {
    return a.first < b.first; }
};

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

// parallel code for converting a string to words
words stringToWords(char *Str, long n) {
  {parallel_for (long i=0; i < n; i++) 
      if (isSpace(Str[i])) Str[i] = 0; }

  // mark start of words
  bool *FL = newA(bool,n);
  FL[0] = Str[0];
  {parallel_for (long i=1; i < n; i++) FL[i] = Str[i] && !Str[i-1];}
    
  // offset for each start of word
  _seq<long> Off = sequence::packIndex<long>(FL, n);
  long m = Off.n;
  long *offsets = Off.A;

  // pointer to each start of word
  char **SA = newA(char*, m);
  {parallel_for (long j=0; j < m; j++) SA[j] = Str+offsets[j];}

  free(offsets); free(FL);
  return words(Str,n,SA,m);
}

template <class vertex>
graph<vertex> readGraphFromFile(char* fname, bool isSymmetric) {
  _seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
#ifndef WEIGHTED
  if (W.Strings[0] != (string) "AdjacencyGraph") {
#else
  if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m -1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
#ifndef WEIGHTED
  if (len != n + m + 2) {
#else
  if (len != n + 2*m + 2) {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  intT* offsets = newA(intT,n);
#ifndef WEIGHTED
  intE* edges = newA(intE,m);
#else
  intE* edges = newA(intE,2*m);
#endif

  {parallel_for(long i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);}
  {parallel_for(long i=0; i<m; i++) {
#ifndef WEIGHTED
      edges[i] = atol(W.Strings[i+n+3]); 
#else
      edges[2*i] = atol(W.Strings[i+n+3]); 
      edges[2*i+1] = atol(W.Strings[i+n+m+3]);
#endif
    }}
  //W.del(); // to deal with performance bug in malloc
    
  vertex* v = newA(vertex,n);

  {parallel_for (uintT i=0; i < n; i++) {
    uintT o = offsets[i];
    uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].setOutDegree(l); 
#ifndef WEIGHTED
    v[i].setOutNeighbors(edges+o);     
#else
    v[i].setOutNeighbors(edges+2*o);
#endif
    }}

  if(!isSymmetric) {
    intT* tOffsets = newA(intT,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    intE* inEdges = newA(intE,m);
    intPair* temp = newA(intPair,m);
#else
    intE* inEdges = newA(intE,2*m);
    intTriple* temp = newA(intTriple,m);
#endif
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<v[i].getOutDegree();j++){	  
#ifndef WEIGHTED
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),i);
#else
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),make_pair(i,v[i].getOutWeight(j)));
#endif
      }
      }}
    free(offsets);

#ifndef WEIGHTED
    quickSort(temp,m,pairFirstCmp<intE>());
#else
    quickSort(temp,m,pairFirstCmp<intPair>());
#endif

    tOffsets[temp[0].first] = 0; 
#ifndef WEIGHTED
    inEdges[0] = temp[0].second;
#else
    inEdges[0] = temp[0].second.first;
    inEdges[1] = temp[0].second.second;
#endif
    {parallel_for(intT i=1;i<m;i++) {
#ifndef WEIGHTED
      inEdges[i] = temp[i].second;
#else
      inEdges[2*i] = temp[i].second.first; 
      inEdges[2*i+1] = temp[i].second.second;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}

    free(temp);
 
    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,n,minF<intT>(),(intT)m);

    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
#ifndef WEIGHTED
      v[i].setInNeighbors(inEdges+o);
#else
      v[i].setInNeighbors(inEdges+2*o);
#endif
      }}    

    free(tOffsets);
    return graph<vertex>(v,n,m,edges,inEdges);
  }
  else {
    free(offsets);
    return graph<vertex>(v,n,m,edges);
  }
}

template <class vertex>
graph<vertex> readGraphFromBinary(char* iFile, bool isSymmetric) {
  char* config = (char*) ".config";
  char* adj = (char*) ".adj";
  char* idx = (char*) ".idx";
  char configFile[strlen(iFile)+7];
  char adjFile[strlen(iFile)+4];
  char idxFile[strlen(iFile)+4];
  strcpy(configFile,iFile);
  strcpy(adjFile,iFile);
  strcpy(idxFile,iFile);
  strcat(configFile,config);
  strcat(adjFile,adj);
  strcat(idxFile,idx);

  ifstream in(configFile, ifstream::in);
  long n;
  in >> n;
  in.close();

  ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
  long m = size/sizeof(uint);

  char* s = (char *) malloc(size);
  in2.read(s,size);
  in2.close();
  
  uintE* edges = (uintE*) s;
  ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  if(n != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

  char* t = (char *) malloc(size);
  in3.read(t,size);
  in3.close();
  intT* offsets = (intT*) t;

  vertex* v = newA(vertex,n);
#ifdef WEIGHTED
  intE* edgesAndWeights = newA(intE,2*m);
  {parallel_for(long i=0;i<m;i++) {
    edgesAndWeights[2*i] = edges[i];
    edgesAndWeights[2*i+1] = 1; //give them unit weight
    }}
  free(edges);
#endif

  {parallel_for(long i=0;i<n;i++) {
    uintT o = offsets[i];
    uintT l = ((i==n-1) ? m : offsets[i+1])-offsets[i];
      v[i].setOutDegree(l); 
#ifndef WEIGHTED
      v[i].setOutNeighbors((intE*)edges+o); 
#else
      v[i].setOutNeighbors(edgesAndWeights+2*o);
#endif
    }}

  if(!isSymmetric) {
    intT* tOffsets = newA(intT,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    uintE* inEdges = newA(uintE,m);
#else
    intE* inEdges = newA(intE,2*m);
#endif
    intPair* temp = newA(intPair,m);
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<v[i].getOutDegree();j++){
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),i);
      }
      }}
    free(offsets);
    quickSort(temp,m,pairFirstCmp<intE>());

    tOffsets[temp[0].first] = 0; 
    inEdges[0] = temp[0].second;
#ifdef WEIGHTED
    inEdges[1] = 1;
#endif
    {parallel_for(intT i=1;i<m;i++) {
#ifndef WEIGHTED
      inEdges[i] = temp[i].second;
#else
      inEdges[2*i] = temp[i].second;
      inEdges[2*i+1] = 1;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,n,minF<intT>(),(intT)m);

    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
#ifndef WEIGHTED
      v[i].setInNeighbors((intE*)inEdges+o);
#else
      v[i].setInNeighbors((intE*)(inEdges+2*o));
#endif
      }}
    free(tOffsets);
    return graph<vertex>(v,n,m,(intE*)edges, (intE*)inEdges);
  }
  free(offsets);
  
  return graph<vertex>(v,n,m,(intE*)edges);
}

template <class vertex>
graph<vertex> readGraph(char* iFile, bool symmetric, bool binary) {
  if(binary) return readGraphFromBinary<vertex>(iFile,symmetric); 
  else return readGraphFromFile<vertex>(iFile,symmetric);
}
