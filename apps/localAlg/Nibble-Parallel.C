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

// This is a parallel implementation of the Nibble algorithm.
// Currently only works with uncompressed graphs, and not with
// compressed graphs.
#include "ligra.h"
#include "sparseSet.h"
#include "sweep.h"

template <class vertex>
struct Nibble_F {
  sparseAdditiveSet<float> p, new_p;
  vertex* V;
  Nibble_F(sparseAdditiveSet<float> &_p, sparseAdditiveSet<float> &_new_p, vertex* _V) : 
    p(_p), new_p(_new_p), V(_V) {}
  inline bool update(uintE s, uintE d){ //update function applies PageRank equation
    return new_p.insert(make_pair(d,p.find(s).second/(2*V[s].getOutDegree())));
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    return new_p.insert(make_pair(d,p.find(s).second/(2*V[s].getOutDegree())));
  }
  inline bool cond (intT d) { return cond_true(d); }}; 

//local update
struct Local_Update {
  sparseAdditiveSet<float> p, new_p;
  Local_Update(sparseAdditiveSet<float> &_p, sparseAdditiveSet<float> &_new_p) :
    p(_p), new_p(_new_p) {}
  inline bool operator () (uintE i) {
    new_p.insert(make_pair(i,p.find(i).second/2));
    return 1;
  }
};

typedef pair<uintE,float> ACLpair;

template <class vertex>
struct activeF {
  vertex* V;
  double epsilon;
  activeF(vertex* _V, double _epsilon) :
    V(_V), epsilon(_epsilon) {}
  inline bool operator () (ACLpair val) {
    return val.second >= V[val.first].getOutDegree()*epsilon;
  }
};

timer t1;

#define LOAD_FACTOR 1.1

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  long start = P.getOptionLongValue("-r",0);
  if(GA.V[start].getOutDegree() == 0) { 
    cout << "starting vertex has degree 0" << endl;
    return;
  }
  const long T = P.getOptionLongValue("-T",10);
  const double epsilon = P.getOptionDoubleValue("-e",0.000000001);
  const int procs = P.getOptionIntValue("-p",0);
  if(procs > 0) setWorkers(procs);
  const intE n = GA.n;

  sparseAdditiveSet<float> p = sparseAdditiveSet<float>(2,1,0.0);
  p.insert(make_pair(start,1.0));
  vertexSubset Frontier(n,start);

  long iter = 0, totalPushes = 0;
  while(Frontier.numNonzeros() > 0 && iter++ < T){
    totalPushes += Frontier.numNonzeros();
    uintT* Degrees = newA(uintT,Frontier.numNonzeros());
    {parallel_for(long i=0;i<Frontier.numNonzeros();i++) Degrees[i] = GA.V[Frontier.s[i]].getOutDegree();}
    long totalDegree = sequence::plusReduce(Degrees,Frontier.numNonzeros());
    free(Degrees);
    long pCount = p.count();
    sparseAdditiveSet<float> new_p = sparseAdditiveSet<float>(max(100L,min((long)n,totalDegree+pCount)),LOAD_FACTOR,0.0); //make bigger hash table
    vertexMap(Frontier,Local_Update(p,new_p));
    vertexSubset output = edgeMap(GA, Frontier, Nibble_F<vertex>(p,new_p,GA.V));
    p.del();
    p = new_p;
    output.del();
    //compute active set
    _seq<ACLpair> vals = p.entries(activeF<vertex>(GA.V,epsilon));
    uintE* Active = newA(uintE,vals.n);
    parallel_for(long i=0;i<vals.n;i++) Active[i] = vals.A[i].first;
    Frontier.del(); vals.del();
    Frontier = vertexSubset(n,vals.n,Active);
  }
  cout << "num iterations = " << iter-1 << " total pushes = " << totalPushes << endl;
  t1.reportTotal("computation time");

  _seq<ACLpair> A = p.entries();
  sweepObject sweep = parSweepCut(GA,A.A,A.n,start);
  cout << "vertices touched = " << A.n << endl;
  cout << "edges touched = " << sweep.vol << endl;
  A.del();
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl;
  Frontier.del(); p.del();
}
