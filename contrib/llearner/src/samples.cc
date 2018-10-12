/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#include "samples.h"
#include "llog.h"
#include "utils.h"
#include "vlong.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

int SampleSplit::write_to_split(const vector<uint64_t>& tokens)
{
	char *p = end;
	int left = capacity_left();

	int count = tokens.size();
	if (write_vlong(&p, &left, count) == -1) {
		return -1;
	}

	vector<uint64_t>::const_iterator it;
	for (it = tokens.begin(); it != tokens.end(); ++it) {
		uint64_t i = *it;
		if (write_vlong(&p, &left, (long)i) == -1) {
			return -1;
		}
	}

	end = p;
	sample_count++;
	return 0;
}

int Samples::write_to_buffer(const vector<uint64_t>& tokens)
{
	bool should_retry = true;
	while (true) {
		if (buffers_.empty()) {
			SampleSplit new_split;
			SampleSplit::alloc_sample_split(&new_split);
			buffers_.push_back(new_split);
			should_retry = false;
		}

		SampleSplit& split = buffers_.back();
		if (split.write_to_split(tokens) == -1) {
			if (!should_retry) return -1;
			else split.shrink_to_fit();

			SampleSplit new_split;
			SampleSplit::alloc_sample_split(&new_split);
			buffers_.push_back(new_split);
			should_retry = false;
		} else {
			return 0;
		}
	}
	return 0;
}

int Samples::write_to_mini_batch(const vector<uint64_t>& tokens, bool new_mini_batch)
{
	if (new_mini_batch) {
		SampleSplit new_split;
		SampleSplit::alloc_sample_split(&new_split);
		buffers_.push_back(new_split);
	}
	SampleSplit& split = buffers_.back();
	if (split.write_to_split(tokens) == -1) {
		split.realloc_sample_split();
		if (split.write_to_split(tokens) == -1)
			return -1;
	} 
	return 0;
}

int Samples::load_sample(const vector<uint64_t>& tokens)
{
	if (tokens.size() > 2) {
		if (mini_batch_size_ == 0) {
			if (write_to_buffer(tokens)) {
				return -1;
			}
		} else {
			bool new_mini_batch = (sample_count_ % mini_batch_size_) == 0 ? true : false;
			if (write_to_mini_batch(tokens, new_mini_batch)) {
				return -1;
			}
		}
		sample_count_++;
		batch_size_ += tokens[1];
	} else {
		cerr << "invalid sample" << endl;
	}

	if (sample_count_ % 1000000 == 0) llog_write(NULL, LL_NOTICE, "sample count [%d]", sample_count_);
	return 0;
}

int Samples::load_sample(const string& line)
{
	vector<uint64_t> tokens;
	uint64_t new_total_weights = total_weights_;

	size_t last = 0;
	size_t index = line.find_first_of(" \t", last);
	int no = 0;
	bool err = false;
	int dup_count = 0;
	while (index != string::npos) {
		string token = line.substr(last, index - last);
		uint64_t i = strtoull(token.c_str(), NULL, 10);
		no++;
		if (no == 1) {
			if (i != 0 && i != 1) err = true;
			else tokens.push_back(i);
		} else if (no == 2 && with_count_) {
			tokens.push_back(i);
			dup_count = i;
		} else {
			if (no == 2) {
				tokens.push_back(1);
				dup_count = 1;
			}
			if (check_bound_ && i >= total_weights_max) err = true;
			else {
				tokens.push_back(i);
				if (i >= new_total_weights) {
					new_total_weights = i + 1;
				}
			}
		}

		last = index + 1;
		index = line.find_first_of(" \t", last);
	}
	if (index - last > 0) {
		string token = line.substr(last, index - last);
		uint64_t i = strtoull(token.c_str(), NULL, 10);
		no++;
		if (no == 1) {
			if (i != 0 && i != 1) err = true;
			else tokens.push_back(i);
		} else if (no == 2 && with_count_) {
			tokens.push_back(i);
			dup_count = i;
		} else {
			if (no == 2) {
				tokens.push_back(1);
				dup_count = 1;
			}
			if (check_bound_ && i >= total_weights_max) err = true;
			else {
				tokens.push_back(i);
				if (i >= new_total_weights) {
					new_total_weights = i + 1;
				}
			}
		}
	}

	if (!err && tokens.size() > 2) {
		if (mini_batch_size_ == 0) {
			if (write_to_buffer(tokens)) {
				return -1;
			}
		} else {
			bool new_mini_batch = (sample_count_ % mini_batch_size_) == 0 ? true : false;
			if (write_to_mini_batch(tokens, new_mini_batch)) {
				return -1;
			}
		}
		sample_count_++;
		batch_size_ += dup_count;
		if (tokens.size() != 2 && total_weights_ != new_total_weights) {
			total_weights_ = new_total_weights;
		}
	} else {
		cerr << "invalid sample: %s" << line << endl;
	}

	tokens.clear();
	//if (sample_count_ % 10000000 == 0) cout << "sample count [" << sample_count_ << "]" << endl;
	if (sample_count_ % 1000000 == 0) llog_write(NULL, LL_NOTICE, "sample count [%d]", sample_count_);
	return 0;
}

int Samples::load(const char *filepath)
{
	ifstream in(filepath);
	if (!in) {
		return -1;
	}

	string line;

	while(getline(in, line)) {
		if (load_sample(line)) {
			in.close();
			return -1;
		}
	}
	if (sample_count_ % 1000000 == 0) llog_write(NULL, LL_NOTICE, "sample count [%d]", sample_count_);

	in.close();
	return 0;
}

int Samples::load_fileset(const FileSet& fileset)
{
	for (int i = 0; i < fileset.length(); i++) {
		string filepath = fileset[i];
		if (load(filepath.c_str()) == -1) return -1;
	}

	return 0;
}

#ifdef TEST_SAMPLES
int main(int argc, char *argv[])
{
	if (argc != 2) {
		cout << argv[0] << " <samples file>" << endl;
		exit(0);
	}

	Samples samples;
	if (samples.load(argv[1]) == -1) {
		cerr << "failed to load " << argv[1] << endl;
		exit(1);
	}

	exit(0);
}
#endif
