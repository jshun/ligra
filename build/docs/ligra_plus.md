---
id: compressed
sectionid: docs
layout: docs
title: Compressed Graphs
next: introduction.html
redirect_from: "docs/index.html"
---

Compressed Graphs
----------

Running code in Ligra+ 
-----------
When using Ligra+, graphs must first be compressed using the encoder
program provided. The encoder program takes as input a file in the
format described in the next section, as well as an output file
name. For symmetric graphs, the flag "-s" should be passed before the
filenames, and for weighted graphs, the flag "-w" should be passed
before the filenames. For example:

```
$ ./encoder -s ../inputs/rMatGraph_J_5_100 inputs/rMatGraph_J_5_100.compressed
$ ./encoder -s -w ../inputs/rMatGraph_WJ_5_100 inputs/rMatGraph_WJ_5_100.compressed
```
 
 After compressing the graphs, the applications can be run in the same
 manner as in Ligra. For example:

 ```
 $ ./BFS -s ../inputs/rMatGraph_J_5_100.compressed
 $ ./BellmanFord -s ../inputs/rMatGraph_WJ_5_100.compressed
