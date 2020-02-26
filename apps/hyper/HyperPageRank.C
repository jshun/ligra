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
#define HYPER 1
#include "hygra.h"
#include "math.h"

template <class vertex>
struct PR_Update_F {
  double* pSrc, *pDest;
  vertex* V;
  PR_Update_F(double* _pSrc, double* _pDest, vertex* _V) : 
    pSrc(_pSrc), pDest(_pDest), V(_V) {}
  inline bool update(uintE s, uintE d){ //update function applies PageRank equation
    pDest[d] += pSrc[s]/V[s].getOutDegree();
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    writeAdd(&pDest[d],pSrc[s]/V[s].getOutDegree());
    return 1;
  }
  inline bool cond (intT d) { return cond_true(d); }};

//vertex map function to update its p value according to PageRank equation
struct PR_Vertex_F {
  double damping;
  double addedConstant;
  double* pV;
  PR_Vertex_F(double* _pV, double _damping, intE n) :
    pV(_pV), damping(_damping), addedConstant((1-_damping)*(1/(double)n)){}
  inline bool operator () (uintE i) {
    pV[i] = damping*pV[i] + addedConstant;
    return 1;
  }
};

//resets p
struct PR_Reset {
  double* p;
  PR_Reset(double* _p) :
    p(_p) {}
  inline bool operator () (uintE i) {
    p[i] = 0.0;
    return 1;
  }
};

struct Entropy_F {
  double* EntropyH, *pH, *pV;
  Entropy_F(double* _EntropyH, double* _pH, double* _pV) : 
    EntropyH(_EntropyH), pH(_pH), pV(_pV) {}
  inline bool update(uintE s, uintE d){ //update function applies PageRank equation
    EntropyH[d] += (pV[s]/pH[d])*log2(pH[d]/pV[s]);
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    writeAdd(&EntropyH[d],(pV[s]/pH[d])*log2(pH[d]/pV[s]));
    return 1;
  }
  inline bool cond (intT d) { return cond_true(d); }};

//assumes connected graph, otherwise PageRank mass will be lost
//pass -entropy flag to compute entropy of hyperedges at the end
template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long maxIters = P.getOptionLongValue("-maxiters",1);
  bool entropy = P.getOptionValue("-entropy");
  const intE nv = GA.nv, nh = GA.nh;
  const double damping = 0.85, epsilon = 0.0000001;
  
  double one_over_nv = 1/(double)nv;
  double* pV = newA(double,nv);
  {parallel_for(long i=0;i<nv;i++) pV[i] = one_over_nv;}
  double* pH = newA(double,nh);
  //{parallel_for(long i=0;i<nh;i++) pH[i] = 0;} //0 if unchanged
  bool* frontierV = newA(bool,nv);
  {parallel_for(long i=0;i<nv;i++) frontierV[i] = 1;}
  bool* frontierH = newA(bool,nh);
  {parallel_for(long i=0;i<nh;i++) frontierH[i] = 1;}

  vertexSubset FrontierV(nv,nv,frontierV);
  hyperedgeSubset FrontierH(nh,nh,frontierH);  
  long iter = 0;
  while(iter++ < maxIters) {
    //cout << "V sum: " << sequence::plusReduce(pV,nv) << endl;
    hyperedgeMap(FrontierH,PR_Reset(pH));
    vertexProp(GA,FrontierV,PR_Update_F<vertex>(pV,pH,GA.V),0,no_output);
    vertexMap(FrontierV,PR_Reset(pV));
    //cout << "H sum: " << sequence::plusReduce(pH,nh) << endl;
    hyperedgeProp(GA,FrontierH,PR_Update_F<vertex>(pH,pV,GA.H),0,no_output);
    vertexMap(FrontierV,PR_Vertex_F(pV,damping,nv));
  }
  if(entropy) {
    double* EntropyH = newA(double,nh);
    {parallel_for(long i=0;i<nh;i++) EntropyH[i] = 0;}
    vertexProp(GA,FrontierV,Entropy_F(EntropyH,pH,pV),0,no_output);
    free(EntropyH);
  }
  FrontierV.del(); FrontierH.del();
  free(pH); free(pV); 
}
