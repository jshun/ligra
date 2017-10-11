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

// This is a parallel implementation of the HeatKernel PageRank
// algorithm (HK-PR).  Currently only works with uncompressed graphs,
// and not with compressed graphs.
#include "ligra.h"
#include "sparseSet.h"
#include "sweep.h"

template <class vertex>
struct HK_F {
  sparseAdditiveSet<float> x, r, new_r;
  vertex* V;
  double toverjplus1;
  HK_F(sparseAdditiveSet<float> &_x, sparseAdditiveSet<float> &_r, sparseAdditiveSet<float> &_new_r, vertex* _V, double _toverjplus1) : 
    x(_x), r(_r), new_r(_new_r), V(_V), toverjplus1(_toverjplus1) {}
  inline bool update(uintE s, uintE d){
    double mass = toverjplus1*r.find(s).second/V[s].getOutDegree();
    return new_r.insert(make_pair(d,mass));
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    double mass = toverjplus1*r.find(s).second/V[s].getOutDegree();
    return new_r.insert(make_pair(d,mass));
  }
  inline bool cond (intT d) { return cond_true(d); }}; 

template <class vertex>
struct HK_Last_F {
  sparseAdditiveSet<float> x, r;
  vertex* V;
  HK_Last_F(sparseAdditiveSet<float> &_x, sparseAdditiveSet<float> &_r, vertex* _V) : 
    x(_x), r(_r), V(_V) {}
  inline bool update(uintE s, uintE d){
    x.insert(make_pair(d,r.find(s).second/V[s].getOutDegree()));
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    x.insert(make_pair(d,r.find(s).second/V[s].getOutDegree()));
    return 1;
  }
  inline bool cond (intT d) { return cond_true(d); }}; 

//local update
struct Local_Update {
  sparseAdditiveSet<float> x, r;
  Local_Update(sparseAdditiveSet<float> &_x, sparseAdditiveSet<float> &_r) :
    x(_x), r(_r) {}
  inline bool operator () (uintE i) {
    x.insert(make_pair(i,r.find(i).second));
    return 1;
  }
};

struct intLT { bool operator () (uintT a, uintT b) { return a < b; }; };

typedef pair<uintE,float> ACLpair;

template <class vertex>
struct activeF {
  vertex* V;
  double constantOverPsi;
  activeF(vertex* _V, double _constantOverPsi) :
    V(_V), constantOverPsi(_constantOverPsi) {}
  inline bool operator () (ACLpair val) {
    return val.second >= V[val.first].getOutDegree()*constantOverPsi;
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
  const int procs = P.getOptionIntValue("-p",0);
  if(procs > 0) setWorkers(procs);
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
  {parallel_for(long m=0;m<N;m++) tm[m]  = pow(t,m);}
  {parallel_for(long k=0;k<N;k++) {
    psis[k] = 0;
    for(long m=0;m<N-k;m++)
      psis[k] += fact[k]*tm[m]/(double)fact[m+k];
    }}

  sparseAdditiveSet<float> x = sparseAdditiveSet<float>(10000,1,0.0);
  sparseAdditiveSet<float> r = sparseAdditiveSet<float>(2,1,0.0);
  x.insert(make_pair(start,0.0));
  r.insert(make_pair(start,1.0));
  vertexSubset Frontier(n,start);

  long j = 0, totalPushes = 0;
  while(Frontier.numNonzeros() > 0){
    totalPushes += Frontier.numNonzeros();
    uintT* Degrees = newA(uintT,Frontier.numNonzeros());
    {parallel_for(long i=0;i<Frontier.numNonzeros();i++) Degrees[i] = GA.V[Frontier.s[i]].getOutDegree();}
    long totalDegree = sequence::plusReduce(Degrees,Frontier.numNonzeros());
    free(Degrees);
    if(j+1 < N) {
      long rCount = r.count();
      //make bigger hash table initialized to 0.0's
      sparseAdditiveSet<float> new_r = sparseAdditiveSet<float>(max(100L,min((long)n,totalDegree+rCount)),LOAD_FACTOR,0.0); 
      vertexMap(Frontier,Local_Update(x,r));
      vertexSubset output = edgeMap(GA, Frontier, HK_F<vertex>(x,r,new_r,GA.V,t/(double)(j+1)));
      r.del(); 
      r = new_r;
      if(x.m < ((uintT) 1 << log2RoundUp((uintT)(LOAD_FACTOR*min((long)n,rCount+output.numNonzeros()))))) {
	sparseAdditiveSet<float> new_x = sparseAdditiveSet<float>(LOAD_FACTOR*min((long)n,rCount+output.numNonzeros()),LOAD_FACTOR,0.0); //make bigger hash table
	new_x.copy(x);
	x.del();
	x = new_x;
      }
      output.del();

      //compute active set (faster in practice to just scan over r)
      _seq<ACLpair> vals = r.entries(activeF<vertex>(GA.V,constant/psis[j+1]));
      uintE* Active = newA(uintE,vals.n);
      parallel_for(long i=0;i<vals.n;i++) Active[i] = vals.A[i].first;
      Frontier.del(); vals.del();
      Frontier = vertexSubset(n,vals.n,Active);
      j++;
    } else { //last iteration
      long rCount = r.count();
      if(x.m < ((uintT) 1 << log2RoundUp((uintT)(LOAD_FACTOR*min((long)n,rCount+totalDegree))))) {
	sparseAdditiveSet<float> new_x = sparseAdditiveSet<float>(min((long)n,rCount+totalDegree),LOAD_FACTOR,0.0); //make bigger hash table
	new_x.copy(x);
	x.del();
	x = new_x;
      }
      vertexMap(Frontier,Local_Update(x,r));
      edgeMap(GA, Frontier, HK_Last_F<vertex>(x,r,GA.V), -1, no_output);
      Frontier.del();
      j++;
      break;
    }
  }
  free(psis); free(fact); free(tm);
  cout << "num iterations = " << j << " total pushes = " << totalPushes << endl;
  t1.stop();

  if(1) {
    _seq<ACLpair> A = x.entries();
    sweepObject sweep = parSweepCut(GA,A.A,A.n,start);
    cout << "vertices touched = " << A.n << endl;
    cout << "edges touched = " << sweep.vol << endl;
    cout << "conductance = " << sweep.conductance << " |S| = " << sweep.sizeS << " vol(S) = " << sweep.volS << " edgesCrossing = " << sweep.edgesCrossing << endl;
    A.del();
  }
  x.del(); r.del();
  t1.reportTotal("computation time");
}
