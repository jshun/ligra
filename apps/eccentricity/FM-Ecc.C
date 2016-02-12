// This code is part of the paper "An Evaluation of Parallel
// Eccentricity Estimation Algorithms on Undirected Real-World
// Graphs", presented at the ACM SIGKDD Conference on Knowledge
// Discovery and Data Mining, 2015.  
// Copyright (c) 2015 Julian Shun
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
#include <sstream>
#include <math.h>

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

//Update function does a bitwise-or
struct Ecc_F {
  intE round;
  intE* ecc;
  intE* VisitedArray, *NextVisitedArray;
  long length;
  Ecc_F(long _length, intE* _Visited, intE* _NextVisited, 
	       intE* _ecc, intE _round) : 
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited), ecc(_ecc), round(_round) 
  {}
  inline bool update(const uintE &s, const uintE &d){
    bool changed = 0;
    for(long i=0;i<length;i++) {
      intE toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	NextVisitedArray[d*length+i] |= toWrite;
	if(ecc[d] != round) { ecc[d] = round; changed = 1; }
      }
    }
    return changed;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    bool changed = 0; 
    for(long i=0; i<length;i++) {
      intE toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	writeOr(&NextVisitedArray[d*length+i],toWrite);
	intE oldEcc = ecc[d];
	if(ecc[d] != round) if(CAS(&ecc[d],oldEcc,round)) changed = 1;
      }
    }
    return changed;
  }
  inline bool cond(const uintE &i) { return cond_true(i);}};

//function passed to vertex map to sync NextVisited and Visited
struct Ecc_Vertex_F {
  intE* VisitedArray, *NextVisitedArray;
  long length;
  Ecc_Vertex_F(long _length, intE* _Visited, intE* _NextVisited) :
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited) {}
  inline bool operator() (const uintE &i) {
    for(long j=i*length;j<(i+1)*length;j++)
      VisitedArray[j] = NextVisitedArray[j];
    return 1;
  }
};

timer t0,t1,t2;

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("main loop");
  t2.reportTotal("total time excluding writing to file");
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t2.start();
  long length = P.getOptionLongValue("-r",0); //number of counters per vertex
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  srand (time(NULL));
  uintT seed = rand();
  cout << "seed = " << seed << endl;
  t0.start();
  long n = GA.n;
  length = max((long)1,length);

  intE* VisitedArray = newA(intE,n*length);
  intE* NextVisitedArray = newA(intE,n*length);
  intE* ecc = newA(intE,n);

  parallel_for(ulong i=0;i<n*length;i++)  { //initialize FM bit-vectors
    intE rand = hashInt((uintE)i+seed);
    intE rightMostBit  = (rand == 0) ? 0 : log2(rand&-rand);
    NextVisitedArray[i] = (1 << rightMostBit);
  }

  {parallel_for(long i=0;i<n;i++) {
      ecc[i] = 0;
    }}
  t0.stop();
  t1.start();
  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;} 
  vertexSubset Frontier(n,n,frontier); //initial frontier contains all vertices

  intE round = 0;
  while(!Frontier.isEmpty()){
    round++;
    vertexMap(Frontier, Ecc_Vertex_F(length,VisitedArray,NextVisitedArray));
    vertexSubset output = 
      edgeMap(GA,Frontier,Ecc_F(length,VisitedArray,NextVisitedArray,ecc,round),GA.m/20);
    Frontier.del();
    Frontier = output;
  }
  Frontier.del();
  t1.stop();
  t0.start();
  free(VisitedArray); free(NextVisitedArray); 
  t0.stop();
  t2.stop();
  reportAll();
  if(oFile != NULL) { //write eccentricities to file if desired
    ofstream file (oFile, ios::out | ios::binary);
    stringstream ss;
    for(long i=0;i<GA.n;i++) ss << ecc[i] << endl;
    file << ss.str();
    file.close();
  }
  free(ecc);
}
