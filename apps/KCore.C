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

// Implementation of K-Core decomposition of a graph. Currently works
// in Ligra, but not Ligra+.
#include "ligra.h"

template <class intT> 
struct nghInFrontier {
  bool* frontier;
  uintE* nghs;
  nghInFrontier(bool* _frontier, uintE* _nghs) : frontier(_frontier), nghs(_nghs) {}
  intT operator() (intT i) { return (intT) frontier[nghs[i]];}
};

template<class vertex>
struct KCore_Vertex_F {
  bool* frontier;
  vertex* V;
  long* coreNumbers;
  long k;
  KCore_Vertex_F(bool* _frontier, vertex* _V, long* _coreNumbers, long _k) : 
    frontier(_frontier), V(_V), k(_k), coreNumbers(_coreNumbers) {}
  inline bool operator () (uintE i) {
    // check if I has at least k neighbors that are in the current frontier
    uintE* nghs = V[i].getOutNeighbors();
    uintT outDeg = V[i].getOutDegree();
    long numIn = sequence::reduce<intT>((intT) 0, (intT) outDeg, addF<intT>(), 
                                        nghInFrontier<intT>(frontier, nghs));
    if (numIn < k) {
      coreNumbers[i] = k-1;
      frontier[i] = 0;
    }
    return (numIn >= k); 
  }
};

// 1) iterate over all remaining active vertices
// 2) for each active vertex, remove if induced degree < k. Any vertex removed has
//    core-number (k-1) (part of (k-1)-core, but not k-core)
// 3) stop once no vertices are removed. Vertices remaining are in the k-core.
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  const long n = GA.n;
  bool* active = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) active[i] = 1;}
  vertexSubset Frontier(n, n, active);
  long* coreNumbers = newA(long, n);
  {parallel_for(long i=0;i<n;i++) coreNumbers[i] = 0;}
  long numActive = n;
  long largestCore = -1;
  for (long k = 1; k <= n; k++) {
    long numRemoved = 0;
    while (true) {
      long prevActive = numActive;
      vertexMap(Frontier, KCore_Vertex_F<vertex>(active, GA.V, coreNumbers, k));
      numActive = sequence::sum(active,n);
      if (numActive == prevActive) { // fixed point. found k-core
        break;
      }
      numRemoved += (prevActive - numActive);
    }
    if (numActive == 0) {
      largestCore = k-1;
      break;
    }
  }
  cout << "largestCore was " << largestCore << endl;
  Frontier.del();
  free(coreNumbers);
}
