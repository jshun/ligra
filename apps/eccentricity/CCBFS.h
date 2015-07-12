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

//BFS portion
struct CCBFS_F {
  intE label;
  intE* Labels;
CCBFS_F(intE _label, intE* _Labels) : label(_label), Labels(_Labels) {}
  inline bool update(const intE &s, const intE& d) {
    if(Labels[d] == INT_E_MAX) { Labels[d] = label; return 1; }
    else return 0; }
  inline bool updateAtomic(const intE &s, const intE &d) {
    return (CAS(&Labels[d],INT_E_MAX,label)); }
  //cond function checks if vertex has been visited yet
  inline bool cond(const uintE &d) { return Labels[d] == INT_E_MAX;}};

template <class vertex>
uintE CCBFS(intE start, graph<vertex> GA, intE* Labels) {
  long n = GA.n;
  Labels[start] = start;
  vertexSubset Frontier(n,start); //creates initial frontier
  intE label = -start-1;
  long round = 0, numVisited = 0;
  while(!Frontier.isEmpty()){ //loop until frontier is empty
    round++;
    numVisited+=Frontier.numNonzeros();
    //apply edgemap
    vertexSubset output = edgeMap(GA, Frontier, CCBFS_F(label,Labels), GA.m/20);
    Frontier.del();
    Frontier = output; //set new frontier
  } 
  Frontier.del(); 
  return numVisited;
}

//CC portion
//update function writes min ID
struct CC_F {
  intE* IDs, *prevIDs;
  CC_F(intE* _IDs, intE* _prevIDs) : 
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool update(const uintE &s, const uintE &d){ 
    intE origID = IDs[d];
    if(IDs[s] < origID) { IDs[d] = min(origID,IDs[s]);
      if(origID == prevIDs[d]) return 1;
    } return 0; }
  inline bool updateAtomic(const uintE &s, const uintE &d){ 
    intE origID = IDs[d];
    return (writeMin(&IDs[d],IDs[s]) 
	    && origID == prevIDs[d]);}
//don't consider vertices visited by BFS (they have negative labels)
  inline bool cond(const uintE &d) { return prevIDs[d] >= 0;}};

//function used by vertex map to sync prevIDs with IDs
struct CC_Vertex_F {
  intE* IDs, *prevIDs;
CC_Vertex_F(intE* _IDs, intE* _prevIDs) :
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator () (const uintE &i) {
    prevIDs[i] = IDs[i];
    return 1; }};

template <class vertex>
void Components(graph<vertex> GA, intE* IDs) {
  long n = GA.n;
  intE* prevIDs = newA(intE,n);
  //initial frontier contains all unvisited vertices
  bool* frontier = newA(bool,n);
  parallel_for(long i=0;i<n;i++)
    if(IDs[i] == INT_E_MAX) {
      frontier[i] = 1;
      IDs[i] = prevIDs[i] = i; //label unvisited vertices with own ID
    } else frontier[i] = 0;
  
  vertexSubset Frontier(n,frontier);  
  long round = 0;
  while(!Frontier.isEmpty()){ //iterate until IDS converge
    round++;
    vertexMap(Frontier,CC_Vertex_F(IDs,prevIDs));
    vertexSubset output = 
      edgeMap(GA, Frontier, CC_F(IDs,prevIDs),
	      GA.m/20);
    Frontier.del();
    Frontier = output;
  }
  Frontier.del();  free(prevIDs);
}

