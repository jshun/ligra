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

// Implementation of the shortcutting label propagation algorithm from
// the paper "Shortcutting Label Propagation for Distributed Connected
// Components", WSDM 2018.
#include "ligra.h"

struct CC_Shortcut {
  uintE* IDs, *prevIDs;
  CC_Shortcut(uintE* _IDs, uintE* _prevIDs) :
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator () (uintE i) {
    uintE l = IDs[IDs[i]];
    if(IDs[i] != l) IDs[i] = l;
    if(prevIDs[i] != IDs[i]) {
      prevIDs[i] = IDs[i];
      return 1; }
    else return 0;
  }
};
  
struct CC_F {
  uintE* IDs, *prevIDs;
  CC_F(uintE* _IDs, uintE* _prevIDs) : 
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool update(uintE s, uintE d){ //Update function writes min ID
    uintE origID = IDs[d];
    if(IDs[s] < origID) {
      IDs[d] = min(origID,IDs[s]);
    } return 1; }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    uintE origID = IDs[d];
    writeMin(&IDs[d],IDs[s]);
    return 1;
  }
  inline bool cond (uintE d) { return cond_true(d); } //does nothing
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n, m = GA.m;
  uintE* IDs = newA(uintE,n), *prevIDs = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) {prevIDs[i] = i; IDs[i] = i;}} //initialize unique IDs

  bool* all = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) all[i] = 1;} 
  vertexSubset All(n,n,all); //all vertices
  bool* active = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) active[i] = 1;} 
  vertexSubset Active(n,n,active); //initial frontier contains all vertices

  while(!Active.isEmpty()){ //iterate until IDS converge
    edgeMap(GA, Active, CC_F(IDs,prevIDs),m/20,no_output);
    vertexSubset output = vertexFilter(All,CC_Shortcut(IDs,prevIDs));
    Active.del();
    Active = output;
  }
  Active.del(); All.del(); free(IDs); free(prevIDs);
}
