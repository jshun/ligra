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

struct maxFirstF { intPair operator() (const intPair& a, const intPair& b) 
  const {return (a.first>b.first) ? a : b;}};

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

timer t0,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10;

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("connected components");
  t2.reportTotal("random sampling");
  t3.reportTotal("BFS from sample");
  t4.reportTotal("compute max distances");
  t5.reportTotal("find furthest vertex w");
  t6.reportTotal("BFS from w");
  t7.reportTotal("BFS from Ngh_S");
  t8.reportTotal("compute estimates");
  t9.reportTotal("main loop");
  t10.reportTotal("total time excluding writing to file");
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t10.start();
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  srand (time(NULL));
  uintT seed = rand();
  cout << "seed = " << seed << endl;
  t0.start();
  long n = GA.n;
  uintE* ecc = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) ecc[i] = UINT_E_MAX;}
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

  t0.start();
  t0.stop();

  uintE maxS = min((uintE)n,(uintE)sqrt(n*log2(n)));
  uintE maxSampleSize = max((uintE)10,min((uintE)n,max(maxS,(uintE)((n/maxS)*log2(n)))));
  //data structures to be shared by all components
  uintE** Dists = newA(uintE*,maxSampleSize);
  uintE* Dist = newA(uintE,maxSampleSize*n);
  {parallel_for(long i=0;i<maxSampleSize;i++) Dists[i] = Dist+i*n;}
  {parallel_for(long i=0;i<n*maxSampleSize;i++) Dist[i] = UINT_E_MAX;}

  uintE* wDist = newA(uintE,n); 
  {parallel_for(long i=0;i<n;i++)
      wDist[i] = UINT_E_MAX;}

  intPair* minDists = newA(intPair,n);
  uintE* starts = newA(uintE,n);
  uintE* starts2 = newA(uintE,n);
  uintE* maxEsts = newA(uintE,n);

  //BEGIN COMPUTE ECCENTRICITES PER COMPONENT
  t9.start();
  for(long k = 0; k < numCC; k++) {
    uintE o = CCoffsets[k];
    uintE CCsize = CCoffsets[k+1] - o;
    if(CCsize == 1) ecc[CCpairs[o].second] = 0; //singletons have ecc of 0
    if(CCsize == 2) { //size 2 CC's have ecc of 1
      ecc[CCpairs[o].second] = ecc[CCpairs[o+1].second] = 1;
    } else if(CCsize > 1) {
      t2.start();
      //do main computation
      uintE s = min(CCsize,(uintE)sqrt(CCsize*log2(CCsize)));
      //pick sample of about \sqrt{n\log n} vertices
      long sampleSize = min(CCsize,max((uintE)10,(uintE)((CCsize/s)*log2(CCsize))));

      {parallel_for(ulong i=0;i<CCsize;i++) {
	  //pick with probability sampleSize/CCsize
	  uintT index = hashInt(i+seed) % CCsize; 
	  if(index < sampleSize) starts[i] = CCpairs[o+i].second;
	  else starts[i] = UINT_E_MAX;
       	}}
      //pack down
      uintE numUnique = sequence::filter(starts,starts2,CCsize,nonMaxF());
      //sample cannot be empty!
      if(numUnique == 0) { starts2[0] = CCpairs[o+(hashInt(seed)%CCsize)].second; numUnique++; }
      if(numUnique > maxSampleSize) numUnique = maxSampleSize;
      t2.stop();
      t3.start();
      //execute BFS per sample
      {for(long i=0;i<numUnique;i++) {
	uintE v = starts2[i];
	Dists[i][v] = 0; //set source dist to 0
	vertexSubset Frontier(n,v);
	uintE round = 0;
	while(!Frontier.isEmpty()){
	  round++;
	  vertexSubset output = 
	    edgeMap(GA, Frontier, BFS_F(Dists[i],round),GA.m/20);
	  Frontier.del();
	  Frontier = output;
	}
	Frontier.del();
	ecc[v] = round-1; //set radius for sample vertex
	}}
      t3.stop();
      t4.start();
      //store max estimate from sample for each vertex so that we can
      //reuse Distance arrays
      {parallel_for(long i=0;i<CCsize;i++) {
	uintE v = CCpairs[o+i].second;
	//if not one of the vertices we did BFS on
	if(ecc[v] == UINT_E_MAX) {
	  uintE max_from_sample = 0;
	  //compute max estimate from sampled vertex
	  for(long j=0;j<numUnique;j++) {
	    uintE d = max(Dists[j][v],ecc[starts2[j]]-Dists[j][v]);
	    if(d > max_from_sample) max_from_sample = d;
	  }
	  maxEsts[i] = max_from_sample;
	}}}
      t4.stop();
      t5.start();
      //find furthest vertex from sample set S
      {parallel_for(long j=0;j<CCsize;j++) {
	uintE v = CCpairs[o+j].second;
	uintE m = UINT_E_MAX;
	for(long i=0;i<numUnique;i++) {
	  uintE d = Dists[i][v];
	  if(d < m) m = d;
	  if(d == 0) break;
	}
	minDists[j] = make_pair(m,v);
	}}

      intPair furthest = 
	sequence::reduce<intPair>(minDists,(intE)CCsize,maxFirstF());
      uintE w = furthest.second;
      t5.stop();
      t3.start();
      //reset Dist array entries
      {parallel_for(long i=0;i<numUnique;i++) {
	  parallel_for(long j=0;j<CCsize;j++) {
	    uintE v = CCpairs[o+j].second;
	    Dists[i][v] = UINT_E_MAX;
	  }
	}}
      t3.stop();
      t6.start();
      //execute BFS from w and find \sqrt{n log n} neighborhood of w
      uintE nghSize = min(CCsize,max((uintE)10,s));
      uintE* Ngh_s = starts; //reuse starts array
      bool filled_Ngh = 0;
      //stores distance from w and index of closest vertex in Ngh_s on
      //path from w to v
      wDist[w] = 0;
      vertexSubset Frontier(n,w);
      uintE round = 0;
      uintE numVisited = 0;
      while(!Frontier.isEmpty()){
	round++;
	if(!filled_Ngh) { 
	  Frontier.toSparse();
	  //Note: if frontier size < nghSize - visited, there is non-determinism in which vertices 
	  //get added to Ngh_s as the ordering of vertices on the frontier is non-deterministic
	  {parallel_for(long i=0;i<min(nghSize-numVisited,(uintE)Frontier.numNonzeros());i++) {
	    Ngh_s[numVisited+i] = Frontier.s[i];
	  }	   
	  numVisited += Frontier.numNonzeros();
	  if(numVisited >= nghSize) filled_Ngh = 1;
	  }}	
	vertexSubset output = 
	  edgeMap(GA, Frontier, BFS_F(wDist,round),GA.m/20);
	Frontier.del();
	Frontier = output;
      }
      Frontier.del();
      ecc[w] = round-1; //set radius for w
      t6.stop();
      t7.start();
      //execute BFS from each vertex in neighborhood of w
      uintE** Dists2 = Dists; //reuse distance array
      uintE* Dist2 = Dist;

      {for(long i=0;i<nghSize;i++) {
	uintE v = Ngh_s[i];
	Dists2[i][v] = 0; //set source dist to 0
	vertexSubset Frontier(n,v);
	uintE round = 0;
	while(!Frontier.isEmpty()){
	  round++;
	  vertexSubset output = 
	    edgeMap(GA, Frontier, BFS_F(Dists2[i],round),GA.m/20);
	  Frontier.del();
	  Frontier = output;
	}
	Frontier.del();
	ecc[v] = round-1; //set radius of vertex in Ngh_s
	}}
      t7.stop();
      t8.start();
      //compute ecc values
      {parallel_for(long i=0;i<CCsize;i++) {
	uintE v = CCpairs[o+i].second;
	//if not one of the vertices we did BFS on
	if(ecc[v] == UINT_E_MAX) {
	  uintE rv = max(maxEsts[i],max(wDist[v],ecc[w]-wDist[v])); //check w
	  //loop through Ngh_s
	  {for(long j=0;j<nghSize;j++) {
	    uintE estimate = max(Dists2[j][v],ecc[Ngh_s[j]]-Dists2[j][v]);
	    if(estimate > rv) rv = estimate;
	    }}
	  ecc[v] = rv;
	}
	}}
      t8.stop();
      t7.start();
      //reset Dist array entries
      {parallel_for(long i=0;i<nghSize;i++) {
	  parallel_for(long j=0;j<CCsize;j++) {
	    uintE v = CCpairs[o+j].second;
	    Dists2[i][v] = UINT_E_MAX;
	  }
	}}
      t7.stop();
      t6.start();
      //reset wDist array entries
      {parallel_for(long i=0;i<CCsize;i++) {
	  uintE v = CCpairs[o+i].second;
	  wDist[v] = UINT_E_MAX;
	}}
      t6.stop();
    }
  }
  t9.stop();
  //END COMPUTE ECCENTRICITES PER COMPONENT

  t0.start();
  free(minDists); free(starts); free(starts2); free(maxEsts);
  free(Dists); free(Dist); free(wDist);
  free(CCoffsets); free(CCpairs); 
  t0.stop(); t10.stop();
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
