/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#ifndef UTILS_H
#define UTILS_H
#include <vector>
#include <map>
#include <string>
#include <vector>
#include <stdint.h>

double sig(double x);

struct WeightsMeta {
    int version;
    std::string type;
    int factor_num;
    WeightsMeta() : version(1), factor_num(0) {

    }
};

int write_lr_weights(const char *filepath, const double *weights, int total_weights, WeightsMeta *meta);
int read_lr_weights(const char *filepath, std::vector<double>& weights, WeightsMeta *meta);
int read_lr_weights(const char *filepath, std::map<uint64_t, double>& weights, WeightsMeta *meta);
int read_lr_codes(const char *filepath, std::map<std::string, int>& codes);
int read_lr_codes(const char *filepath, std::map<std::string, uint64_t>& codes);

int read_inherit_weights(const char *filepath, std::map<int, int>& inherit);

int parse_lr_features(char *p, double *label, int *features, int total_weights, char sep = '\t');
int parse_lr_features(char *p, double *label, uint64_t *features, int features_cap, char sep);

double auc(std::vector<double> *scores);

int read_ftrl_state(const char *input_state, double **weights, int *total_weights);
int write_ftrl_state(const char *output_state, double *weights, int total_weights);
#endif
