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
#include <vector>
#include "math.h"
#include "parallel.h"
#include "parseCommandLine.h"
using namespace std;

typedef double fType;

struct BC_F {
  fType* NumPaths;
  bool* Visited;

  BC_F(fType* _NumPaths, bool* _Visited) : 
    NumPaths(_NumPaths), Visited(_Visited) {}
  inline bool update(intT s, intT d){ //Update function for forward phase
    fType oldV = NumPaths[d];
    NumPaths[d] += NumPaths[s];
    return oldV == 0.0;
  }
  inline bool updateAtomic (intT s, intT d) { //atomic Update, basically an add
    volatile fType oldV, newV; 
    do { 
      oldV = NumPaths[d]; newV = oldV + NumPaths[s];
    } while(!CAS(&NumPaths[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; } //check if visited
};

struct BC_Back_F {
  fType* Dependencies;
  bool* Visited;
  BC_Back_F(fType* _Dependencies, bool* _Visited) : 
    Dependencies(_Dependencies), Visited(_Visited) {}
  inline bool update(intT s, intT d){ //Update function for backwards phase
    fType oldV = Dependencies[d];
    Dependencies[d] += Dependencies[s];
    return oldV == 0.0;
  }
  inline bool updateAtomic (intT s, intT d) { //atomic Update
    volatile fType oldV, newV;
    do {
      oldV = Dependencies[d];
      newV = oldV + Dependencies[s];
    } while(!CAS(&Dependencies[d],oldV,newV));
    return oldV == 0.0;
  }
  inline bool cond (intT d) { return Visited[d] == 0; } //check if visited
};

//vertex map function to mark visited vertexSubset
struct BC_Vertex_F {
  bool* Visited;
  BC_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    return 1;
  }
};

//vertex map function (used on backwards phase) to mark visited vertexSubset
//and add to Dependencies score
struct BC_Back_Vertex_F {
  bool* Visited;
  fType* Dependencies, *inverseNumPaths;
  BC_Back_Vertex_F(bool* _Visited, fType* _Dependencies, fType* _inverseNumPaths) : 
    Visited(_Visited), Dependencies(_Dependencies), inverseNumPaths(_inverseNumPaths) {}
  inline bool operator() (intT i) {
    Visited[i] = 1;
    Dependencies[i] += inverseNumPaths[i];
    return 1;
  }
};

template <class vertex>
void BC(intT start, graph<vertex> GA) {
  intT n = GA.n;
  intT threshold = GA.m/20;

  fType* NumPaths = newA(fType,n);
  {parallel_for(intT i=0;i<n;i++) NumPaths[i] = 0.0;}
  NumPaths[start] = 1.0;

  bool* Visited = newA(bool,n);
  {parallel_for(intT i=0;i<n;i++) Visited[i] = 0;}
  Visited[start] = 1;
  vertexSubset Frontier(n,start);
 
  vector<vertexSubset> Levels;
  Levels.push_back(Frontier);

  intT round = 0;
  timer t1,t2;
  while(!Frontier.isEmpty()){ //first phase
    round++;
    //cout<<"Round "<<round<<" "<<Frontier.numNonzeros()<<endl;
    vertexSubset output = edgeMap(GA, Frontier, BC_F(NumPaths,Visited),threshold);
    vertexMap(output, BC_Vertex_F(Visited)); //mark visited
    Levels.push_back(output); //save frontier onto Levels
    Frontier = output;
  }

  fType* Dependencies = newA(fType,n);
  {parallel_for(intT i=0;i<n;i++) Dependencies[i] = 0.0;}

  //invert numpaths
  fType* inverseNumPaths = NumPaths;
  {parallel_for(intT i=0;i<n;i++) inverseNumPaths[i] = 1/inverseNumPaths[i];}

  Levels[round].del();
  //reuse Visited
  {parallel_for(intT i=0;i<n;i++) Visited[i] = 0;}
  Frontier = Levels[round-1];
  vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));

  //tranpose graph
  GA.transpose();
 
  for(intT r=round-2;r>=0;r--) { //backwards phase
    vertexSubset output = edgeMap(GA, Frontier, 
			      BC_Back_F(Dependencies,Visited),threshold);
    output.del();
    Frontier.del();
    Frontier = Levels[r]; //gets frontier from Levels array
    //vertex map to mark visited and update Dependencies scores
    vertexMap(Frontier,BC_Back_Vertex_F(Visited,Dependencies,inverseNumPaths));
  }
  
  Frontier.del();

  //Update dependencies scores
  parallel_for(intT i=0;i<n;i++) {
    Dependencies[i]=(Dependencies[i]-inverseNumPaths[i])/inverseNumPaths[i];
  }
  free(inverseNumPaths);
  free(Visited);
  free(Dependencies);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  bool binary = P.getOptionValue("-b");
  long start = P.getOptionLongValue("-r",0);
  long rounds = P.getOptionLongValue("-rounds",3);
  if(symmetric) {
    graph<symmetricVertex> G = 
      readGraph<symmetricVertex>(iFile,symmetric,binary); //symmetric graph
    BC((intT)start,G);
    for(int r=0;r<rounds;r++) {
      startTime();
      BC((intT)start,G);
      nextTime("BC");
    }
    G.del(); 
  } else {
    graph<asymmetricVertex> G = 
      readGraph<asymmetricVertex>(iFile,symmetric,binary); //asymmetric graph
    BC((intT)start,G);
    G.transpose();
    for(int r=0;r<rounds;r++) {
      startTime();
      BC((intT)start,G);
      nextTime("BC");
      G.transpose();
    }
    G.del();
  }
}
