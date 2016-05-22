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

// This is a serial implementation of PR-Nibble using the optimized
// update rule and uses a priority queue.  Currently only works with
// uncompressed graphs, and not with compressed graphs.
#include "ligra.h"
#include <unordered_map>
#include <set>
#include "sweep.h"
using namespace std;

typedef pair<double,double> pairDouble;
typedef pair<uintE,double> pairIF;
struct pq_compare { bool operator () (pairIF a, pairIF b) {
    return a.second > b.second; }};

timer t1;
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  long start = P.getOptionLongValue("-r",0);
  if(GA.V[start].getOutDegree() == 0) { 
    cout << "starting vertex has degree 0" << endl;
    return;
  }
  const double alpha = P.getOptionDoubleValue("-a",0.15);
  const double epsilon = P.getOptionDoubleValue("-e",0.000000001);
  const intE n = GA.n;
  const double twoAlphaOverOnePlusAlpha = 2*alpha/(1+alpha);
  const double oneMinusAlphaOverOnePlusAlpha = (1-alpha)/(1+alpha);

  unordered_map <uintE,pairDouble> pr;
  pr[start] = make_pair(0.0,1.0); //starting vertex
  multiset<pairIF,pq_compare> q;
  q.insert(make_pair(start,1.0));
  long totalPushes = 0;
  while(!q.empty()) {
    totalPushes++;
    pairIF top = *q.begin();
    uintE v = top.first;
    q.erase(q.begin());
    pairDouble v_pr = pr[v];
    uintE d = GA.V[v].getOutDegree();
    v_pr.first += twoAlphaOverOnePlusAlpha*v_pr.second;
    double old_vr = v_pr.second;
    v_pr.second = 0;
    pr[v] = v_pr;
    for(long i=0;i<d;i++) {
      uintE ngh = GA.V[v].getOutNeighbor(i);
      pairDouble ngh_pr = pr[ngh]; //creates default entry if non-existent in table
      double oldRes = ngh_pr.second;
      ngh_pr.second += old_vr*oneMinusAlphaOverOnePlusAlpha/d;
      pr[ngh] = ngh_pr;
      uintE ngh_d = GA.V[ngh].getOutDegree();
      double epsd = epsilon*ngh_d;
      if(ngh_pr.second > epsd && oldRes < epsd) { 
	//if previous residual is small, that means it is not in q, so insert it
	q.insert(make_pair(ngh,ngh_pr.second/ngh_d));
      }
    }
  }
  t1.stop();
  pairIF* A = newA(pairIF,pr.size());

  long numNonzerosQ = 0;
  for(auto it = pr.begin(); it != pr.end(); it++) {
    if(it->second.first > 0) { 
      A[numNonzerosQ++] = make_pair(it->first,it->second.first);
    }
  }

  sweepObject sweep = parSweepCut(GA,A,numNonzerosQ,start);
  free(A);
  cout << "number of vertices touched = " << pr.size() << endl;
  cout << "number of edges touched = " << sweep.vol << endl;
  cout << "size of solution = " << numNonzerosQ << endl;
  cout << "total pushes = " << totalPushes << endl;
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl; 
  t1.reportTotal("computation time");
}
