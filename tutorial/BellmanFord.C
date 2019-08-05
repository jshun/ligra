#define WEIGHTED 1
#include "ligra.h"

struct BF_F {
  long* ShortestPathLen;
  BF_F(long* _ShortestPathLen) : 
    ShortestPathLen(_ShortestPathLen) {}
  inline bool updateAtomic (long s, long d, long edgeLen){ //atomic Update
    //FILL IN
  }
  inline bool update (long s, long d, long edgeLen) {
    return updateAtomic(s,d,edgeLen);
  }
  inline bool cond (long d) { 
    //FILL IN
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long n = GA.n;
  //initialize ShortestPathLen to "infinity"
  long* ShortestPathLen = new long[n];
  {parallel_for(long i=0;i<n;i++) ShortestPathLen[i] = LONG_MAX;}
  ShortestPathLen[start] = 0;

  vertexSubset Frontier(n,start); //initial frontier

  long round = 0;
  while(!Frontier.isEmpty()){
    if(round == n) {
      //negative weight cycle
      cout << "negative cycle\n";
      break;
    }
    vertexSubset output = edgeMap(GA, Frontier, BF_F(ShortestPathLen), GA.m/20, remove_duplicates);
    Frontier.del();
    Frontier = output;
    round++;
  }
  Frontier.del();
  free(ShortestPathLen);
}
