#include "util.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>

using namespace std;
namespace lux{

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
    std::deque<std::string> splitVarName(const std::string& str, std::string delims)
    {
        std::deque<std::string> elems;
        string::size_type pos, prev = 0;
        while ((pos = str.find_first_of(delims, prev)) != string::npos) {
            if (pos > prev) {
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

    //Get a timestamp to measure agains
    std::chrono::high_resolution_clock::time_point getTimestamp()
    {
        return std::chrono::high_resolution_clock::now();
    }

    double printTime(std::string                                    name,
                    std::chrono::high_resolution_clock::time_point start,
                    std::chrono::high_resolution_clock::time_point end)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << name << elapsed.count() / 1000.0 << " ms\n" << std::flush;
        return elapsed.count() / 1000.0;
    }

    //Measure the time elapased since timestamp
    double measureTime(std::string name, std::chrono::high_resolution_clock::time_point start)
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return elapsed.count() / 1000.0;
    }

    // Return the contents of the file as a string
    int getFileContents(std::string fileName, std::string& contents)
    {
        // Open the file for reading
        std::ifstream file(fileName);

        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            contents = buffer.str();
        } else {
            std::cerr << "Failed to open the configuration file." << std::endl;
            return -1;
        }

        return 0;
    }
}
