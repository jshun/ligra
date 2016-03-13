---
id: api 
sectionid: docs
layout: docs
title: API
next: introduction.html
redirect_from: "docs/index.html"
---

The Ligra API is conceptually very simple. At a high level, users can 

* Represent subsets of vertices
* Apply a *map* function over the out-edges of a subset of vertices
* Apply a *map* function over a subset of vertices
* Apply a *filter* function over a subset of vertices

Using these simple primitives, we can design and cleanly express most
graph algorithms. 

### Primitive Types

Ligra uses several 'primitive types' that are non-standard in C/C++. This is 
to allow the same application code to work on graphs with more than 
`MAX_INT` many vertices or edges without changing all of the type-annotations
from `int` to `long`. 

* **`intT`** : The vertex identifier type
* **`uintT`** : The unsigned vertex identifier type
* **`intE`** : The edge identifier type
* **`uintE`** : The unsigned edge identifier type

### Data Structures 

**vertexSubset**: represents a subset of vertices in the graph. 

The most common constructors are provided below. A full list can be 
found in `ligra.h`.

* **`vertexSubset(long n, intE v)`**: constructs a subset with a single
  element, `v`.
* **`vertexSubset(long n, long m, uintE* indices)`**: constructs a subset
  of size `m` containing the the first `m` elements pointed to by `indices`. 
* **`vertexSubset(long n, bool* bits)`**: constructs a subset of size $n$
  whose elements are the entries of `bits` that are set to `1`. 

### Functions

#### **edgeMap**: 

``` cpp
template <class vertex, class F>
vertexSubset edgeMap(graph<vertex> GA, vertexSubset &V, F f, 
                     intT threshold = -1, char option=DENSE, bool remDups=false);
```

Takes as input 3 required arguments and 3 optional arguments:

* a `graph` *G*
* `vertexSubset` *V* 
* struct *F*
* threshold argument (optional, default threshold is *m*/20)
* an option in {`DENSE`, `DENSE_FORWARD`} (optional, default value is `DENSE`)
* a boolean indicating whether to remove duplicates (optional, default 
  does not remove duplicates). 

It returns as output a vertexSubset `Out` (see section 4 of paper for how `Out` is computed).

The *F* struct contains three boolean functions: `update`, `updateAtomic`
and `cond`.  `update` and `updateAtomic` should take two integer arguments
(corresponding to source and destination vertex). In addition,
`updateAtomic` should be atomic with respect to the destination
vertex. `cond` takes one argument corresponding to a vertex.  For the
`cond` function which always returns true, `cond_true` can be called. 

``` cpp
struct F {
  inline bool update (intT s, intT d) {
  //fill in
  }
  inline bool updateAtomic (intT s, intT d){ 
  //fill in
  }
  inline bool cond (intT d) {
  //fill in 
  }
};
```

The threshold argument determines when `edgeMap` switches between
`edgeMapSparse` and `edgeMapDense`---for a threshold value *T*, `edgeMap`
calls `edgeMapSparse` if the vertex subset size plus its number of
outgoing edges is less than *T*, and otherwise calls `edgeMapDense`.

`DENSE` is a read-based version where all vertices not satisfying
`cond` loop over their incoming edges and `DENSE_FORWARD` is a write-based
version where each frontier vertex loops over its outgoing edges. This
optimization is described in Section 4 of the paper.

Note that duplicate removal can only be avoided if `updateAtomic`
returns true at most once for each vertex in a call to edgeMap.

#### **vertexMap**: 

``` cpp
template <class F>
void vertexMap(vertexSubset V, F add);
```

Takes as input: 

* `vertexSubset` $V$ and a
* function $F$ which is applied to all vertices in $V$. It does not have
  a return value.

#### **vertexFilter**:

``` cpp
template <class F>
void vertexFilter(vertexSubset V, F filter);
```

Takes as input 

* `vertexSubset` $V$ and 
* boolean function $F$ which is applied to all vertices in $V$.. It 
  returns a vertexSubset containing all vertices $v \in V$ such 
  that $F(v)$ returns true.

``` cpp
struct F {
  inline bool operator () (intT i) {
  //fill in
  }
};
```

To write your own Ligra code, it would be helpful to look at the code
for the provided applications as reference.

Currently the results of the computation are not used, but the code
can be easily modified to output the results to a file.

To develop a new implementation, simply include "ligra.h" in the
implementation files. When finished, one may add it to the ALL
variable in Makefile. The function that is passed to the Ligra/Ligra+
driver is the following Compute function, which is filled in by the
user. The first argument is the graph, and second argument is a
structure containing the command line arguments, which can be
extracted using routines in parseCommandLine.h, or manually from
P.argv and P.argc.

``` cpp
template<class vertex>
void Compute(graph<vertex>& GA, commandLine P){ 

}
```
