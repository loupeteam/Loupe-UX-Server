#include "util.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <regex>

using namespace std;

//Converts string to lower in place
void toLower(string& str)
{
//  std::transform(str.begin(), str.end(), str.begin(),[](unsigned char c){ return std::tolower(c); });
}
//Returns an array of strings split by delim
std::deque<std::string> split(const string& str, char delim)
{
    std::deque<std::string> elems;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

//Split a string by multiple delimiters
std::deque<std::string> split(const std::string& str, std::string delims)
{
    std::deque<std::string> elems;
    string::size_type pos, prev = 0;
    while ((pos = str.find_first_of(delims, prev)) != string::npos) {
        if (pos > prev) {
//            if (1 == pos - prev) {break;}

            if ((prev > 0) && (str[prev - 1] == '[')) {
                prev--;
            }
            //If the character was [ lets keep that
            elems.emplace_back(str, prev, pos - prev);
        }
        prev = pos + 1;
    }
    if (str[prev - 1] == '[') {
        prev--;
    }
    if (prev < str.size()) {elems.emplace_back(str, prev, str.size() - prev);}
    return elems;
}
