#ifndef ADSDATASRC_H
#define ADSDATASRC_H
#include "../datasrc.h"
#include <string>

class adsdatasrc : public DataSource {
private:
    void* _impl;
public:
    adsdatasrc(/* args */);
    ~adsdatasrc();

    virtual crow::json::wvalue getVariable(std::string symbolName);
};

#endif // ADSDATASRC_H
