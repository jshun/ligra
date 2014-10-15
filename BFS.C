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
using namespace std;

struct BFS_F {
  intT* Parents;
  BFS_F(intT* _Parents) : Parents(_Parents) {}
  inline bool update (intT s, intT d) { //Update
    if(Parents[d] == -1) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool updateAtomic (intT s, intT d){ //atomic version of Update
    return (CAS(&Parents[d],(intT)-1,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (intT d) { return (Parents[d] == -1); } 
};

template <class vertex>
void Compute(intT start, graph<vertex> GA) {
  intT n = GA.n;
  //creates Parents array, initialized to all -1, except for start
  intT* Parents = newA(intT,GA.n);
  parallel_for(intT i=0;i<GA.n;i++) Parents[i] = -1;
  Parents[start] = start;

  vertexSubset Frontier(n,start); //creates initial frontier

  while(!Frontier.isEmpty()){ //loop until frontier is empty
    vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents),GA.m/20);    
    Frontier.del();
    Frontier = output; //set new frontier
  } 
  Frontier.del();
  free(Parents); 
}

// int parallel_main(int argc, char* argv[]) {  
//   commandLine P(argc,argv," [-s] <inFile>");
//   char* iFile = P.getArgument(0);
//   bool symmetric = P.getOptionValue("-s");
//   bool binary = P.getOptionValue("-b");
//   long start = P.getOptionLongValue("-r",0);
//   long rounds = P.getOptionLongValue("-rounds",3);
//   if(symmetric) {
//     graph<symmetricVertex> G = 
//       readGraph<symmetricVertex>(iFile,symmetric,binary); //symmetric graph
//     BFS((intT)start,G);
//     for(int r=0;r<rounds;r++) {
//       startTime();
//       BFS((intT)start,G);
//       nextTime("BFS");
//     }
//     G.del(); 
//   } else {
//     graph<asymmetricVertex> G = 
//       readGraph<asymmetricVertex>(iFile,symmetric,binary); //asymmetric graph
//     BFS((intT)start,G);
//     for(int r=0;r<rounds;r++) {
//       startTime();
//       BFS((intT)start,G);
//       nextTime("BFS");
//     }
//     G.del();
//   }
// }
