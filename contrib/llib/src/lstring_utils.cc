#include "lstring_utils.h"

using namespace std;

vector<string> tokenize(const string& src, const string tok,
                           bool trim, const string null_subst)
{
    if (src.empty() || tok.empty())
        throw "tokenize: empty string";

    vector<string> v;
    size_t pre_index = 0, index = 0, len = 0;
    while ((index = src.find_first_of(tok, pre_index)) != string::npos) {
        if ((len = index - pre_index) != 0)
            v.push_back(src.substr(pre_index, len));
        else if (trim == false)
            v.push_back(null_subst);
        pre_index = index + 1;
    }
    string endstr = src.substr(pre_index);
    if (trim == false)
        v.push_back(endstr.empty()? null_subst : endstr);
    else if (!endstr.empty())
        v.push_back(endstr);

    return v;
}

