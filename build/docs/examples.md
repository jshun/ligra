---
id: examples
sectionid: docs
layout: docs
title: Examples
next: introduction.html
redirect_from: "docs/index.html"
---

Implementation files are provided in the apps/ directory: 

* **BFS.C** (breadth-first search)
* **BFS-Bitvector.C** (breadth-first search with a bitvector to mark visited vertices)
* **BC.C** (betweenness centrality) 
* **Radii.C** (graph eccentricity estimation) 
* **Components.C** (connected components)
* **BellmanFord.C** (Bellman-Ford shortest paths)
* **PageRank.C**
* **PageRankDelta.C** 
* **BFSCC.C** (connected components based on BFS)
* **KCore.C** (computes k-cores of the graph)


Eccentricity Estimation 
-------- 
Code for eccentricity estimation is available in the
apps/eccentricity/ directory: 

* **kBFS-Ecc.C** (2 passes of multiple BFS's)
* **kBFS-1Phase-Ecc.C** (1 pass of multiple BFS's)
* **FM-Ecc.C** (estimation using Flajolet-Martin counters; an implementation of
   a variant of HADI from *TKDD '11*)
* **LogLog-Ecc.C** (estimation using LogLog counters; an implementation of a 
  variant of HyperANF from *WWW '11*)
* **RV.C** (parallel implementation of the algorithm by Roditty and 
  Vassilevska Williams from *STOC '13*)
* **CLRSTV.C** (parallel implementation of a variant of the algorithm by Chechik, 
  Larkin, Roditty, Schoenebeck, Tarjan, and Vassilevska Williams from *SODA '14*)
* **kBFS-Exact.C** (exact algorithm using multiple BFS's)
* **TK.C** (a parallel implementation of the exact algorithm by Takes and 
  Kosters from *Algorithms '13*)
*  **Simple-Approx-Ecc.C** (simple 2-approximation algorithm)

Follow the same instructions as above for compilation, but from the apps/eccentricity/ directory.

For kBFS-Ecc.C, kBFS-1Phase-Ecc.C, FM-Ecc.C, LogLog-Ecc.C, and
kBFS-Exact.C, the "-r" flag followed by an integer indicates the
maximum number of words to associate with each vertex. For all
implementations, the "-s" flag should be used as the current
implementations are designed for undirected graphs.  To output the
eccentricity estimates to a file, use the "-out" flag followed by the
name of the output file. The file format is one integer per line, with
the eccentricity estimate for vertex *i* on line *i*.

