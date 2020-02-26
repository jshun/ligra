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

struct BFS_F {
  uintE* Parents;
  BFS_F(uintE* _Parents) : Parents(_Parents) {}
  inline bool update (uintE s, uintE d) { //Update
    if(Parents[d] == UINT_E_MAX) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    return (CAS(&Parents[d],UINT_E_MAX,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); } 
};

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long nv = GA.nv, nh = GA.nh;
  //creates Parents array, initialized to all -1, except for start
  uintE* ParentsV = newA(uintE,nv);
  uintE* ParentsH = newA(uintE,nh);
  {parallel_for(long i=0;i<nv;i++) ParentsV[i] = UINT_E_MAX;}
  {parallel_for(long i=0;i<nh;i++) ParentsH[i] = UINT_E_MAX;}
  ParentsV[start] = start;
  vertexSubset Frontier(nv,start); //creates initial frontier
  while(1){ //loop until frontier is empty
    //cout << Frontier.numNonzeros() << endl;
    hyperedgeSubset output = vertexProp(GA, Frontier, BFS_F(ParentsH));
    Frontier.del();
    Frontier = output; //set new frontier
    if(Frontier.isEmpty()) break;
    //cout << Frontier.numNonzeros() << endl;
    output = hyperedgeProp(GA, Frontier, BFS_F(ParentsV));
    Frontier.del();
    Frontier = output; //set new frontier
    if(Frontier.isEmpty()) break;
  } 
  Frontier.del();
  free(ParentsV); free(ParentsH); 
}
