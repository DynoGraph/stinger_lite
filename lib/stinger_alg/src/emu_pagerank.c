#include <stdio.h>

#include "pagerank.h"
#include "stinger_core/x86_full_empty.h"
#include "stinger_core/emu_xmalloc.h"

inline double * set_tmp_pr(double * tmp_pr_in, int64_t NV) {
  double * tmp_pr = NULL;
  if (tmp_pr_in) {
    tmp_pr = tmp_pr_in;
  } else {
    // TODO replace with malloc1dlong
    tmp_pr = (double *)xmw_malloc1d(NV);
  }
  return tmp_pr;
}

inline void unset_tmp_pr(double * tmp_pr, double * tmp_pr_in) {
  if (!tmp_pr_in)
    mw_free(tmp_pr);
}

int64_t
page_rank_directed(stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter) {
  return page_rank(S, NV, pr, tmp_pr_in, epsilon, dampingfactor, maxiter);
}

#define RECURSIVE_CILK_SPAWN(BEGIN, END, GRAIN, FUNC, ...)         \
do {                                                                \
    size_t low = BEGIN;                                             \
    size_t high = END;                                              \
                                                                    \
    for (;;) {                                                      \
        /* How many elements in my range? */                        \
        int64_t count = high - low;                                 \
                                                                    \
        /* Break out when my range is smaller than the grain size */\
        if (count <= GRAIN) break;                                  \
                                                                    \
        /* Divide the range in half */                              \
        /* Invariant: count >= 2 */                                 \
        int64_t mid = low + count / 2;                              \
                                                                    \
        /* Spawn a thread to deal with the lower half */            \
        cilk_spawn FUNC(low, mid, GRAIN, __VA_ARGS__);              \
                                                                    \
        low = mid;                                                  \
    }                                                               \
                                                                    \
    /* Recursive base case: call worker function */                 \
    FUNC ## _worker(low, high, __VA_ARGS__);                        \
} while (0)


static void calculate_pagerank_worker(size_t begin, size_t end, stinger_t * S, int64_t NV, double * pr, double * tmp_pr, double * pr_constant)
{
  double pr_constant_local = 0;
  for (uint64_t v = begin; v < end; v++) {
    tmp_pr[v] = 0;

    if (stinger_outdegree(S, v) == 0) {
      pr_constant_local += pr[v];
    } else {
      STINGER_FORALL_IN_EDGES_OF_VTX_BEGIN(S, v) {
        int64_t outdegree = stinger_outdegree (S, STINGER_EDGE_DEST);
        tmp_pr[v] += pr[STINGER_EDGE_DEST] / (outdegree ? outdegree : NV-1);
      } STINGER_FORALL_IN_EDGES_OF_VTX_END();
    }
  }
  // Perform partial reduction
  // Can't use an atomic or remote here because pr_constant is a double
  pr_constant_local += readfe((uint64_t*)&pr_constant);
  writeef((uint64_t*)&pr_constant, pr_constant_local);

}

static void calculate_pagerank(size_t begin, size_t end, size_t grain, stinger_t * S, int64_t NV, double * pr, double * tmp_pr, double * pr_constant)
{
  RECURSIVE_CILK_SPAWN(begin, end, grain,
     calculate_pagerank, S, NV, pr, tmp_pr, pr_constant);
}


static void calculate_delta_worker(size_t begin, size_t end, stinger_t * S, double * pr, double * tmp_pr, double * delta)
{
  double delta_local = 0;
  for (uint64_t v = begin; v < end; v++) {
    double mydelta = tmp_pr[v] - pr[v];
    if (mydelta < 0)
      mydelta = -mydelta;
    delta_local += mydelta;
  }
  // Perform partial reduction
  // Can't use an atomic or remote here because delta is a double
  delta_local += readfe((uint64_t*)&delta);
  writeef((uint64_t*)&delta, delta_local);
}

static void calculate_delta(size_t begin, size_t end, size_t grain, stinger_t * S, double * pr, double * tmp_pr, double * delta)
{
  RECURSIVE_CILK_SPAWN(begin, end, grain,
     calculate_delta, S, pr, tmp_pr, delta);
}


// NOTE: This only works on Undirected Graphs!
int64_t
page_rank (stinger_t * S, int64_t NV, double * pr, double * tmp_pr_in, double epsilon, double dampingfactor, int64_t maxiter)
{
  double * tmp_pr = set_tmp_pr(tmp_pr_in, NV);

  int64_t iter = maxiter;
  double delta = 1;
  int64_t iter_count = 0;

  while (delta > epsilon && iter > 0) {
    iter_count++;

    double pr_constant = 0.0;

    calculate_pagerank(0, NV, 32,
      S, NV, pr, tmp_pr, &pr_constant);

    // normalize
    stinger_parallel_for (uint64_t v = 0; v < NV; v++) {
      tmp_pr[v] = (tmp_pr[v] + pr_constant / NV) * dampingfactor + ((1-dampingfactor) / NV);
    }

    // calculate_delta
    delta = 0;
    calculate_delta(0, NV, 32,
      S, pr, tmp_pr, &delta);

    //LOG_I_A("delta : %20.15e", delta);

    // double_buffer
    stinger_parallel_for (uint64_t v = 0; v < NV; v++) {
      pr[v] = tmp_pr[v];
    }

    iter--;
  }

  LOG_I_A("PageRank iteration count : %ld", iter_count);

  unset_tmp_pr(tmp_pr,tmp_pr_in);
}
