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
#include "gettime.h"
#include "math.h"
#include "parseCommandLine.h"
using namespace std;

template <class vertex>
struct PR_F {
  vertex* V;
  double* Delta, *nghSum;
  PR_F(vertex* _V, double* _Delta, double* _nghSum) : 
    V(_V), Delta(_Delta), nghSum(_nghSum) {}
  inline bool update(intT s, intT d){
    nghSum[d] += Delta[s]/V[s].getOutDegree();
    return 1;
  }
  inline bool updateAtomic (intT s, intT d) {
    writeAdd(&nghSum[d],Delta[s]/V[s].getOutDegree());
    return 1;
  }
  inline bool cond (intT d) { return cond_true(d); }
};

struct PR_Vertex_F_FirstRound {
  double damping, addedConstant, one_over_n, epsilon2;
  double* p, *Delta, *nghSum;
  PR_Vertex_F_FirstRound(double* _p, double* _Delta, double* _nghSum, double _damping, double _one_over_n,double _epsilon2) :
    p(_p),
    damping(_damping), Delta(_Delta), nghSum(_nghSum), one_over_n(_one_over_n),
    addedConstant((1-_damping)*_one_over_n),
    epsilon2(_epsilon2) {}
  inline bool operator () (intT i) {
    Delta[i] = damping*(p[i]+nghSum[i])+addedConstant-p[i];
    p[i] += Delta[i];
    Delta[i]-=one_over_n; //subtract off delta from initialization
    return (fabs(Delta[i]) > epsilon2 * p[i]);
  }
};

struct PR_Vertex_F {
  double damping, epsilon2;
  double* p, *Delta, *nghSum;
  PR_Vertex_F(double* _p, double* _Delta, double* _nghSum, double _damping, double _epsilon2) :
    p(_p),
    damping(_damping), Delta(_Delta), nghSum(_nghSum), 
    epsilon2(_epsilon2) {}
  inline bool operator () (intT i) {
    Delta[i] = nghSum[i]*damping;
    p[i]+=Delta[i];
    return (fabs(Delta[i]) > epsilon2*p[i]);
  }
};

struct PR_Vertex_Reset {
  double* nghSum;
  PR_Vertex_Reset(double* _nghSum) :
    nghSum(_nghSum) {}
  inline bool operator () (intT i) {
    nghSum[i] = 0.0;
    return 1;
  }
};

template <class vertex>
void PageRankDelta(graph<vertex> GA) {
  const intT n = GA.n;
  const double damping = 0.85;
  const double epsilon = 0.0000001;
  const double epsilon2 = 0.01;

  double one_over_n = 1/(double)n;
  double* p = newA(double,n);
  {parallel_for(intT i=0;i<n;i++) p[i] = 0.0;}//one_over_n;
  double* Delta = newA(double,n);
  {parallel_for(intT i=0;i<n;i++) Delta[i] = one_over_n;} //initial delta propagation from each vertex
  double* nghSum = newA(double,n);
  {parallel_for(intT i=0;i<n;i++) nghSum[i] = 0.0;}
  bool* frontier = newA(bool,n);
  {parallel_for(intT i=0;i<n;i++) frontier[i] = 1;}

  vertices Frontier(n,n,frontier);
  bool* all = newA(bool,n);
  {parallel_for(intT i=0;i<n;i++) all[i] = 1;}

  vertices All(n,n,all);

  intT round = 0;

  while(1){
    round++;
    // cout<<"Round "<<round<<", frontier size = "<<Frontier.numNonzeros()<<endl;
    vertices output = edgeMap(GA, Frontier, PR_F<vertex>(GA.V,Delta,nghSum),GA.m/20,DENSE_FORWARD);
    output.del();

    vertices active 
      = (round == 1) ? 
      vertexFilter(All,PR_Vertex_F_FirstRound(p,Delta,nghSum,damping,one_over_n,epsilon2)) :
      vertexFilter(All,PR_Vertex_F(p,Delta,nghSum,damping,epsilon2));
    //compute L1-norm (use nghSum as temp array)
    {parallel_for(intT i=0;i<n;i++) {
      nghSum[i] = fabs(Delta[i]);
      }}
    double L1_norm = sequence::plusReduce(nghSum,n);
    //cout<<"L1 norm = "<<L1_norm<<endl;

    if(L1_norm < epsilon) break;
    //reset
    vertexMap(All,PR_Vertex_Reset(nghSum));
    Frontier.del();
    Frontier = active;
  }
  cout<<"Finished in "<<round<<" iterations\n";
  Frontier.del();
  free(p); free(Delta); free(nghSum);
  All.del();
}

int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  bool binary = P.getOptionValue("-b");
  long rounds = P.getOptionLongValue("-rounds",3);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric,binary); //symmetric graph
    PageRankDelta(G);
    for(int r=0;r<rounds;r++) {
      startTime();
      PageRankDelta(G);
      nextTime("PageRankDelta");
    }
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary); //asymmetric graph
    PageRankDelta(G);
    for(int r=0;r<rounds;r++) {
      startTime();
      PageRankDelta(G);
      nextTime("PageRankDelta");
    }
    G.del();
  }
}
