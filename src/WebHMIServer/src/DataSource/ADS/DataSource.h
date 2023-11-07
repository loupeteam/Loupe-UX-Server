#ifndef ADSDATASRC_H
#define ADSDATASRC_H
#include "../datasrc.h"
#include <string>

class adsdatasrc : public DataSource {
private:
    void* _impl;
    void gatherBaseTypeNames(crow::json::rvalue&                                packet,
                             std::string&                                       prefix,
                             std::vector<std::pair<std::string, std::string> >& names);
    void gatherBaseTypeNames_Member(crow::json::rvalue&                                member,
                                    std::string                                        prefix,
                                    std::vector<std::pair<std::string, std::string> >& names);

    void registerDUTChangeCallback();

public:
    unsigned long adsChangeHandle = 0;
    adsdatasrc(/* args */);
    ~adsdatasrc();

    void readSymbolValue(std::string symbolName);
    void readSymbolValue(std::vector<std::string> symbolNames);

    void getSymbolHandle(std::string symbolName);
    void getSymbolHandle(std::vector<std::string> symbolNames);

    void writeSymbolValue(std::string symbolName);
    void writeSymbolValue(crow::json::rvalue symbolNames);

    void connect();
    void readPlcData();
    bool ready();

    crow::json::wvalue getSymbolValue(std::string symbolName);
};

#endif // ADSDATASRC_H
