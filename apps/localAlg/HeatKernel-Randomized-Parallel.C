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
// This is a parallel implementation of the heat kernel PageRank
// algorithm rand-HK-PR.  Currently only works with uncompressed
// graphs, and not with compressed graphs.
#include "ligra.h"
#include "sweep.h"
#include "sampleSort.h"
using namespace std;

typedef pair<uintE,double> ACLpair;

struct intLT { bool operator () (uintT a, uintT b) { return a < b; }; };

struct notMax { bool operator () (uintE a) { return a != UINT_E_MAX; }};

//1/2 probability of staying, 1/2d probability of going to each neighbor
template <class vertex>
inline uintE walk(uintE x, vertex* V, uintE seed) {
  uintE d = V[x].getOutDegree();
  uintE randInt = hashInt(seed) % d;
  return V[x].getOutNeighbor(randInt);
}

timer t1;
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  const int procs = P.getOptionIntValue("-p",0);
  if(procs > 0) setWorkers(procs);
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
  {parallel_for(long k=0;k<K;k++) probs[k] = exp(-t)*pow(t,k)/fact[k];}

  uintE* points = newA(uintE,N);
  {parallel_for(long i=0;i<N;i++) {
	double randDouble = (double) hashInt((uintE)(seed+2*i)) / UINT_E_MAX;
	long j = 0;
	double mass = 0;
	uintE x = start;
	do {
	  mass += probs[j];
	  if(randDouble < mass) break;
	  x = walk(x,GA.V,seed+2*i+1);
	  j++;
	} while(j <= K);
	points[i] = x;
    }}

  sampleSort(points,N,intLT()); 
  uintE* flags = newA(uintE,N);
  flags[0] = 0;
  {parallel_for(long i=1;i<N;i++) if(points[i] != points[i-1]) flags[i] = i; else flags[i] = UINT_E_MAX;}
  uintE* offsets = newA(uintE,N+1);
  uintE numUnique = sequence::filter(flags,offsets,N,notMax());
  offsets[numUnique] = N;
  ACLpair* A = newA(ACLpair,numUnique);
  {parallel_for(long i=0;i<numUnique;i++) {
      A[i] = make_pair(points[offsets[i]],(offsets[i+1]-offsets[i]) / (double)N);
    }}
  free(probs); free(fact); free(points);
  t1.stop();

  sweepObject sweep = parSweepCut(GA,A,numUnique,start);
  free(A);
  cout << "number of vertices touched = " << numUnique << endl;
  cout << "number of edges touched = " << sweep.vol << endl;
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl; 
  t1.reportTotal("computation time");
}
