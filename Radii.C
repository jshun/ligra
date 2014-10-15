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

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

struct Radii_F {
  intT round;
  intT* radii;
  long* Visited, *NextVisited;
  Radii_F(long* _Visited, long* _NextVisited, intT* _radii, intT _round) : 
    Visited(_Visited), NextVisited(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool update (intT s, intT d){ //Update function does a bitwise-or
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      NextVisited[d] |= toWrite;
      if(radii[d] != round) { radii[d] = round; return 1; }
    }
    return 0;
  }
  inline bool updateAtomic (intT s, intT d){ //atomic Update
    long toWrite = Visited[d] | Visited[s];
    if(Visited[d] != toWrite){
      writeOr(&NextVisited[d],toWrite);
      intT oldRadii = radii[d];
      if(radii[d] != round) return CAS(&radii[d],oldRadii,round);
    }
    return 0;
  }
  inline bool cond (intT d) { return cond_true(d); } //does nothing
};

//function passed to vertex map to sync NextVisited and Visited
struct Radii_Vertex_F {
  long* Visited, *NextVisited;
  Radii_Vertex_F(long* _Visited, long* _NextVisited) :
    Visited(_Visited), NextVisited(_NextVisited) {}
  inline bool operator() (intT i) {
    NextVisited[i] = Visited[i];
    return 1;
  }
};

template <class vertex>
void Compute(intT r, graph<vertex> GA) {
  intT n = GA.n;
  intT* radii = newA(intT,n);
  long* Visited = newA(long,n);
  long* NextVisited = newA(long,n);
  {parallel_for(intT i=0;i<n;i++) {
    radii[i] = -1;
    Visited[i] = NextVisited[i] = 0;
    }}
  intT sampleSize = min<intT>(n,64);
  intT* starts = newA(intT,sampleSize);
  
  {parallel_for(ulong i=0;i<sampleSize;i++) { //initial set of vertices
      uintE v = hash(i) % n;
    radii[v] = 0;
    starts[i] = v;
    Visited[v] = (long) 1<<i;
    }}

  vertexSubset Frontier(n,sampleSize,starts); //initial frontier of size 64

  intT round = 0;
  while(!Frontier.isEmpty()){
    round++;
    vertexMap(Frontier, Radii_Vertex_F(Visited,NextVisited));
    vertexSubset output = edgeMap(GA, Frontier, Radii_F(Visited,NextVisited,radii,round),GA.m/20);
    swap(NextVisited,Visited);
    Frontier.del();
    Frontier = output;
  }
  free(Visited); free(NextVisited);  
  Frontier.del();
  free(radii); 
}
