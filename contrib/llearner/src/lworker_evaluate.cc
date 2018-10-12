#include "lworker.h"
#include "utils.h"
#include "vlong.h"
#include "ps.h"
#include "samples.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <assert.h>
#include <pthread.h>
#include <stdexcept>

using namespace std;

double lr_evaluate(const SampleSplit& split, map<uint64_t, FeatureEntry>& dest, const LWorkerContext *cxt)
{
    double fx = 0.0;
    char *p = split.start;
    int left = split.end - split.start;
    for (int j = 0; j < split.sample_count; j++)
    {
        long len = 0;
        long label;
        read_vlong(&p, &left, &len);
        assert(len > 2);
        read_vlong(&p, &left, &label);

        long count;
        read_vlong(&p, &left, &count);

        long index;
        double sum = 0;
        char *p_copy = p;
        int left_copy = left;
        sum += dest[0].x;
        for (int k = 0; k < len - 2; k++)
        {
            read_vlong(&p, &left, &index);
            sum += dest[index].x;
        }

        double h = sig(sum);
        double error = h - label;

        dest[0].g += count * error;
        for (int k = 0; k < len - 2; k++)
        {
            read_vlong(&p_copy, &left_copy, &index);
            dest[index].g += count * error;
        }

        if (label == 0)
        {
            fx -= count * log(1.0 - h);
        }
        else
        {
            fx -= count * log(h);
        }
    }

    map<uint64_t, FeatureEntry>::iterator it;
    for(it = dest.begin(); it != dest.end(); ++it) {
        it->second.g /= split.sample_count;
    }
    fx /= split.sample_count;
    return fx;
}

void split_features(int dim_num, std::map<uint64_t, FeatureEntry>& whole_features, std::map<uint64_t, FeatureEntry>& linear_features, 
        std::map<uint64_t, std::vector<FeatureEntry> >& quad_features) {
    for (std::map<uint64_t, FeatureEntry>::const_iterator it = whole_features.begin(); it != whole_features.end(); ++it) {
        uint64_t key = it->first;
        FeatureEntry value = it->second;

        uint64_t id = key/LWORKER_FACTOR_NUM_MAX;;
        int idx = key - id*LWORKER_FACTOR_NUM_MAX;

        if (idx == 0) {
            if (linear_features.find(id) == linear_features.end()) {
                linear_features.insert(std::pair<uint64_t, FeatureEntry>(id, value));
            } else {
                fprintf(stderr, "duplicate features id %lu\n", id);
            }

        } else {
            if (quad_features.find(id) == quad_features.end()) {	
                std::vector<FeatureEntry> vec(dim_num, FeatureEntry());
                vec[idx-1] = value;
                quad_features.insert(std::pair<uint64_t, std::vector<FeatureEntry> >(id, vec));
            } else {
                quad_features[id][idx-1] = value;
            }
        }

	}
} 

// 预测函数，只预测score function， 顺便带出一些在下一步计算要用到的变量sum_map
double fm_predict(const int factor_num, const std::map<uint64_t, double>& sample, std::map<uint64_t, FeatureEntry>& linear_features,
        std::map<uint64_t, std::vector<FeatureEntry> >& quad_features, vector<double>& sum) {

	double result = 0;

	long bias_id = 0;

	result += linear_features[bias_id].x;

	for (std::map<uint64_t, double>::const_iterator it = sample.begin(); it != sample.end(); ++it) {
		uint64_t id = it->first;
		result += linear_features[id].x;     
	}

	for (int i = 0; i < factor_num; i++) {
		sum[i] = 0;
		double sum_sqr = 0;
		
		// 遍历样本维数
		for (std::map<uint64_t, double>::const_iterator it = sample.begin(); it != sample.end(); ++it) {
			uint64_t id = it->first;
			std::vector<FeatureEntry> vec = quad_features[id];
			sum[i] += vec[i].x;
			sum_sqr += vec[i].x*vec[i].x;
		}	
		result += 0.5*(sum[i]*sum[i] - sum_sqr);
	}

	return result;
}

double fm_evaluate(const SampleSplit& split,  map<uint64_t, FeatureEntry>& dest, const LWorkerContext *cxt)
{

	double loss = 0.0;
	char *p = split.start;
	int left = split.end - split.start;

	std::map<uint64_t, FeatureEntry> linear_features;
    std::map<uint64_t, std::vector<FeatureEntry> > quad_features;

    int factor_num = cxt->factor_num;

	split_features(factor_num, dest, linear_features, quad_features);


	for (int j = 0; j < split.sample_count; j++)
	{
		long count;
		long label;
		long len;
	
		read_vlong(&p, &left, &len);
		assert(len > 2);
		read_vlong(&p, &left, &label);
		read_vlong(&p, &left, &count);
		
		std::map<uint64_t, double> sample_map;
		long index;
		for (int i = 0; i < len -2; i++) {
			read_vlong(&p, &left, &index);
            sample_map.insert(std::pair<uint64_t, double>(index, 1.0));

		}
				
		vector<double> sums(factor_num, 0.0);
		double y_score = fm_predict(factor_num, sample_map, linear_features, quad_features, sums);
		
        double pred = sig(y_score);
        double error = pred - label;

		if (label <= 0.0) {
			loss -= log(1.0-pred)*count;
		} else {
			loss -= log(pred)*count;
		}
	
		dest[0].g += error * count;

		for(std::map<uint64_t, double>::const_iterator it = sample_map.begin(); it != sample_map.end(); ++it) {
			uint64_t id = it->first * LWORKER_FACTOR_NUM_MAX;
			dest[id].g += error * count;
		}

		for (int i = 0; i < factor_num; i++) {
			double tmp_sum = sums[i];
			for (std::map<uint64_t, double>::const_iterator it = sample_map.begin(); it != sample_map.end(); ++it) {
				uint64_t id = it->first * LWORKER_FACTOR_NUM_MAX + i + 1;
				double pre_weight = dest[id].x;
				double grad = tmp_sum - pre_weight;
				dest[id].g += error * grad * count;
			}
		}
	}

	int batch_size = split.sample_count;
	
	for (std::map<uint64_t, FeatureEntry>::iterator it = dest.begin(); it != dest.end(); ++it) {
		it->second.g /= batch_size;		
	}

	return loss/batch_size;
}
