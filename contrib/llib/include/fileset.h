#ifndef FILESET_H
#define FILESET_H
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

/**
 * Files belong to the same directory and with same prefix.
 * Example: prefix.0, prefix.1 ...
 */
class FileSet {
public:
	FileSet(std::string &dir_path, std::string &prefix) : 
		dir_path_(dir_path), prefix_(prefix){}
	FileSet(const char *dir_path, const char *prefix) : 
		dir_path_(dir_path), prefix_(prefix){}
	virtual ~FileSet(){}
	int length() const { return filename_.size();}
	const std::string operator[](int i) const {
		std::string filepath = dir_path_ + "/";
		filepath += filename_[i];
		return filepath;
	}
protected:
	std::string dir_path_;
	std::string prefix_;
	std::vector<std::string> filename_;
};

class ReadFileSet : public FileSet {
public:
	ReadFileSet(std::string &dir_path, std::string &prefix) : 
		FileSet(dir_path, prefix){}
	ReadFileSet(const char *dir_path, const char *prefix) : 
		FileSet(dir_path, prefix){}

	// Upon successful completion, read_dir shall return a non-negative 
	// integer indicating the number of files in dir_path with prefix, 
	// otherwise, the funcation shall return -1
	int read_dir();
};

#endif
