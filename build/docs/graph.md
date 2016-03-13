---
id: graph
sectionid: docs
layout: docs
title: Graph API
prev: vertex.html
next: running_code.html
redirect_from: "docs/index.html"
---

Ligra has a single `graph` type, which is parametrized by an arbitrary 
`vertex` type (see the [vertex api](/ligra/docs/vertex.html)). We represent
a graph as a collection of vertices, along with memory that the graph owns, 
which is necessary for storing information about edges (weights, and endpoints). 

### **Graph Interface**

* **`void transpose()`** : no-op if the vertex type is symmetric, otherwise
  swaps the in-edges and out-edges for all vertices. 
* **`void del()`** : deletes memory owned by the graph 
