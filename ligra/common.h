#ifndef LIGRA_COMMON_H
#define LIGRA_COMMON_H

struct Edge_F {
public:
  virtual inline bool update (uintE s, uintE d) { //Update
    return updateAtomic(s,d);
  }
  virtual inline bool updateAtomic (uintE s, uintE d) = 0; //atomic version of Update
  //cond function checks if vertex has been visited yet
  virtual inline bool cond (uintE d) { return 1; } 
};

#endif
