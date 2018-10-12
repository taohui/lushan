#ifndef LWORKER_H
#define LWORKER_H
#include "samples.h"
#include <string>
#include <map>
#include <pthread.h>

#define LWORKER_THREAD_NUM_DEFAULT 4

#define LWORKER_MINI_BATCH_SIZE_DEFAULT 50
#define LWORKER_MAX_ITERATIONS_DEFAULT 6

// 最大只能是这个值减1
#define LWORKER_FACTOR_NUM_MAX 100

struct FeatureEntry {
    FeatureEntry() : x(0.0), g(0.0) {}
    double x;
    double g;
};

struct LWorkerContext;

typedef double evaluate_functor_t(const SampleSplit& split, std::map<uint64_t, FeatureEntry>& dest, const LWorkerContext *cxt);

struct LWorkerContext {
    Samples *samples;
    std::string host;
    std::string worker_id;
    int hmid;
    int iter;
    int max_iterations;
    int factor_num;
    double fx_sum;
    pthread_mutex_t mutex;
    evaluate_functor_t *evaluate;
    LWorkerContext() : samples(NULL), hmid(0), iter(0), max_iterations(0), factor_num(0), fx_sum(0.0) {
        pthread_mutex_init(&mutex, NULL);
    }
};

#endif
