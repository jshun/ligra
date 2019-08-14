#include "ligra.h"

struct CC_F {
  long* IDs;
  CC_F(long* _IDs) : IDs(_IDs) {}
  inline bool updateAtomic (long s, long d) { //atomic Update
    //FILL IN
  }
  inline bool update(long s, long d){ 
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    //FILL IN
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n;
  long* IDs = new long[n]; 
  {parallel_for(long i=0;i<n;i++) IDs[i] = i;} //initialize unique IDs

  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;} 
  vertexSubset Frontier(n,n,frontier); //initial frontier contains all vertices

  while(!Frontier.isEmpty()){ //iterate until IDS converge
    vertexSubset output = edgeMap(GA, Frontier, CC_F(IDs), GA.m/20,
				  remove_duplicates); //tells edgemap to remove duplicates
    Frontier.del();
    Frontier = output;
  }
  Frontier.del(); free(IDs);
}
