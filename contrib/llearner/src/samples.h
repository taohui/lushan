/* Authors: */
/*     Tao Hui <taohui3@gmail.com> */
#ifndef SAMPLES_H
#define SAMPLES_H
#include "samples.h"
#include "fileset.h"
#include <vector>
#include <algorithm>
#include <string>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct SampleSplit {
	char *start;
	char *end;
	int sample_count;
	ssize_t buf_size;

	static void alloc_sample_split(SampleSplit *split) {
		split->end = split->start = new char[sample_split_buf_size];
		split->sample_count = 0;
		split->buf_size = sample_split_buf_size;
	}
	static void free_sample_split(SampleSplit *split) {
		delete[] split->start;
	}

	int change_size_to(ssize_t new_buf_size) {
		char *new_start = new char[new_buf_size];
		if (!new_start) {
			return -1;
		}

		memcpy(new_start, start, end - start);
		end = new_start + (end - start);
		start = new_start;
		buf_size = new_buf_size;
		return 0;
	}

	int realloc_sample_split() {
		ssize_t new_buf_size = buf_size * 2;
		return change_size_to(new_buf_size);
	}
	int shrink_to_fit() {
		ssize_t new_buf_size = end - start;
		return change_size_to(new_buf_size);
	}

	int write_to_split(const std::vector<uint64_t>& tokens);
private:
	static const int sample_split_buf_size = 32*1024;

	int capacity_left() {
		return buf_size - (end - start);
	}
};

class Samples {
public:
	// weights[0] is bias
	Samples() : sample_count_(0), total_weights_(1), batch_size_(0), 
		with_count_(true), mini_batch_size_(0), cur_mini_batch_(0), 
		check_bound_(true) {}
	~Samples() {
		std::vector<SampleSplit>::iterator it;
		for (it = buffers_.begin(); it != buffers_.end(); ++it) {
			SampleSplit::free_sample_split(&(*it));
		}
	}

	int load(const char *filepath);
	int load_fileset(const FileSet& fileset);

	uint64_t sample_count() { return sample_count_; }

	uint64_t batch_size() { return batch_size_; }

	std::vector<SampleSplit> split() {
		return buffers_;
	}

	int write_to_buffer(const std::vector<uint64_t>& tokens);
	int write_to_mini_batch(const std::vector<uint64_t>& tokens, bool new_mini_batch);

	int total_weights() { return total_weights_; }
	void next_mini_batch() { 
		cur_mini_batch_ = (cur_mini_batch_ + 1) % buffers_.size(); 
		if (cur_mini_batch_ == 0) {
			random_shuffle_splits();
		}
	}
	int cur_mini_batch() { return cur_mini_batch_; }
	int mini_batch_size() { return mini_batch_size_; }

	void set_with_count(bool with_count) {
		with_count_ = with_count;
	}
	void set_mini_batch_size(int mini_batch_size) {
		mini_batch_size_ = mini_batch_size;
	}

	void random_shuffle_splits() {
		std::random_shuffle(buffers_.begin(), buffers_.end());
	}

	void set_check_bound(bool check_bound) {
		check_bound_ = check_bound;
	}

	int load_sample(const std::string& line);
	int load_sample(const std::vector<uint64_t>& tokens);

private:
	std::vector<SampleSplit> buffers_;
	uint64_t sample_count_;
	uint64_t total_weights_;
	uint64_t batch_size_;
	bool with_count_;
	int mini_batch_size_;
	int cur_mini_batch_;
	bool check_bound_;
	static const uint64_t total_weights_max = 50000000;
};

#endif
