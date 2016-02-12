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
#include "blockRadixSort.h"
#include "CCBFS.h"

typedef pair<uintE,uintE> intPair;

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

template <class vertex>
struct getDegree {
  vertex* V;
  getDegree(vertex* VV) : V(VV) {}
  intE operator() (intE i) {return V[i].getOutDegree();}
};

struct maxFirstF { intPair operator() (const intPair& a, const intPair& b) 
  const {return (a.first>b.first) ? a : b;}};

//Update function does a bitwise-or
struct Ecc_F {
  uintE round;
  uintE* ecc;
  long* VisitedArray, *NextVisitedArray;
  const long length;
  Ecc_F(const long _length, long* _Visited, long* _NextVisited, 
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
  inline bool cond(const uintE &i) {return cond_true(i);}};

//function passed to vertex map to sync NextVisited and Visited
struct Ecc_Vertex_F {
  long* VisitedArray, *NextVisitedArray;
  const long length;
  Ecc_Vertex_F(const long _length, long* _Visited, long* _NextVisited) :
    length(_length), VisitedArray(_Visited), NextVisitedArray(_NextVisited) {}
  inline bool operator() (const uintE &i) {
    for(long j=i*length;j<(i+1)*length;j++)
      VisitedArray[j] = NextVisitedArray[j];
    return 1;
  }
};

timer t0,t1,t2,t3;

void reportAll() {
  t0.reportTotal("preprocess");
  t1.reportTotal("connected components");
  t2.reportTotal("Ecc phase 1");
  t3.reportTotal("total time excluding writing to file");
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  t3.start();
  long length = P.getOptionLongValue("-r",0); //number of words per vertex
  char* oFile = P.getOptionValue("-out"); //file to write eccentricites
  srand (time(NULL));
  uintT seed = rand();
  cout << "seed = " << seed << endl;
  t0.start();
  long n = GA.n;

  uintE* ecc = newA(uintE,n);
  {parallel_for(long i=0;i<n;i++) {
      ecc[i] = 0;
    }}
  t0.stop();

  //BEGIN COMPUTE CONNECTED COMPONENTS
  t1.start();  
  intE* Labels = newA(intE,n);
  {parallel_for(long i=0;i<n;i++) {
    if(GA.V[i].getOutDegree() == 0) Labels[i] = -i-1; //singletons
    else Labels[i] = INT_E_MAX;
    }}
  //get max degree vertex
  uintE maxV = sequence::reduce<uintE>((intE)0,(intE)n,maxF<intE>(),getDegree<vertex>(GA.V));
  //visit large component with BFS
  CCBFS(maxV,GA,Labels);
  //visit small components with label propagation
  Components(GA, Labels);

  //sort by component ID
  intPair* CCpairs = newA(intPair,n);
  {parallel_for(long i=0;i<n;i++)
    if(Labels[i] < 0)
      CCpairs[i] = make_pair(-Labels[i]-1,i);
    else CCpairs[i] = make_pair(Labels[i],i);
  }
  free(Labels);

  intSort::iSort(CCpairs, n, n+1,firstF<uintE,uintE>());

  uintE* changes = newA(uintE,n);
  changes[0] = 0;
  {parallel_for(long i=1;i<n;i++) 
      changes[i] = (CCpairs[i].first != CCpairs[i-1].first) ? i : UINT_E_MAX;}

  uintE* CCoffsets = newA(uintE,n);
  uintE numCC = sequence::filter(changes, CCoffsets, n, nonMaxF());
  CCoffsets[numCC] = n;
  free(changes);
  t1.stop();
  //END COMPUTE CONNECTED COMPONENTS

  //init data structures
  t0.start();
  length = max((long)1,min((n+63)/64,(long)length));
  long* VisitedArray = newA(long,n*length);
  long* NextVisitedArray = newA(long,n*length);  
  int* flags = newA(int,n);
  {parallel_for(long i=0;i<n;i++) flags[i] = -1;}
  uintE* starts = newA(uintE,n);
  t0.stop();

  t2.start();
  //BEGIN COMPUTE ECCENTRICITES PER COMPONENT
  for(long k = 0; k < numCC; k++) {
    uintE o = CCoffsets[k];
    uintE CCsize = CCoffsets[k+1] - o;
    if(CCsize == 2) { //size 2 CC's have ecc of 1
      ecc[CCpairs[o].second] = ecc[CCpairs[o+1].second] = 1;
    } else if(CCsize > 1) { //size 1 CC's already have ecc of 0
      //do main computation
      long myLength = min((long)length,((long)CCsize+63)/64);

      //initialize bit vectors for component vertices
      {parallel_for(long i=0;i<CCsize;i++) {
	uintT v = CCpairs[o+i].second;
	parallel_for(long j=0;j<myLength;j++)
	  VisitedArray[v*myLength+j] = NextVisitedArray[v*myLength+j] = 0;
	}}
      long sampleSize = min((long)CCsize,(long)64*myLength);

      uintE* starts2 = newA(uintE,sampleSize);

      //pick random vertices (could have duplicates)
      {parallel_for(ulong i=0;i<sampleSize;i++) {
	uintT index = hashInt(i+seed) % CCsize;
	if(flags[index] == -1 && CAS(&flags[index],-1,(int)i)) {
	  starts[i] = CCpairs[o+index].second;
	  NextVisitedArray[CCpairs[o+index].second*myLength + i/64] = (long) 1<<(i%64);
	} else starts[i] = UINT_E_MAX;
	}}

      //remove duplicates
      uintE numUnique = sequence::filter(starts,starts2,sampleSize,nonMaxF());

      //reset flags
      parallel_for(ulong i=0;i<sampleSize;i++) {
	uintT index = hashInt(i+seed) % CCsize;
	if(flags[index] == i) flags[index] = -1;
      }

      //first round
      vertexSubset Frontier(n,numUnique,starts2); //initial frontier
      //note: starts2 will be freed inside the following loop
      uintE round = 0;
      while(!Frontier.isEmpty()){
	round++;
	vertexMap(Frontier, Ecc_Vertex_F(myLength,VisitedArray,NextVisitedArray));
	vertexSubset output = 
	  edgeMap(GA,Frontier,Ecc_F(myLength,VisitedArray,NextVisitedArray,ecc,round),GA.m/20);
	Frontier.del();
	Frontier = output;
      }
      Frontier.del();
    }
  }
  t2.stop();
  //END COMPUTE ECCENTRICITES PER COMPONENT
  t0.start();
  free(flags); free(VisitedArray); free(NextVisitedArray); 
  free(CCoffsets); free(CCpairs); free(starts);
  t0.stop(); t3.stop();
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
