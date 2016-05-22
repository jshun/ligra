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

// This is a serial implementation of the Nibble algorithm in "A Local
// Clustering Algorithm for Massive Graphs and its Application to
// Nearly Linear Time Graph Partitioning" by Daniel Spielman and
// Shang-Hua Teng, Siam J. Comput. 2013.  Currently only works with
// uncompressed graphs, and not with compressed graphs.
#include "ligra.h"
#include <unordered_map>
#include "sweep.h"
using namespace std;

typedef pair<double,double> pairDouble;
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
  const double epsilon = P.getOptionDoubleValue("-e",0.000000001);
  const long T = P.getOptionLongValue("-T",10);
  const intE n = GA.n;

  unordered_map <uintE,double> q;
  q[start] = 1.0; //starting vertex
  long totalPushes = 0;

  for(long i=0;i<T;i++){
    unordered_map <uintE,double> q_next;
    for(auto it=q.begin();it!=q.end();it++) {
      uintE v = it->first;
      double q = it->second;
      uintE d = GA.V[v].getOutDegree();
      if(q > d*epsilon) {
	totalPushes++;
	q_next[v] += q/2;
	double nghMass = q/(2*d);
	for(long j=0;j<d;j++) {
	  uintE ngh = GA.V[v].getOutNeighbor(j);
	  q_next[ngh] += nghMass;
	}
      }
    }
    if(q_next.size() == 0) break;
    q = q_next;
  }

  t1.stop();
  pairIF* A = newA(pairIF,q.size());

  long numNonzerosQ = 0;
  for(auto it = q.begin(); it != q.end(); it++) {
    A[numNonzerosQ++] = make_pair(it->first,it->second);
  }

  sweepObject sweep = sweepCut(GA,A,numNonzerosQ,start);
  free(A);
  cout << "number of vertices touched = " << q.size() << endl;
  cout << "number of edges touched = " << sweep.vol << endl;
  cout << "total pushes = " << totalPushes << endl;
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl; 
  t1.reportTotal("computation time");
}
