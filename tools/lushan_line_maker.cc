#include "hdict.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <functional>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <vector>

using namespace std;

struct Comparator {
    bool operator()(const idx_t &o1,
                    const idx_t &o2) {
        return (o1.key < o2.key);
        }
} compar;

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stdout, "%s <file> <hdict_dir>\n", argv[0]);
		exit(0);
	}

	if (mkdir(argv[2], 0777) == -1) {
		if (errno == EEXIST) {
			fprintf(stderr, "Path ``%s'' exists\n", argv[2]);
		} else {
			fprintf(stderr, "mkdir() failed: %s, path=%s\n", strerror(errno), argv[2]);
		}
		exit(1);
	}

	FILE *fidx = NULL;
	FILE *fdata = NULL;

	ifstream in(argv[1]);
	if (!in) {
		fprintf(stderr, "fail to open %s\n", argv[1]);
		exit(1);
	}

	char path[256];
	snprintf(path, 256, "%s/idx", argv[2]);
	if ((fidx = fopen(path, "w")) == NULL) {
		fprintf(stderr, "fail to open %s\n", path);
		in.close();
		exit(1);
	}

	snprintf(path, 256, "%s/dat", argv[2]);
	if ((fdata = fopen(path, "w")) == NULL) {
		fprintf(stderr, "fail to open %s\n", path);
		in.close();
		fclose(fidx);
		exit(1);
	}

	idx_t idx;
	idx.key = 0;
	idx.pos = 0;

	off_t off = 0;
	uint64_t uid;
        int32_t count;

	int err = 0;
        uint64_t len;
        vector<idx_t> idx_v;

	string line;
	while(getline(in, line)) {
		size_t index = 0;
		if ((index = line.find_first_of("\t", 0)) == string::npos) {
			continue;
		}
		string key = line.substr(0, index);
		string value = line.substr(index + 1);
		value.erase(0, value.find_first_not_of("\r\t\n "));
		value.erase(value.find_last_not_of("\r\t\n ") + 1);

		uid = strtoull(key.c_str(), NULL, 10);
                len = value.length();
		if (fwrite(value.c_str(), 1, len, fdata) != len) {
			fprintf(stderr, "fail to write data file\n");
			err = 1;
			break;
		}
		idx.key = uid;
		idx.pos = (len<< 40) | off;
                idx_v.push_back(idx);
		off += len;
	}
        if (!err) {
            sort(idx_v.begin(), idx_v.end(), compar);
            vector<idx_t>::iterator it;
            for (it=idx_v.begin(); it != idx_v.end(); ++it) {
                idx = *it;
                if (fwrite(&idx, sizeof(idx_t), 1, fidx) != 1) {
                    fprintf(stderr, "fail to write idx file\n");
                    err = 1;
                    break;
                }
            }
        }

	in.close();
	fclose(fidx);
	fclose(fdata);

	exit(err);
}
