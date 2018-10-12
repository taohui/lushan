#ifndef LSTRING_UTILS_H
#define LSTRING_UTILS_H

#include <string>
#include <vector>

inline const std::string rtrim(const std::string& str, const char *charlist = " \t\r\n")
{
    std::string s(str);
    std::string::size_type pos = s.find_last_not_of(charlist);
    if (pos == std::string::npos) {
        return "";
    } else {
        return s.erase(pos + 1);
    }
}

inline const std::string ltrim(const std::string& str, const char *charlist = " \t\r\n")
{
    std::string s(str);
    return s.erase(0, s.find_first_not_of(charlist));
}

inline const std::string trim(const std::string& str, const char *charlist = " \t\r\n")
{
    return ltrim(rtrim(str, charlist), charlist);
}

std::vector<std::string> tokenize(const std::string& src, const std::string tok,
                                     bool trim = false,
                                     const std::string null_subst = "");


#endif
