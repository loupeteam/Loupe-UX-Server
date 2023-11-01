#include <string>
#include <deque>
#include <vector>
//Converts string to lower in place
void toLower(std::string& str);

//Returns an array of strings split by delim
std::deque<std::string> split(const std::string& str, char delim);
std::deque<std::string> split(const std::string& str, std::string delims);
