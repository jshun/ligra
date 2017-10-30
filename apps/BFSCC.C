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

//This implementation uses BFS's to find connected components until
//all vertices have been visited. This implementation will work well
//for dense low-diameter graphs. Do not use it on very high-diameter
//graphs or graphs with very many components.
#include "ligra.h"

struct BFS_F {
  uintE* Parents;
  uintE label;
  BFS_F(uintE* _Parents, uintE _label) : Parents(_Parents), label(_label) {}
  inline bool update (uintE s, uintE d) { //Update
    if(Parents[d] == UINT_E_MAX) { Parents[d] = label; return 1; }
    else return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    return (CAS(&Parents[d],UINT_E_MAX,label));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); } 
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n;
  //creates Parents array, initialized to all -1, except for start
  uintE* Parents = newA(uintE,GA.n);
  parallel_for(long i=0;i<GA.n;i++) Parents[i] = UINT_E_MAX;
  long numVisited = 0;

  for(long i=0;i<n;i++) {
    uintE start = i;
    if(Parents[start] == UINT_E_MAX) {
      Parents[start] = start;
      vertexSubset Frontier(n,start); //creates initial frontier
      long round = 0;
      while(!Frontier.isEmpty()){ //loop until frontier is empty
	round++;
	numVisited+=Frontier.numNonzeros();
	//apply edgemap
	vertexSubset output = edgeMap(GA,Frontier,BFS_F(Parents,start));    
	Frontier.del();
	Frontier = output; //set new frontier
      } 
      Frontier.del();
      if(numVisited == n) break;
    }
  }
  free(Parents); 
}
