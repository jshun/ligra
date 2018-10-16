//This is a Ligra implementation of a gradient descent-based parallel
//algorithm for collaborative filtering (matrix factorization)
//described in the paper "GraphMat: High performance graph analytics
//made productive", VLDB 2015
//(https://github.com/narayanan2004/GraphMat/blob/master/src/SGD.cpp). The
//Ligra implementation is written by Yunming Zhang (and slightly
//modified by Julian Shun).

//The input to the program is a symmetrized weighted bipartite graph
//between users and items, where the weights represent the rating a
//user gives to an item. Each vertex in the graph represents either a
//user or an item. The optional arguments to the program are as
//follows: "-K" specifies the dimension of the latent vector (default
//is 20), "-numiter" is the number of iterations of gradient descent
//to run (default is 5), "-step" is the step size in the algorithm
//(default is 0.00000035), "-lambda" is the regularization parameter
//(default is 0.001), and "-randInit" specifies that the latent vector
//should be initialized randomly (by default every entry is
//initialized to 0.5).

#define WEIGHTED 1
#include "ligra.h"
//uncomment the following line to compute the sum of squared errors per iteration
//#define COMPUTE_ERROR 1
//uncomment the following line to print out the sum of values in latent vector
//#define DEBUG 1

#ifdef COMPUTE_ERROR
double* squaredErrors;
#endif

template <class vertex>
struct CF_Edge_F {
  double* latent_curr, *error;
  vertex* V;
  int K;
  CF_Edge_F(vertex* _V, double* _latent_curr, double* _error, int _K) : 
    V(_V), latent_curr(_latent_curr), error(_error), K(_K) {}
  inline bool update(uintE s, uintE d, intE edgeLen){ //updates latent vector based on neighbors' data
    double estimate = 0;
    long current_offset = K*d, ngh_offset = K*s;
    double *cur_latent =  &latent_curr[current_offset], *ngh_latent = &latent_curr[ngh_offset];
    for(int i = 0; i < K; i++){
      estimate +=  cur_latent[i]*ngh_latent[i];
    }
    double err = edgeLen - estimate;

#ifdef COMPUTE_ERROR
    squaredErrors[d] += err*err;
#endif

    double* cur_error = &error[current_offset];
    for (int i = 0; i < K; i++){
      cur_error[i] += ngh_latent[i]*err;
    }
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d, intE edgeLen) {
    //not needed as we will always do pull based
    return update(s,d,edgeLen);
  }
  inline bool cond (intT d) { return cond_true(d); }};

struct CF_Vertex_F {
  double step, lambda;
  double *latent_curr, *error;
  int K;
  CF_Vertex_F(double _step, double _lambda, double* _latent_curr, double* _error, int _K) :
    step(_step), lambda(_lambda), latent_curr(_latent_curr), error(_error), K(_K) {}
  inline bool operator () (uintE i) {
    for (int j = 0; j < K; j++){
      latent_curr[K*i + j] += step*(-lambda*latent_curr[K*i + j] + error[K*i + j]);
      error[K*i+j] = 0.0;
    }
#ifdef COMPUTE_ERROR
    squaredErrors[i] = 0;
#endif
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  int K = P.getOptionIntValue("-K",20); //dimensions of the latent vector  
  int numIter = P.getOptionIntValue("-numiter",5); //number of iterations
  double step = P.getOptionDoubleValue("-step",0.00000035); //step size
  double lambda = P.getOptionDoubleValue("-lambda",0.001); //lambda
  bool randInit = P.getOption("-randInit"); //pass flag for random initialization of latent vector
  //initialize latent vectors and errors
  const intE n = GA.n;
  double* latent_curr = newA(double, K*n);
  double* error = newA(double, K*n);
#ifdef COMPUTE_ERROR
  squaredErrors = newA(double,n);
#endif
  if(randInit) { 
    srand(0);
    long seed = rand();
    parallel_for(uintE i = 0; i < n; i++){
#ifdef COMPUTE_ERROR
      squaredErrors[n] = 0;
#endif
      for (int j = 0; j < K; j++){
	latent_curr[i*K+j] = ((double)(seed+hashInt((uintE)i*K+j))/(double)UINT_E_MAX);
	error[i*K+j] = 0.0;
      }
    }
  } else {
    parallel_for(uintE i = 0; i < n; i++){
#ifdef COMPUTE_ERROR
      squaredErrors[n] = 0;
#endif
      for (int j = 0; j < K; j++){
	latent_curr[i*K+j] = 0.5; //default initial value of 0.5
	error[i*K+j] = 0.0;
      }
    }
  }

  bool* frontier = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) frontier[i] = 1;}
  vertexSubset Frontier(n,n,frontier);

  for (int iter = 0; iter < numIter; iter++){
    //edgemap to accumulate error for each node
    edgeMap(GA, Frontier, CF_Edge_F<vertex>(GA.V,latent_curr,error,K), 0, no_output);

#ifdef COMPUTE_ERROR
    cout << "sum of squared error: " << sequence::plusReduce(squaredErrors,n)/2 << " for iter: " << iter << endl;
#endif

    //vertexmap to update the latent vectors
    vertexMap(Frontier,CF_Vertex_F(step,lambda,latent_curr,error,K));

#ifdef DEBUG
    cout << "latent sum: " << sequence::plusReduce(latent_curr,K*n) << endl;
#endif
  }
  Frontier.del(); free(latent_curr); free(error);
#ifdef COMPUTE_ERROR
  free(squaredErrors);
#endif
}
