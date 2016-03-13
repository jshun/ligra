---
id: vertex
sectionid: docs
layout: docs
title: Vertex API
prev: api.html
next: graph.html
redirect_from: "docs/index.html"
---

Ligra currently supports four different vertex types. 

* `symmetricVertex`
* `asymmetricVertex`
* `compressedSymmetricVertex`
* `compressedAsymmetricVertex`

The framework automatically picks the correct vertex type to use based on
the command line flag a user supplies when running the application. 

* If the flag `-c` is supplied, then the graph is assumed to be *compressed* and 
one of the compressed vertex types is selected. 
* If the flag `-s` is supplied, the graph is assumed to be *symmetric* (undirected) 
and a symmetric vertex type is selected. Otherwise, an assymetric vertex type is selected. 

Certain applications, like PageRank, must work on both directed and undirected graphs,
and therefore must do things like only iterating over *out*-edges, etc. In order to maximize
code-reuse and not have different PageRank applications for directed and undirected graphs
Ligra ensures that all four vertex types (implicitly) ascribe to the same interface. 

### **Vertex Interface**

* **`intE getInNeighbor(intT j)`** : Returns the j'th in-neighbor of the vertex. 
* **`intE getOutNeighbor(intT j)`** : Returns the j'th out-neighbor of the vertex. 
* **`intE getInWeight(intT j)`** : Returns the weight of the edge from the j'th in-neighbor.
* **`intE getOutWeight(intT j)`** : Returns the weight of the edge to the j'th out-neighbor.
* **`intE getInDegree()`** : Returns the number of in-neighbors of the vertex.
* **`intE getOutDegree()`** : Returns the number of out-neighbors of the vertex.


