// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2013 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifndef _BENCH_GETTIME_INCLUDED
#define _BENCH_GETTIME_INCLUDED

#include <stdlib.h>
#include <sys/time.h>
#include <iomanip>
#include <iostream>

struct timer {
  double totalTime;
  double lastTime;
  double totalWeight;
  bool on;
  struct timezone tzp;
  timer() {
    struct timezone tz = {0, 0};
    totalTime=0.0; 
    totalWeight=0.0;
    on=0; tzp = tz;}
  double getTime() {
    timeval now;
    gettimeofday(&now, &tzp);
    return ((double) now.tv_sec) + ((double) now.tv_usec)/1000000.;
  }
  void start () {
    on = 1;
    lastTime = getTime();
  } 
  double stop () {
    on = 0;
    double d = (getTime()-lastTime);
    totalTime += d;
    return d;
  } 
  double stop (double weight) {
    on = 0;
    totalWeight += weight;
    double d = (getTime()-lastTime);
    totalTime += weight*d;
    return d;
  } 

  double total() {
    if (on) return totalTime + getTime() - lastTime;
    else return totalTime;
  }

  double next() {
    if (!on) return 0.0;
    double t = getTime();
    double td = t - lastTime;
    totalTime += td;
    lastTime = t;
    return td;
  }

  void reportT(double time) {
    std::cout << std::setprecision(3) << time <<  std::endl;;
  }

  void reportTime(double time) {
    reportT(time);
  }

  void reportStop(double weight, std::string str) {
    std::cout << str << " :" << weight << ": ";
    reportTime(stop(weight));
  }

  void reportTotal() {
    double to = (totalWeight > 0.0) ? total()/totalWeight : total();
    reportTime(to);
    totalTime = 0.0;
    totalWeight = 0.0;
  }

  void reportTotal(std::string str) {
    std::cout << str << " : "; 
    reportTotal();}

  void reportNext() {reportTime(next());}

  void reportNext(std::string str) {std::cout << str << " : "; reportNext();}
};

static timer _tm;
#define timeStatement(_A,_string) _tm.start();  _A; _tm.reportNext(_string);
#define startTime() _tm.start();
#define stopTime(_weight,_str) _tm.reportStop(_weight,_str);
#define reportTime(_str) _tm.reportTotal(_str);
#define nextTime(_string) _tm.reportNext(_string);
#define nextTimeN() _tm.reportT(_tm.next());

#endif // _BENCH_GETTIME_INCLUDED

