#include "ligra.h"

struct Update_Deg {
  long* Degrees;
  Update_Deg(long* _Degrees) : Degrees(_Degrees) {}
  inline bool updateAtomic (long s, long d){
    //FILL IN
  }
  inline bool update (long s, long d) { 
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    //FILL IN
  }
};

template<class vertex>
struct Deg_LessThan_K {
  vertex* V;
  long* coreNumbers;
  long* Degrees;
  long k;
  Deg_LessThan_K(vertex* _V, long* _Degrees, long* _coreNumbers, long _k) : 
    V(_V), k(_k), Degrees(_Degrees), coreNumbers(_coreNumbers) {}
  inline bool operator () (long i) {
    //FILL IN
  }
};

template<class vertex>
struct Deg_AtLeast_K {
  vertex* V;
  long *Degrees;
  long k;
  Deg_AtLeast_K(vertex* _V, long* _Degrees, long _k) : 
    V(_V), k(_k), Degrees(_Degrees) {}
  inline bool operator () (long i) {
    // FILL IN
  }
};

//assumes symmetric graph
// 1) iterate over all remaining active vertices
// 2) for each active vertex, remove if induced degree < k. Any vertex removed has
//    core-number (k-1) (part of (k-1)-core, but not k-core)
// 3) stop once no vertices are removed. Vertices remaining are in the k-core.
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  const long n = GA.n;
  bool* active = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) active[i] = 1;}
  vertexSubset Frontier(n, n, active);
  long* coreNumbers = new long[n];
  long* Degrees = new long[n];
  {parallel_for(long i=0;i<n;i++) {
      coreNumbers[i] = 0;
      Degrees[i] = GA.V[i].getOutDegree();
    }}
  long largestCore = -1;
  for (long k = 1; k <= n; k++) {
    while (true) {
      vertexSubset toRemove 
	= vertexFilter(Frontier,Deg_LessThan_K<vertex>(GA.V,Degrees,coreNumbers,k));
      vertexSubset remaining = vertexFilter(Frontier,Deg_AtLeast_K<vertex>(GA.V,Degrees,k));
      Frontier.del();
      Frontier = remaining;
      if (0 == toRemove.numNonzeros()) { // fixed point. found k-core
	toRemove.del();
        break;
      }
      else {
	vertexSubset output = edgeMap(GA,toRemove,Update_Deg(Degrees));
	toRemove.del(); output.del();
      }
    }
    if(Frontier.numNonzeros() == 0) { largestCore = k-1; break; }
  }
  cout << "largestCore was " << largestCore << endl;
  Frontier.del(); free(coreNumbers); free(Degrees);
}
