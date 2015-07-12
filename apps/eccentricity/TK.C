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
#include "ligra.h"
#include <sstream>
#include "blockRadixSort.h"
#include "CCBFS.h"
using namespace std;

timer t0,t1,t2,t3;

typedef pair<uintE,uintE> intPair;

template <class vertex>
struct getDegree {
  vertex* V;
  getDegree(vertex* VV) : V(VV) {}
  intE operator() (intE i) {return V[i].getOutDegree();}
};

struct min_lower {
  min_lower(uintE* _lower) : lower(_lower) {}
  uintE operator() (const uintE& a, const uintE& b) {
    return (lower[a] < lower[b]) ? a : b;}
  uintE* lower;
};

struct max_upper {
  max_upper(uintE* _upper) : upper(_upper) {}
  uintE operator() (const uintE& a, const uintE& b) {
    return (upper[a] > upper[b]) ? a : b;}
  uintE* upper;
};

struct BFS_F {
  uintE* Dist; uintE round;
  BFS_F(uintE* _Dist, uintE _round) : Dist(_Dist), round(_round) {}
  inline bool update(const uintE &s, const uintE& d) {
    if(Dist[d] == UINT_E_MAX) { Dist[d] = round; return 1; }
    else return 0; }
  inline bool updateAtomic(const uintE &s, const uintE &d) {
    return (CAS(&Dist[d],UINT_E_MAX,round)); }
  //cond function checks if vertex has been visited yet
  inline bool cond(const uintE &d) { return Dist[d] == UINT_E_MAX;}};

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("connected components");
  t2.reportTotal("main loop");
  t3.reportTotal("total time excluding writing to file");
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t3.start();
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  t0.start();
  long n = GA.n; 
  uintE* ecc = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) ecc[i] = 0;}
  t0.stop();

  //BEGIN COMPUTE CONNECTED COMPONENTS
  t1.start();  
  intE* Labels = newA(intE,n);
  {parallel_for(long i=0;i<n;i++) {
    if(GA.V[i].getOutDegree() == 0) Labels[i] = -i-1; //singletons
    else Labels[i] = INT_E_MAX;
    }}

  //get max degree vertex
  uintE maxV = sequence::reduce<uintE>((intE)0,(intE)n,maxF<intE>(),getDegree<vertex>(GA.V));

  //visit large component with BFS
  CCBFS(maxV,GA,Labels);
  //visit small components with label propagation
  Components(GA, Labels);

  //sort by component ID
  intPair* CCpairs = newA(intPair,n);
  {parallel_for(long i=0;i<n;i++)
    if(Labels[i] < 0)
      CCpairs[i] = make_pair(-Labels[i]-1,i);
    else CCpairs[i] = make_pair(Labels[i],i);
  }
  free(Labels);

  intSort::iSort(CCpairs, n, n+1, firstF<uintE,uintE>());

  uintE* changes = newA(uintE,n);
  changes[0] = 0;
  {parallel_for(long i=1;i<n;i++) 
      changes[i] = (CCpairs[i].first != CCpairs[i-1].first) ? i : UINT_E_MAX;}

  uintE* CCoffsets = newA(uintE,n);
  uintE numCC = sequence::filter(changes, CCoffsets, n, nonMaxF());
  CCoffsets[numCC] = n;
  free(changes);
  t1.stop();
  //END COMPUTE CONNECTED COMPONENTS

  //init data structures
  uintE* Dists = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) Dists[i] = UINT_E_MAX;}
  uintE* lower = newA(uintE,n);
  uintE* upper = newA(uintE,n);
  uintE* W = newA(uintE,n);
  uintE* W2 = newA(uintE,n);
  uintE totalIters = 0;
  //BEGIN COMPUTE ECCENTRICITES PER COMPONENT
  t2.start();
  for(long k = 0; k < numCC; k++) {
    uintE o = CCoffsets[k];
    uintE CCsize = CCoffsets[k+1] - o;
    if(CCsize == 2) { //size 2 CC's have ecc of 1
      ecc[CCpairs[o].second] = ecc[CCpairs[o+1].second] = 1;
    } else if(CCsize > 1) { //size 1 CC's already have ecc of 0
      //do main computation
      //init lower and upper bounds, and make active set of vertices
      {parallel_for(long i=0;i<CCsize;i++) {
	uintE v = CCpairs[o+i].second;
	lower[v] = 0; upper[v] = UINT_E_MAX;
	W[i] = v;
	}}
      uintE numIters = 0;
      uintE sizeW = CCsize;
      while(sizeW > 0) {
	numIters++;
	//pick BFS source w
	uintE w = (numIters % 2) ? 
	  sequence::reduce<uintE>(W,sizeW,max_upper(upper)) :
	  sequence::reduce<uintE>(W,sizeW,min_lower(lower));

	//execute BFS from w 
	Dists[w] = 0; //set source dist to 0
	vertexSubset Frontier(n,w);
	uintE round = 0;
	while(!Frontier.isEmpty()){
	  round++;
	  vertexSubset output = 
	    edgeMap(GA, Frontier, BFS_F(Dists,round),GA.m/20);
	  Frontier.del();
	  Frontier = output;
	}
	Frontier.del();
	ecc[w] = round-1; //set radius for sample vertex

	//update bounds for vertices in W
	parallel_for(long i=0;i<sizeW;i++) {
	  uintE v = W[i];
	  uintE lower_est = max(ecc[w] - Dists[v], Dists[v]);
	  if(lower_est > lower[v]) lower[v] = lower_est;
	  uintE upper_est = ecc[w] + Dists[v];
	  if(upper_est < upper[v]) upper[v] = upper_est;
	  if(lower[v] == upper[v]) { ecc[v] = lower[v]; W[i] = UINT_E_MAX; }
	}

	//filter out vertices with correct eccentricity
	sizeW = sequence::filter(W,W2,sizeW,nonMaxF());
	swap(W,W2);

	//reset distances
	{parallel_for(long j=0;j<CCsize;j++) {
	  uintE v = CCpairs[o+j].second;
	  Dists[v] = UINT_E_MAX;
	  }}
      }
      totalIters += numIters;
    }
  }
  t2.stop();
  //END COMPUTE ECCENTRICITES PER COMPONENT
  t0.start();
  free(lower); free(upper); free(Dists);
  free(CCoffsets); free(CCpairs);
  t0.stop(); t3.stop();
  reportAll();
  cout << "num iterations = " << totalIters << endl;
  if(oFile != NULL) {
    ofstream file (oFile, ios::out | ios::binary);
    stringstream ss;
    for(long i=0;i<GA.n;i++) ss << ecc[i] << endl;
    file << ss.str();
    file.close();
  }  
  free(ecc);
}
