// This code is part of the project "Parallel Local Clustering
// Algorithms".  Copyright (c) 2016 Julian Shun
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights (to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// This is a serial implementation of the heat kernel PageRank
// algorithm (rand-HK-PR) in "Computing Heat Kernel Pagerank and a
// Local Clustering Algorithm" by Fan Chung and Olivia Simpson, IWOCA
// 2014.  Currently only works with uncompressed graphs, and not with
// compressed graphs.
#include "ligra.h"
#include <unordered_map>
#include <unordered_set>
#include "sweep.h"
using namespace std;

//1/2 probability of staying, 1/2d probability of going to each neighbor
template <class vertex>
inline uintE walk(uintE x, vertex* V, uintE seed) {
  uintE d = V[x].getOutDegree();
  uintE randInt = hashInt(seed) % d;
  return V[x].getOutNeighbor(randInt);
}

typedef pair<uintE,double> pairIF;

timer t1;
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  long start = P.getOptionLongValue("-r",0);
  if(GA.V[start].getOutDegree() == 0) { 
    cout << "starting vertex has degree 0" << endl;
    return;
  }
  const uintE K = P.getOptionIntValue("-K",10);
  const uintE N = P.getOptionIntValue("-N",10);
  const double t = P.getOptionDoubleValue("-t",3);
  srand (time(NULL));
  uintE seed = rand();
  const intE n = GA.n;

  //walk length probabilities
  double* fact = newA(double,K);
  fact[0] = 1;
  for(long k=1;k<K;k++) fact[k] = k*fact[k-1];
  double* probs = newA(double,K);
  for(long k=0;k<K;k++) probs[k] = exp(-t)*pow(t,k)/fact[k];

  unordered_map<uintE,double> p;
  for(long i=0;i<N;i++) {
    double randDouble = (double) hashInt(seed++) / UINT_E_MAX;
    long j = 0;
    double mass = 0;
    uintE x = start;
    do {
      mass += probs[j];
      if(randDouble < mass) break;
      x = walk(x,GA.V,seed++);
      j++;
    } while(j <= K);
    p[x]++;
  }
  for(auto it=p.begin();it!=p.end();it++) {
    p[it->first] /= N;
  }

  free(probs); free(fact);
  t1.stop();
  pairIF* A = newA(pairIF,p.size());

  long numNonzerosQ = 0;
  for(auto it = p.begin(); it != p.end(); it++) {
    A[numNonzerosQ++] = make_pair(it->first,it->second);
  }
  sweepObject sweep = sweepCut(GA,A,numNonzerosQ,start);
  free(A);
  cout << "number of vertices touched = " << p.size() << endl;
  cout << "number of edges touched = " << sweep.vol << endl;
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl; 
  t1.reportTotal("computation time");
}
