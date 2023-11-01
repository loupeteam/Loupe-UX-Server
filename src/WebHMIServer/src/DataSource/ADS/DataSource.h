#ifndef ADSDATASRC_H
#define ADSDATASRC_H
#include "../datasrc.h"
#include <string>

class adsdatasrc : public DataSource {
private:
    void* _impl;
    void gatherBaseTypeNames(crow::json::rvalue&                   packet,
                             std::string&                          prefix,
                             std::vector<std::pair<std::string,
                                                   std::string> >& names);
    void gatherBaseTypeNames_Member(crow::json::rvalue&                   member,
                                    std::string                           prefix,
                                    std::vector<std::pair<std::string,
                                                          std::string> >& names);
public:
    adsdatasrc(/* args */);
    ~adsdatasrc();

    void readSymbolValue(std::string symbolName);
    void readSymbolValue(std::vector<std::string> symbolNames);
    void writeSymbolValue(std::string symbolName);
    void writeSymbolValue(crow::json::rvalue symbolNames);
    void readPlcData();
    bool ready();
    crow::json::wvalue getSymbolValue(std::string symbolName);
};

#endif // ADSDATASRC_H
