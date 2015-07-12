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

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

//Update function does a bitwise-or
struct Ecc_F {
  uintE round;
  uintE* ecc;
  long* VisitedArray, *NextVisitedArray;
  long length;
  Ecc_F(long _length, long* _Visited, long* _NextVisited, 
	       uintE* _ecc, uintE _round) : 
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited), ecc(_ecc), round(_round) 
  {}
  inline bool update(const uintE &s, const uintE &d){
    bool changed = 0;
    for(long i=0;i<length;i++) {
      long toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	NextVisitedArray[d*length+i] |= toWrite;
	if(ecc[d] < round) { ecc[d] = round; changed = 1; }
      }
    }
    return changed;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    bool changed = 0; 
    for(long i=0; i<length;i++) {
      long toWrite = VisitedArray[d*length+i] | VisitedArray[s*length+i];
      if(VisitedArray[d*length+i] != toWrite){
	writeOr(&NextVisitedArray[d*length+i],toWrite);
	uintE oldEcc = ecc[d];
	if(ecc[d] < round) if(CAS(&ecc[d],oldEcc,round)) changed = 1;
      }
    }
    return changed;
  }
  inline bool cond(const uintE &i) { return cond_true(i);}};

//function passed to vertex map to sync NextVisited and Visited
struct Ecc_Vertex_F {
  long* VisitedArray, *NextVisitedArray;
  long length;
  Ecc_Vertex_F(long _length, long* _Visited, long* _NextVisited) :
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited) {}
  inline bool operator() (const uintE &i) {
    for(long j=i*length;j<(i+1)*length;j++)
      VisitedArray[j] |= NextVisitedArray[j];
    return 1;
  }
};

timer t0;

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t0.start();
  long length = P.getOptionLongValue("-r",0);
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  long n = GA.n;
  uintE* allEcc = newA(uintE,n);
  parallel_for(intT i=0;i<n;i++) allEcc[i] = 0;
  length = max((long)1,min((n+63)/64,(long)length));

  long numIters = (n+length*64-1)/(length*64);
  cout << "length = "<<length << " numIters = "<<numIters << endl;
  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);
  uintE* ecc = newA(uintE,n);

  for(long iter = 0; iter < numIters; iter++) {
    {parallel_for(long i=0;i<n*length;i++) 
	VisitedArray[i] = NextVisitedArray[i] = 0;}

    {parallel_for(long i=0;i<n;i++) {
	ecc[i] = 0;
      }}
    long sampleSize = min(n-64*length*iter,(long)64*length);

    uintE* starts = newA(uintE,sampleSize);
  
    {parallel_for(long i=0;i<sampleSize;i++) { //initial set of vertices
	uintE v = 64*length*iter+i;
	starts[i] = v;
	NextVisitedArray[v*length + i/64] = (long) 1<<(i%64);
      }}
    vertexSubset Frontier(n,sampleSize,starts); //initial frontier of size 64

    uintE round = 0;
    while(!Frontier.isEmpty()){
      round++;
      vertexMap(Frontier, Ecc_Vertex_F(length,VisitedArray,NextVisitedArray));
      vertexSubset output = 
	edgeMap(GA, Frontier, 
		Ecc_F(length,VisitedArray,NextVisitedArray,ecc,round),
		GA.m/20);
      Frontier.del();
      Frontier = output;
    }
    Frontier.del();
    {parallel_for(intT i=0;i<n;i++) allEcc[i] = max(allEcc[i],ecc[i]);}
  }
  free(ecc); free(VisitedArray); free(NextVisitedArray); 
  t0.reportTotal("total time excluding writing to file");
  if(oFile != NULL) {
    ofstream file (oFile, ios::out | ios::binary);
    stringstream ss;
    for(long i=0;i<GA.n;i++) ss << allEcc[i] << endl;
    file << ss.str();
    file.close();
  }
  free(allEcc);
}
