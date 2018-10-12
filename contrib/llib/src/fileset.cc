#include "fileset.h"
#include <cstring>
#include <iostream>
#include <string>
#include <dirent.h>
#include <sys/types.h>

using namespace std;

int ReadFileSet::read_dir()
{
	DIR *dir;
	struct dirent *de;

	if ((dir = opendir(dir_path_.c_str())) == NULL) {
		return -1;
	}
	while ((de = readdir(dir)) != NULL) {
		if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;

		if(strncmp(de->d_name, prefix_.c_str(), prefix_.length()) == 0) {
			string path(de->d_name);
			filename_.push_back(path);
		}
	}
	closedir(dir);
	return filename_.size();
}

