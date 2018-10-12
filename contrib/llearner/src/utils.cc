/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#include "utils.h"
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

double sig(double x)
{
    if (x > 100.0) {
        return 0.999999999;
    } else if (x < -100.0){
        return 0.000000001;
    } else {
        return 1.0 / (1 + exp(-x));
    }
}

static int write_lr_weights_meta(ofstream& out, WeightsMeta *meta)
{
	out << "version" << " : " << meta->version << endl;
	out << "type" << " : " << meta->type << endl;
	if (meta->type.compare("fm") == 0) {
		out << "factor_num" << " : " << meta->factor_num << endl;
	}
	return 0;
}
int write_lr_weights(const char *filepath, const double *weights, int total_weights, WeightsMeta *meta)
{
	ofstream out(filepath);
	if (!out) {
		return -1;
	}

	if (meta) {
		write_lr_weights_meta(out, meta);
	}

	for (int i=0; i<total_weights; i++) {
		if (weights[i]) out << i << " : " << weights[i] << endl;
	}
	out.close();
	return 0;
}

static int read_lr_weights_meta(ifstream& in, WeightsMeta *meta)
{
	string name;
	string sep;
	string value;
	if (!(in >> name >> sep >> value)) return -1;

	if (name.compare("version") == 0) {
		meta->version = atoi(value.c_str());
		if (meta->version != 1) return -1;
	} else {
		return -1;
	}

	if (!(in >> name >> sep >> value)) return -1;
	if (name.compare("type") == 0) {
		meta->type = value;
		if (meta->type.compare("fm") == 0) {
			if (!(in >> name >> sep >> value)) return -1;
			if (name.compare("factor_num") == 0) {
				meta->factor_num = atoi(value.c_str());
			} else {
				return -1;
			}
		} else {
			return -1;
		}
	} else {
		return -1;
	}
	return 0;
}

int read_lr_weights(const char *filepath, vector<double>& weights, WeightsMeta *meta)
{
	ifstream in(filepath);
	if (!in) {
		return -1;
	}

	if (meta) {
		if (read_lr_weights_meta(in, meta)) {
			in.close();
			return -1;
		}
	}

	int index;
	string sep;
	double weight;
	while (in >> index >> sep >> weight) {
		weights.resize(index+1, 0);
		weights[index] = weight;
	}

	in.close();
	return 0;
}

int read_lr_weights(const char *filepath, map<uint64_t, double>& weights, WeightsMeta *meta)
{
	ifstream in(filepath);
	if (!in) {
		return -1;
	}

	if (meta) {
		if (read_lr_weights_meta(in, meta)) {
			in.close();
			return -1;
		}
	}

	uint64_t index;
	string sep;
	double weight;
	while (in >> index >> sep >> weight) {
		weights.insert(pair<uint64_t, double>(index, weight));
	}

	in.close();
	return 0;
}

int read_lr_codes(const char *filepath, map<string, int>& codes)
{
    ifstream in(filepath);
    if (!in) {
        return -1; 
    }   

    int index;
    string name;
    while (in >> name >> index) {
		codes.insert(pair<string, int>(name, index));
    }

    in.close();
    return 0;
}

int read_lr_codes(const char *filepath, map<string, uint64_t>& codes)
{
    ifstream in(filepath);
    if (!in) {
        return -1; 
    }   

    int index;
    string name;
    while (in >> name >> index) {
		codes.insert(pair<string, uint64_t>(name, index));
    }

    in.close();
    return 0;
}

int read_inherit_weights(const char *filepath, map<int, int>& inherit)
{
    ifstream in(filepath);
    if (!in) {
        return -1; 
    }   

    int index;
    int another;
    while (in >> index >> another) {
		inherit.insert(pair<int, int>(index, another));
    }

    in.close();
    return 0;
}

int parse_lr_features(char *p, double *label, int *features, int total_weights, char sep)
{
    int valid_features = 0;
    features[valid_features++] = 0;

    int index;

    char *p1 = strchr(p, sep);
    if (p1 == NULL) return -1;
    *p1 = 0;

    if (strcmp(p, "0") == 0) *label = 0;
    else *label = 1;

    p = p1 + 1;
    while ((p1 = strchr(p, sep)) != NULL){
        *p1 = '\0';
        index = atoi(p);
        if (index > 0 && index < total_weights) {
            features[valid_features++] = index;
		}
        p = p1 + 1;
    }
    index = atoi(p);
    if (index > 0 && index < total_weights) {
        features[valid_features++] = index;
	}

    return valid_features;
}

int parse_lr_features(char *p, double *label, uint64_t *features, int features_cap, char sep)
{
    int valid_features = 0;
    features[valid_features++] = 0;

    uint64_t index;

    char *p1 = strchr(p, sep);
    if (p1 == NULL) return -1;
    *p1 = 0;

    if (strcmp(p, "0") == 0) *label = 0;
    else *label = 1;

    p = p1 + 1;
    while ((p1 = strchr(p, sep)) != NULL){
        *p1 = '\0';
        index = strtoull(p, NULL, 10);
        if (index > 0 && valid_features < features_cap) {
            features[valid_features++] = index;
		}
        p = p1 + 1;
    }
    index = strtoull(p, NULL, 10);
    if (index > 0 && valid_features < features_cap) {
        features[valid_features++] = index;
	}

    return valid_features;
}

struct Comparator {
	bool operator()(double o1, double o2) {
		return o1 < o2;
	}
} compar;

double auc(std::vector<double> *scores)
{
	sort(scores[0].begin(), scores[0].end(), compar);
	sort(scores[1].begin(), scores[1].end(), compar);

	int i0 = 0;
	int i1 = 0;
	int rank = 1;
	int n0 = scores[0].size();
	int n1 = scores[1].size();
	double rank_sum = 0.0;
	double tie_score;
	while (i0 < n0 && i1 < n1) {
		double v0 = scores[0][i0];
		double v1 = scores[1][i1];

		if (v0 < v1) {
			i0++;
			rank++;
		} else if (v0 > v1) {
			i1++;
			rank_sum += rank;
			rank++;
		} else {
			tie_score = v0;

			int k0 = 0;
			while (i0 < n0 && scores[0][i0] == tie_score) {
				k0++;
				i0++;
			}

			int k1 = 0;
			while (i1 < n1 && scores[1][i1] == tie_score) {
				k1++;
				i1++;
			}

			rank_sum += (rank + (k0 + k1 - 1)/2.0) * k1;
			rank += k0 + k1;
		}

	}

	if (i1 < n1) {
		rank_sum += (rank + (n1 - i1 - 1)/2.0) * (n1 - i1);
		rank += n1 - i1;
	}

	double a = (rank_sum/n1 - (n1+1)/2.0)/n0;
	return a;
}

int read_ftrl_state(const char *input_state, double **weights, int *total_weights)
{
	FILE *fp;
	if ((fp = fopen(input_state, "r")) == NULL) {
		return -1;
	}
	int size;
	if (fread(&size, sizeof(int), 1, fp) != 1) {
		fclose(fp);
		return -1;
	}
	if (*total_weights > size) {
		*weights = new double[*total_weights * 3];
	} else {
		*weights = new double[size * 3];
		*total_weights = size;
	}

	for (int k=0; k<3; k++) {
		if (fread(*weights + *total_weights * k, sizeof(double), size, fp) != size) {
			fclose(fp);
			delete[] weights;
			return -1;
		}
	}

	fclose(fp);
	return size;
}

int write_ftrl_state(const char *output_state, double *weights, int total_weights)
{
	FILE *fp;
	if ((fp = fopen(output_state, "w")) == NULL) {
		return -1;
	}
	
	if (fwrite(&total_weights, sizeof(int), 1, fp) != 1) {
		fclose(fp);
		return -1;
	}

	if (fwrite(weights, sizeof(double), total_weights*3, fp) != total_weights*3) {
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}
