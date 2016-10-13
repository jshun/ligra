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

// This file contains both the serial and the parallel implementations
// of sweep cut. Currently only works with uncompressed graphs, and
// not with compressed graphs.

// To print out the set found by the sweep cut, uncomment the
// following line (not to be used when measuring running time).
//#define PRINT_SET

#include "graph.h"
#include <unordered_map>
#include <unordered_set>
#include <math.h>
#include <float.h>
#include "sparseSet.h"
#include "sampleSort.h"
#include "blockRadixSort.h"
using namespace std;

typedef pair<double,double> pairDouble;
typedef pair<uintE,double> pairIF;

static double bestGlobalCond = DBL_MAX;
static long bestSize = -1;
static long bestStart = -1;

template <class vertex>
struct sweep_compare { 
  vertex* V;
sweep_compare(vertex* _V) : V(_V) {}
  bool operator () (pairIF a, pairIF b) {
    return a.second/V[a.first].getOutDegree() > b.second/V[b.first].getOutDegree(); }};

struct sweepObject {
  double conductance;
  long sizeS, volS, vol, edgesCrossing;
sweepObject(double _c, long _s, long _volS, long _vol, long _e) :
  conductance(_c), sizeS(_s), volS(_volS), vol(_vol), edgesCrossing(_e) {}
};

timer s1,s2;

template <class vertex, class fType>
  sweepObject sweepCut(graph<vertex>& GA, pair<uintE,fType> *p, long numNonzeros, long start) {  
  int procs = getWorkers();
  setWorkers(1);
  s1.start();
  sampleSort(p,(uintE)numNonzeros,sweep_compare<vertex>(GA.V));
  //find cluster with sweep cut
  unordered_set<uintE> S;
  long volS = 0;
  long edgesCrossing = 0;
  double bestConductance = DBL_MAX;
  long bestCut = -1;
  long bestEdgesCrossing = -1;
  long bestVol = -1;
  for(long i=0;i<numNonzeros;i++) {
    uintE v = p[i].first;
    S.insert(v);
    volS += GA.V[v].getOutDegree();
    long denom = min(volS,GA.m-volS);
    for(long j=0;j<GA.V[v].getOutDegree();j++) {
      uintE ngh = GA.V[v].getOutNeighbor(j);
      if(S.find(ngh) != S.end()) edgesCrossing--;
      else edgesCrossing++;
    }
    double conductance = (edgesCrossing == 0 || denom == 0) ? 1 : (double)edgesCrossing/denom;
    
    if(conductance < bestConductance) {
      bestConductance = conductance;
      bestCut = i;
      bestEdgesCrossing = edgesCrossing;
      bestVol = volS;
    }
  }
  if(bestConductance < bestGlobalCond) { 
    bestGlobalCond = bestConductance; bestSize = bestCut+1; bestStart = start; }
  //s1.reportTotal("Serial sweep time");
#ifdef PRINT_SET
  cout << "S = {";
  {for(long i=0;i<bestCut;i++) {
      cout << p[i].first << ", ";
    }
  }
  cout << p[bestCut].first << "}" << endl;
#endif
  setWorkers(procs);
  return sweepObject(bestConductance,bestCut+1,bestVol,volS,bestEdgesCrossing);
}

typedef pair<intT,intT> pairIntT;
typedef pair<double,intT> diPair;

struct edgeLessPair{
  bool operator() (pairIntT a, pairIntT b){
    return (a.second < b.second); }};

struct extractSecond {
  intT operator() (pairIntT a) { return a.second; }};

struct addFirst { pairIntT operator() (pairIntT a, pairIntT b){
  b.first=a.first+b.first;return b;
  }};

struct minDiPair { diPair operator() (diPair a, diPair b){
  return (a.first < b.first) ? a : b;
}};

template <class vertex, class fType>
  sweepObject parSweepCut(graph<vertex>& GA, pair<uintE,fType> *p, long numNonzeros, long start) {
  s2.start();
  sampleSort(p,(uintE)numNonzeros,sweep_compare<vertex>(GA.V));
  //store rank of each vertex in ordering. vertices not in p have highest rank
  sparseAdditiveSet<uintE> ranks = sparseAdditiveSet<uintE>(numNonzeros,2,numNonzeros); 
  //compute offsets into edge array
  uintT* Degrees = newA(uintT,numNonzeros+1);
  Degrees[numNonzeros]=0;
  {parallel_for(long i=0;i<numNonzeros;i++) {
      uintT v = p[i].first;
      Degrees[i] = GA.V[v].getOutDegree()*2;
      ranks.insert(make_pair(v,i));
    }}
  long totalDegree = sequence::plusScan(Degrees,Degrees,numNonzeros+1);
  //create (1,-1) if entry goes to higher ranked neighbor, else (0,0)
  pairIntT* edges = newA(pairIntT,totalDegree);
  {parallel_for(intT i=0;i<numNonzeros;i++){
    long o = Degrees[i];
    uintE v = p[i].first;
    if(GA.V[v].getOutDegree() > 1000) {
      parallel_for(long j=0;j<GA.V[v].getOutDegree();j++){
	long jj = j*2;
	uintE ngh = GA.V[v].getOutNeighbor(j);
	uintE rankNgh = ranks.find(ngh).second;
	if(rankNgh > i){
	  edges[o+jj].first = 1;
	  edges[o+jj+1].first = -1;
	} else {
	  edges[o+jj].first = 0;
	  edges[o+jj+1].first = 0;
	}
	edges[o+jj].second = i;
	edges[o+jj+1].second = rankNgh;
      }
    } else {
      for(long j=0;j<GA.V[v].getOutDegree();j++){
	long jj = j*2;
	uintE ngh = GA.V[v].getOutNeighbor(j);
	uintE rankNgh = ranks.find(ngh).second;
	if(ranks.find(ngh).second > i){
	  edges[o+jj].first = 1;
	  edges[o+jj+1].first = -1;
	} else {
	  edges[o+jj].first = 0;
	  edges[o+jj+1].first = 0;
	}
	edges[o+jj].second = i;
	edges[o+jj+1].second = rankNgh;
      }
    }}}
  //store edges by incident vertex id
  intSort::iSort(edges,totalDegree,numNonzeros+1,extractSecond());
  sequence::scanI(edges,edges,totalDegree,addFirst(),make_pair((intT)0,(intT)-1));
  diPair* cuts = newA(diPair,numNonzeros);
  {parallel_for(long i=0;i<totalDegree-1;i++) {
      if(edges[i].second != edges[i+1].second) {
	//cut point
	cuts[edges[i].second] = make_pair(edges[i].first,edges[i].second);
	//divide by denominator of conductance
	long denom = min(Degrees[edges[i].second+1]/2,(intT)GA.m-Degrees[edges[i].second+1]/2);
	if(denom == 0) cuts[edges[i].second].first = 1;
	else cuts[edges[i].second].first /= denom;
      }
    }
  }
  if(edges[totalDegree-1].second != numNonzeros) { //last edge is still in p
	long denom = min(totalDegree/2,(intT)GA.m-totalDegree/2);
	if(denom == 0) cuts[edges[totalDegree-1].second] = 
			 make_pair(1,edges[totalDegree-1].second);
	else cuts[edges[totalDegree-1].second] = 
	       make_pair(edges[totalDegree-1].first/denom,edges[totalDegree-1].second);
  }

  diPair best = sequence::reduce(cuts,numNonzeros,minDiPair());
  double bestConductance = best.first; uintT bestCut = best.second; uintT bestVol = Degrees[bestCut+1]/2; 
  uintT bestEdgesCrossing = best.first * min(Degrees[best.second+1]/2,(intT)GA.m-Degrees[best.second+1]/2);
  free(cuts); free(edges); free(Degrees);
  if(bestConductance < bestGlobalCond) { 
    bestGlobalCond = bestConductance; bestSize = bestCut+1; bestStart = start; }
#ifdef PRINT_SET
  cout << "S = {";
  {for(long i=0;i<bestCut;i++) {
      cout << p[i].first << ", ";
    }
  }
  cout << p[bestCut].first << "}" << endl;
#endif
  //s2.reportTotal("Parallel sweep time");
  return sweepObject(bestConductance,bestCut+1,bestVol,totalDegree/2,bestEdgesCrossing);
}
