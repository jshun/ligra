#ifndef VERTEX_H
#define VERTEX_H

#include "vertexSubset.h"

using namespace std;

struct symmetricVertex {
#ifndef WEIGHTED
  uintE* neighbors;
#else 
  intE* neighbors;
#endif
  uintT degree;
  void del() {free(neighbors); }
#ifndef WEIGHTED
symmetricVertex(uintE* n, uintT d) 
#else 
symmetricVertex(intE* n, uintT d) 
#endif
: neighbors(n), degree(d) {}
#ifndef WEIGHTED
  uintE* getInNeighbors () { return neighbors; }
  uintE* getOutNeighbors () { return neighbors; }
  uintE getInNeighbor(uintT j) { return neighbors[j]; }
  uintE getOutNeighbor(uintT j) { return neighbors[j]; }
  void setInNeighbors(uintE* _i) { neighbors = _i; }
  void setOutNeighbors(uintE* _i) { neighbors = _i; }
#else
  //weights are stored in the entry after the neighbor ID
  //so size of neighbor list is twice the degree
  intE* getInNeighbors () { return neighbors; }
  intE* getOutNeighbors () { return neighbors; }
  intE getInNeighbor(intT j) { return neighbors[2*j]; }
  intE getOutNeighbor(intT j) { return neighbors[2*j]; }
  intE getInWeight(intT j) { return neighbors[2*j+1]; }
  intE getOutWeight(intT j) { return neighbors[2*j+1]; }
  void setInNeighbors(intE* _i) { neighbors = _i; }
  void setOutNeighbors(intE* _i) { neighbors = _i; }
#endif

  template <class F>
  void decodeInNghBreakEarly(long i, bool* vertexSubset, F &f, bool* next, bool parallel = 0) {
    uintE d = getInDegree();
    if (!parallel || d < 1000) {
      for (uintE j=0; j<d; j++){
        uintE ngh = getInNeighbor(j);
#ifndef WEIGHTED
        if (vertexSubset[ngh] && f.update(ngh,i))
#else
        if (vertexSubset[ngh] && f.update(ngh,i,getInWeight(j)))
#endif
          next[i] = 1;
        if(!f.cond(i)) break;
      }
    } else {
      parallel_for(uintE j=0; j<d; j++){
        uintE ngh = getInNeighbor(j);
#ifndef WEIGHTED
        if (vertexSubset[ngh] && f.update(ngh,i))
#else
        if (vertexSubset[ngh] && f.update(ngh,i,getInWeight(j)))
#endif
          next[i] = 1;
      }
    }
  }

  template <class F>
  void decodeOutNgh(long i, bool* vertexSubset, F &f, bool* next) {
    uintE d = getOutDegree();
    if(d < 1000) {
      for(uintE j=0; j<d; j++){
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if (f.cond(ngh) && f.updateAtomic(i,ngh))
#else 
        if (f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j))) 
#endif
          next[ngh] = 1;
      }
    } else {
      parallel_for(uintE j=0; j<d; j++){
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if (f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
          if (f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
        next[ngh] = 1;
      }
    }
  }

  template <class F>
  void decodeOutNghSparse(long i, uintT o, F &f, uintE* outEdges) {
    uintE d = getOutDegree();
    if(d < 1000) {
      for (uintE j=0; j < d; j++) {
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if(f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
        if(f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
          outEdges[o+j] = ngh;
        else outEdges[o+j] = UINT_E_MAX;
      }
    } else {
      parallel_for (uintE j=0; j < d; j++) {
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if(f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
        if(f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
          outEdges[o+j] = ngh;
        else outEdges[o+j] = UINT_E_MAX;
      }
    }
  }

  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
};

struct asymmetricVertex {
#ifndef WEIGHTED
  uintE* inNeighbors, *outNeighbors;
#else
  intE* inNeighbors, *outNeighbors;
#endif
  uintT outDegree;
  uintT inDegree;
  void del() {free(inNeighbors); free(outNeighbors);}
#ifndef WEIGHTED
asymmetricVertex(uintE* iN, uintE* oN, uintT id, uintT od) 
#else
asymmetricVertex(intE* iN, intE* oN, uintT id, uintT od) 
#endif
: inNeighbors(iN), outNeighbors(oN), inDegree(id), outDegree(od) {}
#ifndef WEIGHTED
  uintE* getInNeighbors () { return inNeighbors; }
  uintE* getOutNeighbors () { return outNeighbors; }
  uintE getInNeighbor(uintT j) { return inNeighbors[j]; }
  uintE getOutNeighbor(uintT j) { return outNeighbors[j]; }
  void setInNeighbors(uintE* _i) { inNeighbors = _i; }
  void setOutNeighbors(uintE* _i) { outNeighbors = _i; }
#else 
  intE* getInNeighbors () { return inNeighbors; }
  intE* getOutNeighbors () { return outNeighbors; }
  intE getInNeighbor(uintT j) { return inNeighbors[2*j]; }
  intE getOutNeighbor(uintT j) { return outNeighbors[2*j]; }
  intE getInWeight(uintT j) { return inNeighbors[2*j+1]; }
  intE getOutWeight(uintT j) { return outNeighbors[2*j+1]; }
  void setInNeighbors(intE* _i) { inNeighbors = _i; }
  void setOutNeighbors(intE* _i) { outNeighbors = _i; }
#endif

  template<class F>
  void decodeInNghBreakEarly(long i, bool* vertexSubset, F &f, bool* next, bool parallel = 0) {
    uintE d = getInDegree();
    if (!parallel || d < 1000) {
      for (uintE j=0; j<d; j++){
        uintE ngh = getInNeighbor(j);
#ifndef WEIGHTED
        if (vertexSubset[ngh] && f.update(ngh,i))
#else
        if (vertexSubset[ngh] && f.update(ngh,i,getInWeight(j)))
#endif
          next[i] = 1;
        if(!f.cond(i)) break;
      }
    } else {
      parallel_for(uintE j=0; j<d; j++){
        uintE ngh = getInNeighbor(j);
#ifndef WEIGHTED
        if (vertexSubset[ngh] && f.update(ngh,i))
#else
        if (vertexSubset[ngh] && f.update(ngh,i,getInWeight(j)))
#endif
          next[i] = 1;
      }
    }
  }

  template <class F>
  void decodeOutNgh(long i, bool* vertexSubset, F &f, bool* next) {
    uintE d = getOutDegree();
    if(d < 1000) {
      for(uintE j=0; j<d; j++){
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if (f.cond(ngh) && f.updateAtomic(i,ngh))
#else 
        if (f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j))) 
#endif
          next[ngh] = 1;
      }
    } else {
      parallel_for(uintE j=0; j<d; j++){
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if (f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
          if (f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
        next[ngh] = 1;
      }
    }
  }

  template <class F>
  void decodeOutNghSparse(long i, uintT o, F &f, uintE* outEdges) {
    uintE d = getOutDegree();
    if(d < 1000) {
      for (uintE j=0; j < d; j++) {
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if(f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
        if(f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
          outEdges[o+j] = ngh;
        else outEdges[o+j] = UINT_E_MAX;
      }
    } else {
      parallel_for (uintE j=0; j < d; j++) {
        uintE ngh = getOutNeighbor(j);
#ifndef WEIGHTED
        if(f.cond(ngh) && f.updateAtomic(i,ngh)) 
#else
        if(f.cond(ngh) && f.updateAtomic(i,ngh,getOutWeight(j)))
#endif
          outEdges[o+j] = ngh;
        else outEdges[o+j] = UINT_E_MAX;
      }
    }
  }



  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); swap(inDegree,outDegree); }
};




#endif
