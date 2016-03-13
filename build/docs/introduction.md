---
id: introduction
sectionid: docs
layout: docs
title: The Ligra Graph Processing Framework
next: getting_started.html
redirect_from: "docs/index.html"
---

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

Julian Shun and Guy Blelloch. [*Ligra: A
Lightweight Graph Processing Framework for Shared
Memory*](http://www.cs.cmu.edu/~jshun/ligra.pdf). Proceedings of the
ACM SIGPLAN Symposium on Principles and Practice of Parallel
Programming (PPoPP), pp. 135-146, 2013.

Julian Shun, Laxman Dhulipala and Guy Blelloch. [*Smaller and Faster:
Parallel Processing of Compressed Graphs with
Ligra+*](http://www.cs.cmu.edu/~jshun/ligra+.pdf). Proceedings of the
IEEE Data Compression Conference (DCC), pp. 403-412, 2015.

Julian Shun. [*An Evaluation of Parallel Eccentricity Estimation
Algorithms on Undirected Real-World
Graphs*](http://www.cs.cmu.edu/~jshun/kdd-final.pdf). Proceedings of
the ACM SIGKDD Conference on Knowledge Discovery and Data Mining
(KDD), 2015.
