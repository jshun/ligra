---
id: introduction
sectionid: docs
layout: docs
title: The Ligra Graph Processing Framework
next: getting_started.html
redirect_from: "docs/index.html"
---

## Tutorial

You can find slides for the Ligra tutorial [here](https://github.com/jshun/ligra/blob/master/tutorial/tutorial.pdf).

## Introduction

Welcome! These documents will teach you about the Ligra Graph Processing Framework. Ligra 
is a lightweight framework for processing graphs in shared memory. It is particularly 
suited for implementing parallel graph traversal algorithms where only a subset of the 
vertices are processed in an iteration. The project was motivated by the fact that the 
largest publicly available real-world graphs all fit in shared memory. When graphs fit 
in shared-memory, processing them using Ligra can give performance improvements of up 
orders of magnitude compared to distributed-memory graph processing systems. 

This document is split up into a number of sections.  

* [Getting Started](/ligra/docs/getting_started.html) - Set up your machine to use Ligra
* [Tutorial: BFS](/ligra/docs/tutorial_bfs.html) - Develop a simple breadth-first search 
  in Ligra
* [Tutorial: KCore](/ligra/docs/tutorial_kcore.html) - Develop an app to compute the 
  KCores of a graph
* [API](/ligra/docs/api.html) - Comprehensive API reference 
* [Examples](/ligra/docs/examples.html) - Overview of example Ligra applications


## Resources

Julian Shun and Guy E. Blelloch. [*Ligra: A
Lightweight Graph Processing Framework for Shared
Memory*](https://people.csail.mit.edu/jshun/ligra.pdf). Proceedings of the
ACM SIGPLAN Symposium on Principles and Practice of Parallel
Programming (PPoPP), pp. 135-146, 2013.

Julian Shun, Laxman Dhulipala, and Guy E. Blelloch. [*Smaller and Faster:
Parallel Processing of Compressed Graphs with
Ligra+*](https://people.csail.mit.edu/jshun/ligra+.pdf).
Proceedings of the
IEEE Data Compression Conference (DCC), pp. 403-412, 2015.

Julian Shun. [*An Evaluation of Parallel Eccentricity Estimation
Algorithms on Undirected Real-World
Graphs*](https://people.csail.mit.edu/jshun/kdd-final.pdf).
Proceedings of
the ACM SIGKDD Conference on Knowledge Discovery and Data Mining
(KDD), pp. 1095-1104, 2015.

Julian Shun, Farbod Roosta-Khorasani, Kimon Fountoulakis, and Michael
W. Mahoney. [*Parallel Local Graph Clustering*](https://people.csail.mit.edu/jshun/local.pdf). Proceedings of the
International Conference on Very Large Data Bases (VLDB), 9(12),
pp. 1041-1052, 2016.

Laxman Dhulipala, Guy E. Blelloch, and Julian Shun. [*Julienne: A Framework for Parallel Graph Algorithms using Work-efficient Bucketing*](https://people.csail.mit.edu/jshun/bucketing.pdf). Proceedings of the ACM Symposium on Parallelism in Algorithms and Architectures (SPAA), pp. 293-304, 2017.