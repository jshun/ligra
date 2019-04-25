// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
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

// Adds a random integer weight in [1...log(n)] to each edge
#include <math.h>
#include <assert.h>
#include "graphIO.h"
#include "parseCommandLine.h"
#include "parallel.h"
using namespace benchIO;
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<inFile> <outFile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;

  graph<uintT> G = readGraphFromFile<uintT>(iFile);

  long m = G.m;
  long n = G.n;

  size_t* offs = newA(size_t, n);
  parallel_for(size_t i=0; i<n; i++) {
    offs[i] = G.V[i].degree;
  }
  size_t mm = sequence::scan(offs, offs, n,addF<size_t>(),(size_t) 0);

  assert(mm == m);
  cout << "n = " << n << " origm = " << m << endl;

  using int_edge = edge<uintT>;
  edge<uintT>* edges = newA(int_edge, mm);
  parallel_for(size_t i=0; i<n; i++) {
    size_t offset = offs[i];
    for (size_t j=0; j<G.V[i].degree; j++) {
      edges[offset + j] = edge<uintT>(i, G.V[i].Neighbors[j]);
      uintT u = edges[i].u;
      uintT v = edges[i].v;
    }
  }
  edgeArray<uintT> E = edgeArray<uintT>(edges, n, n, m);

  std::cout << "generated edge array" << std::endl;
  writeGraphToFile<uintT>(graphFromEdges(E, /* symmetrize = */ true),oFile);

  return 1;
}
