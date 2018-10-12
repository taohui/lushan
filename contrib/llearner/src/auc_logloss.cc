/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#include "utils.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <functional>
#include <algorithm>
#include <map>

using namespace std;

#define LINE_LENGTH_MAX 1024
#define SAMPLE_FEATURE_NUM_MAX 10000

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stdout, "%s <model> [-t|-b] [feature map]\n", argv[0]);
		exit(0);
	}

	char sep = '\t';
	if (argc == 3 && strcmp(argv[2], "-b") == 0) sep = ' ';

	map<uint64_t, double> weights;
	map<uint64_t, double>::iterator weights_it;
	if (read_lr_weights(argv[1], weights, NULL)) exit(1);

	map<uint64_t, string> code2name;
	bool have_codes = false;
	if (argc == 4) {
		map<string, uint64_t> name2code;
		if (read_lr_codes(argv[3], name2code)) exit(1);
		map<string, uint64_t>::iterator it;
		for (it = name2code.begin(); it != name2code.end(); ++it) {
			code2name.insert(pair<uint64_t, string>(it->second, it->first));
		}
		have_codes = true;
	}

	int total_weights = weights.size();

	vector<double> scores[2];

	uint64_t *features = (uint64_t *)malloc(SAMPLE_FEATURE_NUM_MAX * sizeof(uint64_t));

	double label;
	double score;
	double sum;
	char line[LINE_LENGTH_MAX];
	char line_copy[LINE_LENGTH_MAX];
	int count = 0;
	double loss = 0;
	while(fgets(line, LINE_LENGTH_MAX, stdin)) {
		strcpy(line_copy, line);
		int valid_features =  parse_lr_features(line, &label, features, SAMPLE_FEATURE_NUM_MAX, sep);
		sum = 0;
		for (int i=0; i<valid_features; i++) {
			if ((weights_it = weights.find(features[i])) != weights.end()) {
				sum += weights_it->second;
			}
		}
		score = sig(sum);

		if (label == 1) {
			loss += -log(score);
		} else {
			loss += -log(1 - score);
		}

		scores[(int)label].push_back(score);
		count++;
		if (count % 10000 == 0) {
			printf("%d\t%f", count, score);
			if (!have_codes) {
				printf("\t%s", line_copy);
			} else {
				map<uint64_t, string>::iterator it;
				for (int i=0; i<valid_features; i++) {
					if ((it = code2name.find(features[i])) != code2name.end()) {
						printf("\t%s", it->second.c_str());
					} else {
						printf("\t%d", features[i]);
					}
				}
			}
		}
	}

	double a = auc(scores);
	double l = loss / count;

	int pos = 0;
	int neg = 0;
	float pctr = 0;
	for(int i=0;i<scores[0].size();i++){
		pctr += scores[0][i];
		neg ++;
	}
	for(int i=0;i<scores[1].size();i++){
		pctr += scores[0][i];
		pos ++;
	}

	float tctr = pos*1.0/(pos + neg);
	pctr = pctr/(pos + neg);

	printf("auc [%f], logloss [%f], tctr [%f], pctr [%f], pctr/tctr [%f]\n", a, l, tctr, pctr, tctr == 0? 0 : pctr/tctr);

	free(features);
	exit(0);
}
