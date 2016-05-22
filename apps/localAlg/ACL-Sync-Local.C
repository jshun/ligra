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

// This is a parallel implementation of PR-Nibble using the original
// update rule.  Currently only works with uncompressed graphs, and
// not with compressed graphs.
#include "ligra.h"
#include "sparseSet.h"
#include "sweep.h"

template <class vertex>
struct ACL_F {
  sparseAdditiveSet<float> p, r, new_r;
  vertex* V;
  double oneMinusAlphaOverTwo;
  ACL_F(sparseAdditiveSet<float> &_p, sparseAdditiveSet<float> &_r, sparseAdditiveSet<float> &_new_r, vertex* _V, double _oneMinusAlphaOverTwo) : 
    p(_p), r(_r), new_r(_new_r), V(_V), oneMinusAlphaOverTwo(_oneMinusAlphaOverTwo) {}
  inline bool update(uintE s, uintE d){ //update function applies PageRank equation
    return new_r.insert(make_pair(d,oneMinusAlphaOverTwo*r.find(s).second/V[s].getOutDegree()));
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    return new_r.insert(make_pair(d,oneMinusAlphaOverTwo*r.find(s).second/V[s].getOutDegree()));
  }
  inline bool cond (intT d) { return cond_true(d); }}; 

//local update
struct Local_Update {
  sparseAdditiveSet<float> p, r;
  double alpha, toSubtract;
  Local_Update(sparseAdditiveSet<float> &_p, sparseAdditiveSet<float> &_r, double _alpha) :
    p(_p), r(_r), alpha(_alpha), toSubtract((1+_alpha)/2) {}
  inline bool operator () (uintE i) {
    p.insert(make_pair(i,alpha*r.find(i).second));
    r.insert(make_pair(i,-toSubtract*r.find(i).second));
    return 1;
  }
};

struct intLT { bool operator () (uintT a, uintT b) { return a < b; }; };

typedef pair<uintE,float> ACLpair;

template <class vertex>
struct activeF {
  vertex* V;
  double epsilon;
  activeF(vertex* _V, double _epsilon) :
    V(_V), epsilon(_epsilon) {}
  inline bool operator () (ACLpair val) {
    return val.second > V[val.first].getOutDegree()*epsilon;
  }
};

timer t1;

#define LOAD_FACTOR 1.1

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t1.start();
  long maxIters = P.getOptionLongValue("-maxiters",10000);
  long start = P.getOptionLongValue("-r",0);
  if(GA.V[start].getOutDegree() == 0) { 
    cout << "starting vertex has degree 0" << endl;
    return;
  }
  const double alpha = P.getOptionDoubleValue("-a",0.15);
  const double epsilon = P.getOptionDoubleValue("-e",0.000000001);
  const int procs = P.getOptionIntValue("-p",0);
  if(procs > 0) setWorkers(procs);
  const intE n = GA.n;
  const double oneMinusAlphaOverTwo = (1-alpha)/2;

  sparseAdditiveSet<float> p = sparseAdditiveSet<float>(10000,1,0.0);
  sparseAdditiveSet<float> r = sparseAdditiveSet<float>(2,1,0.0);
  p.insert(make_pair(start,0.0));
  r.insert(make_pair(start,1.0));
  vertexSubset Frontier(n,start);

  long iter = 0, totalPushes = 0;
  while(Frontier.numNonzeros() > 0 && iter++ < maxIters){
    totalPushes += Frontier.numNonzeros();
    uintT* Degrees = newA(uintT,Frontier.numNonzeros());
    {parallel_for(long i=0;i<Frontier.numNonzeros();i++) Degrees[i] = GA.V[Frontier.s[i]].getOutDegree();}
    long totalDegree = sequence::plusReduce(Degrees,Frontier.numNonzeros());
    free(Degrees);
    long rCount = r.count();
    sparseAdditiveSet<float> new_r = sparseAdditiveSet<float>(max(100L,min((long)n,totalDegree+rCount)),LOAD_FACTOR,0.0); //make bigger hash table
    new_r.copy(r);
    vertexMap(Frontier,Local_Update(p,new_r,alpha));
    vertexSubset output = edgeMap(GA, Frontier, ACL_F<vertex>(p,r,new_r,GA.V,oneMinusAlphaOverTwo));
    r.del(); 
    r = new_r;
    if(p.m < ((uintT) 1 << log2RoundUp((uintT)(LOAD_FACTOR*min((long)n,rCount+output.numNonzeros()))))) {
      sparseAdditiveSet<float> new_p = sparseAdditiveSet<float>(min((long)n,rCount+output.numNonzeros()),LOAD_FACTOR,0.0); //make bigger hash table
      new_p.copy(p);
      p.del();
      p = new_p;
    }
    output.del();
    //compute active set (faster to scan over all of r)
    _seq<ACLpair> vals = r.entries(activeF<vertex>(GA.V,epsilon));
    uintE* Active = newA(uintE,vals.n);
    parallel_for(long i=0;i<vals.n;i++) Active[i] = vals.A[i].first;
    Frontier.del(); vals.del();
    Frontier = vertexSubset(n,vals.n,Active);
  }
  cout << "num iterations = " << iter << " total pushes = " << totalPushes << endl;
  t1.reportTotal("computation time");
  
  _seq<ACLpair> A = p.entries();
  sweepObject sweep = parSweepCut(GA,A.A,A.n,start);
  cout << "vertices touched = " << A.n << endl;
  cout << "edges touched = " << sweep.vol << endl;
  A.del();
  cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl;
  Frontier.del(); p.del(); r.del();
}
