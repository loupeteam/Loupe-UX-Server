#include <string>
#include <deque>
#include <vector>
#include <chrono>

//Converts string to lower in place
void toLower(std::string& str);

//Returns an array of strings split by delim
std::deque<std::string> split(const std::string& str, char delim);
std::deque<std::string> splitVarName(const std::string& str, std::string delims);
std::chrono::high_resolution_clock::time_point getTimestamp();
double measureTime(std::string name, std::chrono::high_resolution_clock::time_point start);
