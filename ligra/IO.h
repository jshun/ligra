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
#pragma once
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include "blockRadixSort.h"
#include "graph.h"
#include "parallel.h"
#include "quickSort.h"
#include "utils.h"
using namespace ligra;
using namespace std;

// Used for non-weighted graphs:
// <vertex_id, vertex_id>
typedef pair<uintE, uintE> intPair;

// Used for weighted graphs:
// <vertex_id, <vertex_id, weight>>
typedef pair<uintE, pair<uintE, intE>> intTriple;

// Used for weighted temporal graphs:
// <vertex_id, vertex_id, weight, start_time, end_time>
typedef tuple<uintE, uintE, intE, intE, intE> intQuintuple;

template <class E>
struct pairFirstCmp {
  bool operator()(const pair<uintE, E>& a, const pair<uintE, E>& b) {
    return a.first < b.first;
  }
};

template <class E>
struct getFirst {
  uintE operator()(const pair<uintE, E>& a) { return a.first; }
};

// These are needed for weighted temporal graphs.
struct quintupleFirstCmp {
  bool operator()(const intQuintuple& a, const intQuintuple& b) {
    return get<0>(a) < get<1>(b);
  }
};
struct quintupleGetFirst {
  uintE operator()(const intQuintuple& a) { return get<0>(a); }
};

template <class IntType>
struct pairBothCmp {
  bool operator()(pair<uintE, IntType> a, pair<uintE, IntType> b) {
    if (a.first != b.first) return a.first < b.first;
    return a.second < b.second;
  }
};

// A structure that keeps a sequence of strings all allocated from
// the same block of memory
struct words {
  long n;          // total number of characters
  char* Chars;     // array storing all strings
  long m;          // number of substrings
  char** Strings;  // pointers to strings (all should be null terminated)
  words() {}
  words(char* C, long nn, char** S, long mm)
      : Chars(C), n(nn), Strings(S), m(mm) {}
  void del() {
    free(Chars);
    free(Strings);
  }
};

inline bool isSpace(char c) {
  switch (c) {
    case '\r':
    case '\t':
    case '\n':
    case 0:
    case ' ':
      return true;
    default:
      return false;
  }
}

_seq<char> mmapStringFromFile(const char* filename) {
  struct stat sb;
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(-1);
  }
  if (fstat(fd, &sb) == -1) {
    perror("fstat");
    exit(-1);
  }
  if (!S_ISREG(sb.st_mode)) {
    perror("not a file\n");
    exit(-1);
  }
  char* p =
      static_cast<char*>(mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
  if (p == MAP_FAILED) {
    perror("mmap");
    exit(-1);
  }
  if (close(fd) == -1) {
    perror("close");
    exit(-1);
  }
  size_t n = sb.st_size;
  //  char *bytes = newA(char, n);
  //  parallel_for(size_t i=0; i<n; i++) {
  //    bytes[i] = p[i];
  //  }
  //  if (munmap(p, sb.st_size) == -1) {
  //    perror("munmap");
  //    exit(-1);
  //  }
  //  cout << "mmapped" << endl;
  //  free(bytes);
  //  exit(0);
  return _seq<char>(p, n);
}

_seq<char> readStringFromFile(char* fileName) {
  ifstream file(fileName, ios::in | ios::binary | ios::ate);
  if (!file.is_open()) {
    std::cout << "Unable to open file: " << fileName << std::endl;
    abort();
  }
  long end = file.tellg();
  file.seekg(0, ios::beg);
  long n = end - file.tellg();
  char* bytes = newA(char, n + 1);
  file.read(bytes, n);
  file.close();
  return _seq<char>(bytes, n);
}

// parallel code for converting a string to words
words stringToWords(char* Str, long n) {
  { parallel_for(long i = 0; i < n; i++) if (isSpace(Str[i])) Str[i] = 0; }

  // mark start of words
  bool* FL = newA(bool, n);
  FL[0] = Str[0];
  { parallel_for(long i = 1; i < n; i++) FL[i] = Str[i] && !Str[i - 1]; }

  // offset for each start of word
  _seq<long> Off = ligra::sequence::packIndex<long>(FL, n);
  long m = Off.n;
  long* offsets = Off.A;

  // pointer to each start of word
  char** SA = newA(char*, m);
  { parallel_for(long j = 0; j < m; j++) SA[j] = Str + offsets[j]; }

  free(offsets);
  free(FL);
  return words(Str, n, SA, m);
}

template <class vertex>
graph<vertex> readGraphFromFile(char* fname, bool isSymmetric, bool mmap) {
  words W;
  if (mmap) {
    _seq<char> S = mmapStringFromFile(fname);
    char* bytes = newA(char, S.n);
    // Cannot mutate the graph unless we copy.
    parallel_for(size_t i = 0; i < S.n; i++) { bytes[i] = S.A[i]; }
    if (munmap(S.A, S.n) == -1) {
      perror("munmap");
      exit(-1);
    }
    S.A = bytes;
    W = stringToWords(S.A, S.n);
  } else {
    _seq<char> S = readStringFromFile(fname);
    W = stringToWords(S.A, S.n);
  }
#ifdef WEIGHTED
  if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
#elif WEIGHTED_TEMPORAL
  if (W.Strings[0] != (string) "WeightedTemporalAdjacencyGraph") {
#else
  if (W.Strings[0] != (string) "AdjacencyGraph") {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m - 1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
#ifdef WEIGHTED
  if (len != n + 2 * m + 2) {
#elif WEIGHTED_TEMPORAL
  if (len != n + 4 * m + 2) {
#else
  if (len != n + m + 2) {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  uintT* offsets = newA(uintT, n);
#ifdef WEIGHTED
  intE* edges = newA(intE, 2 * m);
#elif WEIGHTED_TEMPORAL
  intE* edges = newA(intE, 4 * m);
#else
  uintE* edges = newA(uintE, m);
#endif

  { parallel_for(long i = 0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]); }
  {
    parallel_for(long i = 0; i < m; i++) {
#ifdef WEIGHTED
      edges[2 * i] = atol(W.Strings[i + n + 3]);
      edges[2 * i + 1] = atol(W.Strings[i + n + m + 3]);
#elif WEIGHTED_TEMPORAL
      edges[4 * i] = atol(W.Strings[i + n + 3]);
      edges[4 * i + 1] = atol(W.Strings[i + n + m + 3]);
      edges[4 * i + 2] = atol(W.Strings[i + n + 2 * m + 3]);
      edges[4 * i + 3] = atol(W.Strings[i + n + 3 * m + 3]);
#else
      edges[i] = atol(W.Strings[i + n + 3]);
#endif
    }
  }
  // W.del(); // to deal with performance bug in malloc

  vertex* v = newA(vertex, n);

  {
    parallel_for(uintT i = 0; i < n; i++) {
      uintT o = offsets[i];
      uintT l = ((i == n - 1) ? m : offsets[i + 1]) - offsets[i];
      v[i].setOutDegree(l);
#ifdef WEIGHTED
      v[i].setOutNeighbors(edges + 2 * o);
#elif WEIGHTED_TEMPORAL
      v[i].setOutNeighbors(edges + 4 * o);
#else
      v[i].setOutNeighbors(edges + o);
#endif
    }
  }

  if (!isSymmetric) {
    uintT* tOffsets = newA(uintT, n);
    { parallel_for(long i = 0; i < n; i++) tOffsets[i] = INT_T_MAX; }
#ifdef WEIGHTED
    intTriple* temp = newA(intTriple, m);
#elif WEIGHTED_TEMPORAL
    intQuintuple* temp = newA(intQuintuple, m);
#else
    intPair* temp = newA(intPair, m);
#endif
    {
      parallel_for(long i = 0; i < n; i++) {
        uintT o = offsets[i];
        for (uintT j = 0; j < v[i].getOutDegree(); j++) {
#ifdef WEIGHTED
          temp[o + j] =
              make_pair(v[i].getOutNeighbor(j) /* first */,
                        make_pair(i /* second.first */,
                                  v[i].getOutWeight(j) /* second.second */));
#elif WEIGHTED_TEMPORAL
          temp[o + j] =
              make_tuple(v[i].getOutNeighbor(j), i, v[i].getOutWeight(j),
                         v[i].getOutStartTime(j), v[i].getOutEndTime(j));
#else
          temp[o + j] = make_pair(v[i].getOutNeighbor(j), i);
#endif
        }
      }
    }
    free(offsets);

#ifdef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, getFirst<intPair>());
#else
    quickSort(temp, m, pairFirstCmp<intPair>());
#endif  // LOWMEM
#elif WEIGHTED_TEMPORAL
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, quintupleGetFirst());
#else
    quickSort(temp, m, quintupleFirstCmp());
#endif  // LOWMEM
#else   // WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, getFirst<uintE>());
#else
    quickSort(temp, m, pairFirstCmp<uintE>());
#endif  // LOWMEM
#endif  // WEIGHTED

#ifdef WEIGHTED
    tOffsets[temp[0].first] = 0;
    intE* inEdges = newA(intE, 2 * m);
    inEdges[0] = temp[0].second.first;
    inEdges[1] = temp[0].second.second;
#elif WEIGHTED_TEMPORAL
    tOffsets[get<0>(temp[0])] = 0;
    intE* inEdges = newA(intE, 4 * m);
    inEdges[0] = get<1>(temp[0]);
    inEdges[1] = get<2>(temp[0]);
    inEdges[2] = get<3>(temp[0]);
    inEdges[3] = get<4>(temp[0]);
#else
    tOffsets[temp[0].first] = 0;
    uintE* inEdges = newA(uintE, m);
    inEdges[0] = temp[0].second;
#endif  // WEIGHTED
    {
      parallel_for(long i = 1; i < m; i++) {
#ifdef WEIGHTED
        inEdges[2 * i] = temp[i].second.first;
        inEdges[2 * i + 1] = temp[i].second.second;
        if (temp[i].first != temp[i - 1].first) {
          tOffsets[temp[i].first] = i;
        }
#elif WEIGHTED_TEMPORAL
        inEdges[4 * i] = get<1>(temp[i]);
        inEdges[4 * i + 1] = get<2>(temp[i]);
        inEdges[4 * i + 2] = get<3>(temp[i]);
        inEdges[4 * i + 3] = get<4>(temp[i]);
        if (get<0>(temp[i]) != get<0>(temp[i - 1])) {
          tOffsets[get<0>(temp[i])] = i;
        }
#else
        inEdges[i] = temp[i].second;
        if (temp[i].first != temp[i - 1].first) {
          tOffsets[temp[i].first] = i;
        }
#endif
      }
    }

    free(temp);

    // fill in offsets of degree 0 vertices by taking closest non-zero
    // offset to the right
    ligra::sequence::scanIBack(tOffsets, tOffsets, n, minF<uintT>(), (uintT)m);

    {
      parallel_for(long i = 0; i < n; i++) {
        uintT o = tOffsets[i];
        uintT l = ((i == n - 1) ? m : tOffsets[i + 1]) - tOffsets[i];
        v[i].setInDegree(l);
#ifdef WEIGHTED
        v[i].setInNeighbors(inEdges + 2 * o);
#elif WEIGHTED_TEMPORAL
        v[i].setInNeighbors(inEdges + 4 * o);
#else
        v[i].setInNeighbors(inEdges + o);
#endif
      }
    }

    free(tOffsets);
    Uncompressed_Mem<vertex>* mem =
        new Uncompressed_Mem<vertex>(v, n, m, edges, inEdges);
    return graph<vertex>(v, n, m, mem);
  } else {
    free(offsets);
    Uncompressed_Mem<vertex>* mem =
        new Uncompressed_Mem<vertex>(v, n, m, edges);
    return graph<vertex>(v, n, m, mem);
  }
}

template <class vertex>
graph<vertex> readGraphFromBinary(char* iFile, bool isSymmetric) {
  char* config = (char*)".config";
  char* adj = (char*)".adj";
  char* idx = (char*)".idx";
  char configFile[strlen(iFile) + strlen(config) + 1];
  char adjFile[strlen(iFile) + strlen(adj) + 1];
  char idxFile[strlen(iFile) + strlen(idx) + 1];
  *configFile = *adjFile = *idxFile = '\0';
  strcat(configFile, iFile);
  strcat(adjFile, iFile);
  strcat(idxFile, iFile);
  strcat(configFile, config);
  strcat(adjFile, adj);
  strcat(idxFile, idx);

  ifstream in(configFile, ifstream::in);
  long n;
  in >> n;
  in.close();

  ifstream in2(adjFile, ifstream::in | ios::binary);  // stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
#ifdef WEIGHTED
  long m = size / (2 * sizeof(uint));
#elif WEIGHTED_TEMPORAL
  long m = size / (4 * sizeof(uint));
#else
  long m = size / sizeof(uint);
#endif
  char* s = (char*)malloc(size);
  in2.read(s, size);
  in2.close();
  uintE* edges = (uintE*)s;

  ifstream in3(idxFile, ifstream::in | ios::binary);  // stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  if (n != size / sizeof(intT)) {
    cout << "File size wrong\n";
    abort();
  }

  char* t = (char*)malloc(size);
  in3.read(t, size);
  in3.close();
  uintT* offsets = (uintT*)t;

  vertex* v = newA(vertex, n);
#ifdef WEIGHTED
  intE* edgesAndWeights = newA(intE, 2 * m);
  {
    parallel_for(long i = 0; i < m; i++) {
      edgesAndWeights[2 * i] = edges[i];
      edgesAndWeights[2 * i + 1] = edges[i + m];
    }
  }
// free(edges);
#elif WEIGHTED_TEMPORAL
  intE* edgesAndWeights = newA(intE, 4 * m);
  {
    parallel_for(long i = 0; i < m; i++) {
      edgesAndWeights[4 * i] = edges[i];
      edgesAndWeights[4 * i + 1] = edges[i + m];
      edgesAndWeights[4 * i + 2] = edges[i + 2 * m];
      edgesAndWeights[4 * i + 3] = edges[i + 3 * m];
    }
  }
#endif  // WEIGHTED
  {
    parallel_for(long i = 0; i < n; i++) {
      uintT o = offsets[i];
      uintT l = ((i == n - 1) ? m : offsets[i + 1]) - offsets[i];
      v[i].setOutDegree(l);
#ifdef WEIGHTED
      v[i].setOutNeighbors(edgesAndWeights + 2 * o);
#elif WEIGHTED_TEMPORAL
      v[i].setOutNeighbors(edgesAndWeights + 4 * o);
#else
      v[i].setOutNeighbors((uintE*)edges + o);
#endif
    }
  }

  if (!isSymmetric) {
    uintT* tOffsets = newA(uintT, n);
    { parallel_for(long i = 0; i < n; i++) tOffsets[i] = INT_T_MAX; }
#ifdef WEIGHTED
    intTriple* temp = newA(intTriple, m);
#elif WEIGHTED_TEMPORAL
    intQuintuple* temp = newA(intQuintuple, m);
#else
    intPair* temp = newA(intPair, m);
#endif
    {
      parallel_for(intT i = 0; i < n; i++) {
        uintT o = offsets[i];
        for (uintT j = 0; j < v[i].getOutDegree(); j++) {
#ifdef WEIGHTED
          temp[o + j] =
              make_pair(v[i].getOutNeighbor(j) /* first */,
                        make_pair(i /* second.first */,
                                  v[i].getOutWeight(j) /* second.second */));
#elif WEIGHTED_TEMPORAL
          temp[o + j] =
              make_tuple(v[i].getOutNeighbor(j), i, v[i].getOutWeight(j),
                         v[i].getOutStartTime(j), v[i].getOutEndTime(j));
#else
          temp[o + j] = make_pair(v[i].getOutNeighbor(j), i);
#endif
        }
      }
    }
    free(offsets);
#ifdef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, getFirst<intPair>());
#else
    quickSort(temp, m, pairFirstCmp<intPair>());
#endif  // LOWMEM
#elif WEIGHTED_TEMPORAL
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, quintupleGetFirst());
#else
    quickSort(temp, m, quintupleFirstCmp());
#endif  // LOWMEM
#else   // WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp, m, n + 1, getFirst<uintE>());
#else
    quickSort(temp, m, pairFirstCmp<uintE>());
#endif  // LOWMEM
#endif  // WEIGHTED

#ifdef WEIGHTED
    tOffsets[temp[0].first] = 0;
    intE* inEdges = newA(intE, 2 * m);
    inEdges[0] = temp[0].second.first;
    inEdges[1] = temp[0].second.second;
#elif WEIGHTED_TEMPORAL
    tOffsets[get<0>(temp[0])] = 0;
    intE* inEdges = newA(intE, 4 * m);
    inEdges[0] = get<1>(temp[0]);
    inEdges[1] = get<2>(temp[0]);
    inEdges[2] = get<3>(temp[0]);
    inEdges[3] = get<4>(temp[0]);
#else
    tOffsets[temp[0].first] = 0;
    uintE* inEdges = newA(uintE, m);
    inEdges[0] = temp[0].second;
#endif  // WEIGHTED
    {
      parallel_for(long i = 1; i < m; i++) {
#ifdef WEIGHTED
        inEdges[2 * i] = temp[i].second.first;
        inEdges[2 * i + 1] = temp[i].second.second;
        if (temp[i].first != temp[i - 1].first) {
          tOffsets[temp[i].first] = i;
        }
#elif WEIGHTED_TEMPORAL
        inEdges[4 * i] = get<1>(temp[i]);
        inEdges[4 * i + 1] = get<2>(temp[i]);
        inEdges[4 * i + 2] = get<3>(temp[i]);
        inEdges[4 * i + 3] = get<4>(temp[i]);
        if (get<0>(temp[i]) != get<0>(temp[i - 1])) {
          tOffsets[get<0>(temp[i])] = i;
        }
#else
        inEdges[i] = temp[i].second;
        if (temp[i].first != temp[i - 1].first) {
          tOffsets[temp[i].first] = i;
        }
#endif
      }
    }
    free(temp);
    // fill in offsets of degree 0 vertices by taking closest non-zero
    // offset to the right
    ligra::sequence::scanIBack(tOffsets, tOffsets, n, minF<uintT>(), (uintT)m);
    {
      parallel_for(long i = 0; i < n; i++) {
        uintT o = tOffsets[i];
        uintT l = ((i == n - 1) ? m : tOffsets[i + 1]) - tOffsets[i];
        v[i].setInDegree(l);
#ifdef WEIGHTED
        v[i].setInNeighbors((intE*)(inEdges + 2 * o));
#elif WEIGHTED_TEMPORAL
        v[i].setInNeighbors((intE*)(inEdges + 4 * o));
#else
        v[i].setInNeighbors((uintE*)inEdges + o);
#endif
      }
    }
    free(tOffsets);
#ifdef WEIGHTED
    Uncompressed_Mem<vertex>* mem =
        new Uncompressed_Mem<vertex>(v, n, m, edgesAndWeights, inEdges);
    return graph<vertex>(v, n, m, mem);
#elif WEIGHTED_TEMPORAL
    // Should be the exact same as WEIGHTED: the difference is in how we
    // initialize edgesAndWeights (multiples of 4 instead of 2).
    //
    // Reason why it's multiples of 4: each temporal edge has 2 additional
    // fields, which are "start time" and "end time".
    Uncompressed_Mem<vertex>* mem =
        new Uncompressed_Mem<vertex>(v, n, m, edgesAndWeights, inEdges);
    return graph<vertex>(v, n, m, mem);
#else
    Uncompressed_Mem<vertex>* mem =
        new Uncompressed_Mem<vertex>(v, n, m, edges, inEdges);
    return graph<vertex>(v, n, m, mem);
#endif
  }
  free(offsets);
#ifdef WEIGHTED
  Uncompressed_Mem<vertex>* mem =
      new Uncompressed_Mem<vertex>(v, n, m, edgesAndWeights);
  return graph<vertex>(v, n, m, mem);
#elif WEIGHTED_TEMPORAL
  // Should be the exact same as WEIGHTED: the difference is in how we
  // initialize edgesAndWeights (multiples of 4 instead of 2).
  //
  // Reason why it's multiples of 4: each temporal edge has 2 additional
  // fields, which are "start time" and "end time".
  Uncompressed_Mem<vertex>* mem =
      new Uncompressed_Mem<vertex>(v, n, m, edgesAndWeights);
  return graph<vertex>(v, n, m, mem);
#else
  Uncompressed_Mem<vertex>* mem = new Uncompressed_Mem<vertex>(v, n, m, edges);
  return graph<vertex>(v, n, m, mem);
#endif
}

template <class vertex>
graph<vertex> readGraph(char* iFile, bool compressed, bool symmetric,
                        bool binary, bool mmap) {
  if (binary)
    return readGraphFromBinary<vertex>(iFile, symmetric);
  else
    return readGraphFromFile<vertex>(iFile, symmetric, mmap);
}

template <class vertex>
graph<vertex> readCompressedGraph(char* fname, bool isSymmetric, bool mmap) {
  char* s;
  if (mmap) {
    _seq<char> S = mmapStringFromFile(fname);
    // Cannot mutate graph unless we copy.
    char* bytes = newA(char, S.n);
    parallel_for(size_t i = 0; i < S.n; i++) { bytes[i] = S.A[i]; }
    if (munmap(S.A, S.n) == -1) {
      perror("munmap");
      exit(-1);
    }
    s = bytes;
  } else {
    ifstream in(fname, ifstream::in | ios::binary);
    in.seekg(0, ios::end);
    long size = in.tellg();
    in.seekg(0);
    cout << "size = " << size << endl;
    s = (char*)malloc(size);
    in.read(s, size);
    in.close();
  }

  long* sizes = (long*)s;
  long n = sizes[0], m = sizes[1], totalSpace = sizes[2];

  cout << "n = " << n << " m = " << m << " totalSpace = " << totalSpace << endl;
  cout << "reading file..." << endl;

  uintT* offsets = (uintT*)(s + 3 * sizeof(long));
  long skip = 3 * sizeof(long) + (n + 1) * sizeof(intT);
  uintE* Degrees = (uintE*)(s + skip);
  skip += n * sizeof(intE);
  uchar* edges = (uchar*)(s + skip);

  uintT* inOffsets;
  uchar* inEdges;
  uintE* inDegrees;
  if (!isSymmetric) {
    skip += totalSpace;
    uchar* inData = (uchar*)(s + skip);
    sizes = (long*)inData;
    long inTotalSpace = sizes[0];
    cout << "inTotalSpace = " << inTotalSpace << endl;
    skip += sizeof(long);
    inOffsets = (uintT*)(s + skip);
    skip += (n + 1) * sizeof(uintT);
    inDegrees = (uintE*)(s + skip);
    skip += n * sizeof(uintE);
    inEdges = (uchar*)(s + skip);
  } else {
    inOffsets = offsets;
    inEdges = edges;
    inDegrees = Degrees;
  }

  vertex* V = newA(vertex, n);
  parallel_for(long i = 0; i < n; i++) {
    long o = offsets[i];
    uintT d = Degrees[i];
    V[i].setOutDegree(d);
    V[i].setOutNeighbors(edges + o);
  }

  if (sizeof(vertex) == sizeof(compressedAsymmetricVertex)) {
    parallel_for(long i = 0; i < n; i++) {
      long o = inOffsets[i];
      uintT d = inDegrees[i];
      V[i].setInDegree(d);
      V[i].setInNeighbors(inEdges + o);
    }
  }

  cout << "creating graph..." << endl;
  Compressed_Mem<vertex>* mem = new Compressed_Mem<vertex>(V, s);

  graph<vertex> G(V, n, m, mem);
  return G;
}
