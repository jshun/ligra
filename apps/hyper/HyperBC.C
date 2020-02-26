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
#include <vector>

typedef double fType;

struct BC_F {
  fType* NumPathsSrc, *NumPathsDest;
  bool* Visited;
  BC_F(fType* _NumPathsSrc, fType* _NumPathsDest, bool* _Visited) : 
    NumPathsSrc(_NumPathsSrc), NumPathsDest(_NumPathsDest), Visited(_Visited) {}
  inline bool update(uintE s, uintE d){ //Update function for forward phase
    fType oldV = NumPathsDest[d];
    NumPathsDest[d] += NumPathsSrc[s];
    return oldV == 0.0;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update, basically an add
    volatile fType oldV, newV; 
    do { 
      oldV = NumPathsDest[d]; newV = oldV + NumPathsSrc[s];
    } while(!CAS(&NumPathsDest[d],oldV,newV));
    return oldV == 0.0;
    //return xadd(&NumPathsDest[d],NumPathsSrc[s]) == 0;
  }
  inline bool cond (uintE d) { return Visited[d] == 0; } //check if visited
};

struct BC_Back_VtoH {
  fType* DependenciesV, *DependenciesH, *NumPathsV;
  bool* VisitedH;
  BC_Back_VtoH(fType* _DependenciesV, fType* _DependenciesH, fType * _NumPathsV, bool* _VisitedH) : 
    DependenciesV(_DependenciesV), DependenciesH(_DependenciesH), NumPathsV(_NumPathsV), VisitedH(_VisitedH) {}
  inline bool update(uintE s, uintE d){ //Update function for backwards phase
    DependenciesH[d] += DependenciesV[s]/NumPathsV[s];
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    writeAdd(&DependenciesH[d],DependenciesV[s]/NumPathsV[s]);
    return 1;	
  }
  inline bool cond (uintE d) { return VisitedH[d] == 0; } //check if visited
};

struct BC_Back_HtoV {
  fType* DependenciesV, *DependenciesH, *NumPathsV;
  bool* VisitedV;
  BC_Back_HtoV(fType* _DependenciesV, fType* _DependenciesH, fType * _NumPathsV, bool* _VisitedV) : 
    DependenciesV(_DependenciesV), DependenciesH(_DependenciesH), NumPathsV(_NumPathsV), VisitedV(_VisitedV) {}
  inline bool update(uintE s, uintE d){ //Update function for backwards phase
    DependenciesV[d] += DependenciesH[s]*NumPathsV[d];
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    writeAdd(&DependenciesV[d],DependenciesH[s]*NumPathsV[d]);
    return 1;
  }
  inline bool cond (uintE d) { return VisitedV[d] == 0; } //check if visited
};

//vertex map function to mark visited vertexSubset
struct BC_Vertex_F {
  bool* Visited;
  BC_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator() (uintE i) {
    Visited[i] = 1;
    return 1;
  }
};

//vertex map function (used on backwards phase) to mark visited vertexSubset
//and add to Dependencies score
struct BC_Back_Vertex_F {
  bool* Visited;
  fType* Dependencies;
  BC_Back_Vertex_F(bool* _Visited, fType* _Dependencies) : 
    Visited(_Visited), Dependencies(_Dependencies) {}
  inline bool operator() (uintE i) {
    Visited[i] = 1;
    Dependencies[i] += 1; //inverseNumPaths[i];
    return 1; }};

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long nv = GA.nv, nh = GA.nh;

  fType* NumPathsV = newA(fType,nv);
  {parallel_for(long i=0;i<nv;i++) NumPathsV[i] = 0.0;}
  fType* NumPathsH = newA(fType,nh);
  {parallel_for(long i=0;i<nh;i++) NumPathsH[i] = 0.0;}
  NumPathsV[start] = 1.0;

  bool* VisitedV = newA(bool,nv);
  {parallel_for(long i=0;i<nv;i++) VisitedV[i] = 0;}
  VisitedV[start] = 1;
  bool* VisitedH = newA(bool,nh);
  {parallel_for(long i=0;i<nh;i++) VisitedH[i] = 0;}
  vertexSubset Frontier(nv,start);
 
  vector<vertexSubset> Levels;
  Levels.push_back(Frontier);
  long round = 0;

  while(1){ //forward phase
    round++;
    hyperedgeSubset output = vertexProp(GA, Frontier, BC_F(NumPathsV,NumPathsH,VisitedH));
    hyperedgeMap(output, BC_Vertex_F(VisitedH)); //mark visited
    Levels.push_back(output); //save frontier onto Levels
    Frontier = output;
    if(Frontier.isEmpty()) break;
    //cout << round << " frontier size= " << Frontier.numNonzeros() << endl;
    round++;
    output = hyperedgeProp(GA, Frontier, BC_F(NumPathsH,NumPathsV,VisitedV));
    vertexMap(output, BC_Vertex_F(VisitedV)); //mark visited
    Levels.push_back(output); //save frontier onto Levels
    Frontier = output;
    if(Frontier.isEmpty()) break;
    //cout << round << " frontier size= " << Frontier.numNonzeros() << endl;
  }
  free(NumPathsH);
  
  //first phase ended on hyperedges
  if((round % 2) == 0) Levels[round--].del();
  Levels[round].del();
  
  fType* DependenciesV = newA(fType,nv);
  {parallel_for(long i=0;i<nv;i++) DependenciesV[i] = 0.0;}
  fType* DependenciesH = newA(fType,nh);
  {parallel_for(long i=0;i<nh;i++) DependenciesH[i] = 0.0;}

  //reuse Visited
  {parallel_for(long i=0;i<nv;i++) VisitedV[i] = 0;}
  {parallel_for(long i=0;i<nh;i++) VisitedH[i] = 0;}
  
  //tranpose graph
  GA.transpose();
  for(long r=round-1;r>0;r=r-2) { //backward phase
    Frontier = Levels[r];
    //cout << r << " " << Frontier.numNonzeros() << endl;
    //mark vertices as visited and add to dependency score
    vertexMap(Frontier,BC_Back_Vertex_F(VisitedV,DependenciesV)); 
    //vertex dependencies to hyperedges
    vertexProp(GA, Frontier, BC_Back_VtoH(DependenciesV,DependenciesH,NumPathsV,VisitedH), -1, no_output);
    Frontier.del();
    Frontier = Levels[r-1]; //gets frontier from Levels array
    hyperedgeMap(Frontier,BC_Vertex_F(VisitedH)); //mark hyperedges as visited
    //cout << r-1 << " " << Frontier.numNonzeros() << endl; 
    //hyperedge dependencies to vertices
    hyperedgeProp(GA, Frontier, BC_Back_HtoV(DependenciesV,DependenciesH,NumPathsV,VisitedV), -1, no_output);
    Frontier.del();
  }
  Levels[0].del(); //first frontier
  free(NumPathsV);
  free(VisitedV); free(VisitedH);
  free(DependenciesV); free(DependenciesH);
}
