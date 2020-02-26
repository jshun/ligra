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
#define WEIGHTED 1
#include "hygra.h"

struct BF_Relax_F {
  intE* ShortestPathLenSrc, *ShortestPathLenDest;
  int* Visited;
  BF_Relax_F(intE* _ShortestPathLenSrc, intE* _ShortestPathLenDest, int* _Visited) : 
    ShortestPathLenSrc(_ShortestPathLenSrc), ShortestPathLenDest(_ShortestPathLenDest), Visited(_Visited) {}
  inline bool update (uintE s, uintE d, intE edgeLen) { //Update ShortestPathLen if found a shorter path
    intE newDist = ShortestPathLenSrc[s] + edgeLen;
    if(ShortestPathLenDest[d] > newDist) {
      ShortestPathLenDest[d] = newDist;
      if(Visited[d] == 0) { Visited[d] = 1 ; return 1;}
    }
    return 0;
  }
  inline bool updateAtomic (uintE s, uintE d, intE edgeLen){ //atomic Update
    intE newDist = ShortestPathLenSrc[s] + edgeLen;
    return (writeMin(&ShortestPathLenDest[d],newDist) &&
	    CAS(&Visited[d],0,1));
  }
  inline bool cond (uintE d) { return cond_true(d); }
};

//reset visited elements
struct BF_Reset_F {
  int* Visited;
  BF_Reset_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator() (uintE i){
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long nv = GA.nv, nh = GA.nh;
  //initialize ShortestPathLen to "infinity"
  intE* ShortestPathLenV = newA(intE,nv);
  intE* ShortestPathLenH = newA(intE,nh);
  {parallel_for(long i=0;i<nv;i++) ShortestPathLenV[i] = INT_MAX/2;}
  {parallel_for(long i=0;i<nh;i++) ShortestPathLenH[i] = INT_MAX/2;}
  ShortestPathLenV[start] = 0;

  int* Visited = newA(int,max(nv,nh));
  {parallel_for(long i=0;i<max(nv,nh);i++) Visited[i] = 0;}

  vertexSubset Frontier(nv,start); //initial frontier

  long round = 0;
  while(1){
    if(round == nv-1) {
      //negative weight cycle
      {parallel_for(long i=0;i<nv;i++) ShortestPathLenV[i] = -(INT_E_MAX/2);}
      break;
    }
    //cout << Frontier.numNonzeros() << endl;
    hyperedgeSubset output = vertexProp(GA, Frontier, BF_Relax_F(ShortestPathLenV,ShortestPathLenH,Visited),-1,dense_forward);
    hyperedgeMap(output,BF_Reset_F(Visited));
    Frontier.del();
    Frontier = output;
    if(Frontier.isEmpty()) break;
    //cout << Frontier.numNonzeros() << endl;
    output = hyperedgeProp(GA, Frontier, BF_Relax_F(ShortestPathLenH,ShortestPathLenV,Visited),-1,dense_forward);
    vertexMap(output,BF_Reset_F(Visited));
    Frontier.del();
    Frontier = output;
    if(Frontier.isEmpty()) break;
    round++;
  }
  Frontier.del(); free(Visited);
  free(ShortestPathLenV); free(ShortestPathLenH);
}
