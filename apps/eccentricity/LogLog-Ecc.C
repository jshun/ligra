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

#define Kminus1 5
#define L6 0x41041041041041
#define H6 0x820820820820820
#define H6_comp 0xf7df7df7df7df7df

#define xLessY(_x, _y)					\
  (((((_x | H6) - (_y & H6_comp)) | (_x ^ _y)) ^ (_x | ~_y)) & H6);

#define xLessEqualY(_x, _y)					\
  (((((_y | H6) - (_x & H6_comp)) | (_x ^ _y)) ^ (_x & ~_y)) & H6);

#define mask(_xLessY)				\
    ((((_xLessY >> Kminus1) | H6) - L6) | H6) ^ _xLessY;

#define broadwordMax(_x,_y,_m)			\
    (_x & _m) | (_y & ~_m);

//atomically writeMax into location a
inline bool writeBroadwordMax(long *a, long b) {
  long c; bool r=0; long d, e, m;
  do {c = *a; d = xLessEqualY(b,c);
    if(d != H6) { //otherwise all values we are trying to write are smaller 
      m = mask(d);
      e = broadwordMax(b,c,m);
      r = CAS(a,c,e);
    } else break;}
    while (!r);
  return r;
}

//Update function does a broadword max on log-log counters
struct Ecc_F {
  intE round;
  intE* ecc;
  long* VisitedArray, *NextVisitedArray;
  long length;
  Ecc_F(long _length, long* _Visited, long* _NextVisited, 
	       intE* _ecc, intE _round) : 
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited), ecc(_ecc), round(_round) 
  {}
  inline bool update(const uintE &s, const uintE &d){
    bool changed = 0;
    for(long i=0;i<length;i++) {
      long nv = NextVisitedArray[d*length+i], v = VisitedArray[s*length+i];
      long a = xLessEqualY(v,nv);
      if(a != H6) {
	long b = mask(a);
	long c = broadwordMax(v,nv,b);
	NextVisitedArray[d*length+i] = c;
	if(ecc[d] != round) { ecc[d] = round; changed = 1; }
      }
    }
    return changed;
  }
  inline bool updateAtomic(const uintE &s, const uintE &d){
    bool changed = 0; 
    for(long i=0; i<length;i++) {
      if(writeBroadwordMax(&NextVisitedArray[d*length+i],VisitedArray[s*length+i])) {
	intE oldEcc = ecc[d];
	if(ecc[d] != round) if(CAS(&ecc[d],oldEcc,round)) changed = 1;
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
  long length = P.getOptionLongValue("-r",0); //number of words per vertex
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  t2.start();
  srand (time(NULL));
  uintT seed = rand();
  cout << "seed = " << seed << endl;
  t0.start();
  long n = GA.n;
  length = max((long)1,length);

  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);
  intE* ecc = newA(intE,n);

  parallel_for(ulong i=0;i<n*length;i++)  { 
    //initialize log-log counters (10 registers per counter)
    ulong counter = 0;
    for(ulong j=0;j<10;j++) {
      ulong rand = hashInt(i*10+j+(ulong)seed*10);
      ulong rightMostBit  = (rand == 0) ? 0 : log2(rand&-rand);
      counter |= (rightMostBit << (6*j));
    }
    NextVisitedArray[i] = counter;
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
      edgeMap(GA, Frontier, 
	      Ecc_F(length,VisitedArray,NextVisitedArray,ecc,round),
	      GA.m/20);
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
  if(oFile != NULL) {
    ofstream file (oFile, ios::out | ios::binary);
    stringstream ss;
    for(long i=0;i<GA.n;i++) ss << ecc[i] << endl;
    file << ss.str();
    file.close();
  }
  free(ecc);
}
