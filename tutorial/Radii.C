#include "ligra.h"

//atomically do bitwise-OR of *a with b and store in location a
template <class ET>
inline void writeOr(ET *a, ET b) {
  volatile ET newV, oldV; 
  do {oldV = *a; newV = oldV | b;}
  while ((oldV != newV) && !CAS(a, oldV, newV));
}

struct Radii_F {
  long round;
  long* radii;
  long* Visited, *NextVisited;
  Radii_F(long* _Visited, long* _NextVisited, long* _radii, long _round) : 
    Visited(_Visited), NextVisited(_NextVisited), radii(_radii), round(_round) 
  {}
  inline bool updateAtomic (long s, long d){ //atomic Update
    //FILL IN
  }
  inline bool update (long s, long d){
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    //FILL IN
  }
};

//function passed to vertex map to sync NextVisited and Visited
struct Radii_Vertex_F {
  long* Visited, *NextVisited;
  Radii_Vertex_F(long* _Visited, long* _NextVisited) :
    Visited(_Visited), NextVisited(_NextVisited) {}
  inline bool operator() (long i) {
    Visited[i] = NextVisited[i];
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n;
  long* radii = new long[n];
  long* Visited = new long[n];
  long* NextVisited = new long[n];
  {parallel_for(long i=0;i<n;i++) {
    radii[i] = -1;
    Visited[i] = NextVisited[i] = 0;
    }}
  long sampleSize = min(n,(long)64);
  uint* starts = new uint[sampleSize];
  
  {parallel_for(ulong i=0;i<sampleSize;i++) { //initial set of vertices
      long v = hashInt(i) % n;
    radii[v] = 0;
    starts[i] = v;
    NextVisited[v] = (long) 1<<i;
    }}

  vertexSubset Frontier(n,sampleSize,starts); //initial frontier of size 64

  long round = 0;
  while(!Frontier.isEmpty()){
    round++;
    vertexMap(Frontier, Radii_Vertex_F(Visited,NextVisited));
    vertexSubset output = edgeMap(GA,Frontier,Radii_F(Visited,NextVisited,radii,round));
    Frontier.del();
    Frontier = output;
  }
  free(Visited); free(NextVisited); Frontier.del(); free(radii); 
}
