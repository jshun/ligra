---
id: tutorial_components
sectionid: docs
layout: docs
title: "Tutorial: Connected Components"
prev: tutorial_kcore.html
next: api.html
redirect_from: "docs/index.html"
---

In this project we will build a simple application to compute
the connected components of a graph. Given an undirected graph 
$G = (V, E)$  the connected components problem is to compute a 
map $f : V \rightarrow \mathbb{N}$ s.t. for $u,v \in V$, $f(v) = f(u)$ 
if and only if $v$ and $u$ are in the same component. Two vertices
are in the same component if there exists a path between them. 
This is an intuitive, and fundamental problem on graphs that appears
as a subroutine for many other algorithms in numerous areas such
as computer vision and data mining.

### A simple connectivity algorithm

A simple algorithm for connectivity goes as follows. Initially, every
vertex is a part of a component containing just itself. We identify a
component with a *label* and the label of vertex $v$ is just $v$. In 
each step of the algorithm, every $v \in V$ scans its neighbors and sets
its value to be the minimum over all of its neighbors labels. The algorithm
is to just apply this step until no vertex labels change in an iteration. 

### Implementation

Open up a new file, `Components.C`, and enter in the following stub. 

``` cpp
#include "ligra.h"
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long n = GA.n;
  long* IDs = new long[n]; 
  {parallel_for(long i=0;i<n;i++) IDs[i] = i;} //initialize unique IDs
}
```

The simple connectivity algorithm we described is actually a special case
of a *label-propagation* algorithm. We could implement a version where in 
each iteration, every vertex checks its neighbors, but this is wasteful. 
Instead, we should only have vertices that have changed their labels in 
the current round be active in the next round. Initially, we don't have this
information, so we set everyone to be active. 

Add the following code to the end of `Compute`. 

``` cpp
  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;} 
  vertexSubset Frontier(n,n,frontier); //initial frontier contains all vertices

  while(!Frontier.isEmpty()){ //iterate until IDS converge
    // TODO
  }
```

The loop termination condition in this app is simple: it just checks that 
the frontier is non-empty. Inside of the loop, we need to examine all of
the out-edges for each vertex in the frontier and see if we can lower the 
label of a neighbor. If this is possible, we should (1) lower the label and 
(2) place our neighbor in the next frontier. 

To do this, we will need to use an `edgeMap`, and pass in the following 
function. 

``` cpp
struct CC_F {
  long* IDs;
  CC_F(long* _IDs) : IDs(_IDs) {}
  inline bool updateAtomic (long s, long d) { //atomic Update
    return (writeMin(&IDs[d], IDs[s]));
  }
  inline bool update(long s, long d){ 
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    reutrn cond_true(d); //does nothing
  }
};
```

The final step is to fill in the remaining loop logic. All we should do is 
apply the `edgeMap` with `CC_F` and set the new frontier to be the output 
of the `edgeMap`. The finished application should like:

``` cpp
#include "ligra.h"

struct CC_F {
  long* IDs;
  CC_F(long* _IDs) : IDs(_IDs) {}
  inline bool updateAtomic (long s, long d) { //atomic Update
    return (writeMin(&IDs[d], IDs[s]));
  }
  inline bool update(long s, long d){ 
    return updateAtomic(s,d);
  }
  inline bool cond (long d) { 
    reutrn cond_true(d); //does nothing
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
    vertexSubset output = edgeMap(GA, Frontier, CC_F(IDs),
				  GA.m/20,DENSE,true); //tells edgemap to remove duplicates
    Frontier.del();
    Frontier = output;
  }
  Frontier.del(); free(IDs);
}
```

### Compilation

You can compile your algorithm by adding it to the Ligra `Makefile`. Open up the
`Makefile` in the `apps/` directory in your editor, and change the definition of
`ALL` to:

``` bash
ALL= Components encoder ... (other apps) ...
```

Now, compile the application by running `make`, which will produce a binary called 
`Components`.

### Testing

Let's run our program on one of the test-inputs provided by ligra in the `inputs/`
directory. Note that the `-s` flag tells Ligra that the graph is symmetric (undirected). 

``` 
./Components -s -start 1 ../inputs/rMatGraph_J_5_100
Running time : 0.000956
Running time : 0.000518
Running time : 0.000439
```
