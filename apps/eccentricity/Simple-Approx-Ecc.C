// This code is part of the paper "An Evaluation of Parallel
// Eccentricity Estimation Algorithms on Undirected Real-World
// Graphs", presented at the ACM SIGKDD Conference on Knowledge
// Discovery and Data Mining, 2015.  
// Copyright (c) 2015 Julian Shun
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

timer t0,t1,t2,t3;

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("connected components");
  t2.reportTotal("main loop");
  t3.reportTotal("total time");
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t3.start();
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  t0.start();
  srand (time(NULL));
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


  //BEGIN COMPUTE ECCENTRICITES PER COMPONENT
  uintE* Dists = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) Dists[i] = UINT_E_MAX;}
  t2.start();
  for(long k = 0; k < numCC; k++) {
    uintE o = CCoffsets[k];
    uintE CCsize = CCoffsets[k+1] - o;
    if(CCsize == 2) { //size 2 CC's have ecc of 1
      ecc[CCpairs[o].second] = ecc[CCpairs[o+1].second] = 1;
    } else if(CCsize > 1) { //size 1 CC's already have ecc of 0
      //execute BFS from a random vertex w in component 
      uintE w = CCpairs[o+(rand()%CCsize)].second;
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
      round--;

      //update bounds for vertices in component
      parallel_for(long i=0;i<CCsize;i++) {
	uintE v = CCpairs[o+i].second;
	ecc[v] = round;
      }
    }
  }
  t2.stop();
  //END COMPUTE ECCENTRICITES PER COMPONENT
  t0.start();
  free(Dists);
  free(CCoffsets); free(CCpairs);
  t0.stop(); t3.stop();
  reportAll();
  if(oFile != NULL) {
    ofstream file (oFile, ios::out | ios::binary);
    stringstream ss;
    for(long i=0;i<GA.n;i++) ss << ecc[i] << endl;
    file << ss.str();
    file.close();
  }  
  free(ecc);
}
