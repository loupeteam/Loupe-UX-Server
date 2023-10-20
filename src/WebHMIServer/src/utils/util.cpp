#include "util.h"
#include <algorithm>
#include <iostream>
#include <sstream>


using namespace std;

//Converts string to lower in place
void toLower( string &str){
  std::transform(str.begin(), str.end(), str.begin(),[](unsigned char c){ return std::tolower(c); });
}
//Returns an array of strings split by delim
std::vector<std::string> split( string &str, char delim){
  std::vector<std::string> elems;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

