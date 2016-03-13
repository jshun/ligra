#include "ligra.h"

struct BFS_F {
  long* Parents;
  BFS_F(long* _Parents) : Parents(_Parents) {}
  inline bool updateAtomic (long s, long d){ //atomic version of Update
    //FILL IN
  }
  inline bool update (long s, long d) { //Update
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    //FILL IN
  } 
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long n = GA.n;
  //creates Parents array, initialized to all -1, except for start
  long* Parents = new long[n];
  parallel_for(long i=0;i<n;i++) Parents[i] = -1L;
  Parents[start] = start;
  vertexSubset Frontier(n,start); //creates initial frontier
  while(!Frontier.isEmpty()){ //loop until frontier is empty
    vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents));    
    Frontier.del();
    Frontier = output; //set new frontier
  } 
  Frontier.del();
  free(Parents); 
}
