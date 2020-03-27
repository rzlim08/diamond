#ifndef _MULTIPROCESSING_H_
#define _MULTIPROCESSING_H_
// various tools related to the multiprocessing parallelization, to be moved elsewhere

#include <string>
#include <vector>

using std::vector;
using std::string;

vector<string> split(const string & str, const char delim);
string join(const vector<string> & tokens, const char delim);
string quote(const string & str);
string unquote(const string & str);
void copy(const string & src_file_name, const string & dst_file_name);
string join_path(const string & path_1, const string & path_2);

#endif
