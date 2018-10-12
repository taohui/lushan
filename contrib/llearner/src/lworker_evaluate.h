#ifndef LWORKER_EVALUATE_H
#define LWORKER_EVALUATE_H
#include "lworker.h"
#include "ps.h"
#include "samples.h"
#include <map>

double lr_evaluate(const SampleSplit& split, std::map<uint64_t, FeatureEntry>& dest, const LWorkerContext *cxt);

double fm_evaluate(const SampleSplit& split, std::map<uint64_t, FeatureEntry>& dest, const LWorkerContext *cxt);

#endif
