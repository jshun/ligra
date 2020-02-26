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

struct CC_Update_F {
  uintE* SrcIDs, *DestIDs, *DestPrevIDs;
  CC_Update_F(uintE* _SrcIDs, uintE* _DestIDs, uintE* _DestPrevIDs) : 
    SrcIDs(_SrcIDs), DestIDs(_DestIDs), DestPrevIDs(_DestPrevIDs) {}
  inline bool update(uintE s, uintE d){ //Update function writes min ID
    uintE origID = DestIDs[d];
    if(SrcIDs[s] < origID) {
      DestIDs[d] = min(origID,SrcIDs[s]);
      if(origID == DestPrevIDs[d]) return 1;
    } return 0; }
  inline bool updateAtomic (uintE s, uintE d) { //atomic Update
    uintE origID = DestIDs[d];
    return (writeMin(&DestIDs[d],SrcIDs[s]) && origID == DestPrevIDs[d]);
  }
  inline bool cond (uintE d) { return cond_true(d); } //does nothing
};

//function used by vertex map to sync prevIDs with IDs
struct CC_Copy_F {
  uintE* IDs, *prevIDs;
  CC_Copy_F(uintE* _IDs, uintE* _prevIDs) :
    IDs(_IDs), prevIDs(_prevIDs) {}
  inline bool operator () (uintE i) {
    prevIDs[i] = IDs[i];
    return 1; }};

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long nv = GA.nv, nh = GA.nh;
  uintE* IDsV = newA(uintE,nv), *prevIDsV = newA(uintE,nv);
  uintE* IDsH = newA(uintE,nh), *prevIDsH = newA(uintE,nh);
  {parallel_for(long i=0;i<nv;i++) IDsV[i] = i;} //initialize unique IDs
  {parallel_for(long i=0;i<nh;i++) IDsH[i] = prevIDsH[i] = UINT_E_MAX;}
  bool* frontier = newA(bool,nv);
  {parallel_for(long i=0;i<nv;i++) frontier[i] = 1;} 
  vertexSubset FrontierV(nv,nv,frontier); //initial frontier contains all vertices
  hyperedgeSubset FrontierH(nh);
  while(1){ //iterate until IDs converge
    //cout << FrontierV.numNonzeros() << endl;
    hyperedgeMap(FrontierH,CC_Copy_F(IDsH,prevIDsH));
    hyperedgeSubset output = vertexProp(GA, FrontierV, CC_Update_F(IDsV,IDsH,prevIDsH));
    FrontierH.del();
    FrontierH = output;
    if(FrontierH.isEmpty()) break;
    //cout << FrontierH.numNonzeros() << endl;
    vertexMap(FrontierV,CC_Copy_F(IDsV,prevIDsV));
    output = hyperedgeProp(GA, FrontierH, CC_Update_F(IDsH,IDsV,prevIDsV));
    FrontierV.del();
    FrontierV = output;
    if(FrontierV.isEmpty()) break;
    
  }
  FrontierV.del(); FrontierH.del(); free(IDsV); free(prevIDsV); free(IDsH); free(prevIDsH);
}
