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

// This is a serial implementation of the HeatKernel PageRank
// algorithm (HK-PR) in "Heat Kernel Based Community Detection" by
// Kyle Kloster and David Gleich, KDD 2014.  Currently only works with
// uncompressed graphs, and not with compressed graphs.
#include "ligra.h"
#include <unordered_map>
#include <queue>
#include "sweep.h"
using namespace std;

typedef pair<double,double> pairDouble;
typedef pair<uintE,double> pairIF;
typedef pair<uintE,uintE> pairInt;

class hashPair {
public:
  size_t operator () (const intPair &x) const {
    return hashInt(x.first)*31+hashInt(x.second);
  }};

class equalPair {
public:
  bool operator () (const intPair &a, const intPair &b) const {
    return a.first == b.first && a.second == b.second;
  }};

timer t1;
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  long start = P.getOptionLongValue("-r",0);
  if(GA.V[start].getOutDegree() == 0) { 
    cout << "starting vertex has degree 0" << endl;
    return;
  }
  const double t = P.getOptionDoubleValue("-t",3);
  const double epsilon = P.getOptionDoubleValue("-e",0.000000001);
  const uintE N = P.getOptionIntValue("-N",1);
  const intE n = GA.n;
  const double constant = exp(t)*epsilon/(2*(double)N);
  double* psis = newA(double,N);
  double* fact = newA(double,N);
  fact[0] = 1;
  for(long k=1;k<N;k++) fact[k] = k*fact[k-1];
  double* tm = newA(double,N);
  for(long m=0;m<N;m++) tm[m]  = pow(t,m);
  for(long k=0;k<N;k++) {
    psis[k] = 0;
    for(long m=0;m<N-k;m++)
      psis[k] += fact[k]*tm[m]/fact[m+k];
  }

  unordered_map <uintE,double> x;
  unordered_map <pairInt,double,hashPair,equalPair> r;
  r[make_pair(start,0)] = 1.0; //starting vertex
  queue<pairInt> q;
  q.push(make_pair(start,0));
  long totalPushes = 0;
  while(!q.empty()) {
    pairInt vj = q.front();
    uintE v = vj.first, j = vj.second;
    q.pop();
    double rvj = r[vj];
    x[v] += rvj;
    //r[vj] = 0;
    uintE d = GA.V[v].getOutDegree();
    double mass = t*rvj/((double)(j+1)*d);
    totalPushes++;
    for(long i=0;i<d;i++) {
      uintE ngh = GA.V[v].getOutNeighbor(i);
      if(j+1 == N) { x[ngh] += rvj/d; continue; }
      pairInt next = make_pair(ngh,j+1);
      double thresh = GA.V[ngh].getOutDegree()*constant/psis[j+1];
      if(r[next] < thresh && r[next] + mass >= thresh)
	q.push(next);
      r[next] += mass;
    }
  }
  free(psis); free(fact); free(tm);
  t1.stop();
  pairIF* A = newA(pairIF,x.size());

  long numNonzerosQ = 0;
  for(auto it = x.begin(); it != x.end(); it++) {
    A[numNonzerosQ++] = make_pair(it->first,it->second);
  }
  sweepObject sweep = sweepCut(GA,A,numNonzerosQ,start);
  free(A);
  cout << "number of vertices touched = " << x.size() << endl;
  cout << "number of edges touched = " << sweep.vol << endl;
  cout << "total pushes = " << totalPushes << endl;
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl; 
  t1.reportTotal("computation time");
}
