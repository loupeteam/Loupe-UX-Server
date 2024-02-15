#ifndef ADSPARSER_IMPL_H
#define ADSPARSER_IMPL_H

#include <codecvt>
#include <string>
#include <vector>
#include "../datasrc.h"
#include "SymbolParser.h"
#include "util.h"

class dataType_member_base {
public:
    bool valid = false;
    bool isArray = false;
    bool isMember = false;
    bool baseType = false;
    std::string name;
    std::string type;
    unsigned long offset;
    unsigned long size;
    unsigned long datatype = 0;
    std::vector<dataType_member_base*> members;
    dataType_member_base(std::string name, std::string type, unsigned long offset,
                         unsigned long size)
        : name(name), type(type), offset(offset), size(size){}
    dataType_member_base(){}
    virtual bool parse(crow::json::wvalue& variable, void* buffer, unsigned long size){return false;}
    virtual bool encode(BYTE* buffer, std::string value, unsigned long size){return false;}
    static bool encode(unsigned long type, BYTE* buffer, std::string& value, unsigned long size);
    static bool parse(unsigned long type, crow::json::wvalue& variable, void* buffer, unsigned long size);
};

class symbolMetadata {
    std::unordered_map<std::string, std::shared_ptr<symbolMetadata>> _members;
public:
    std::string name;
    bool valid = false;
    bool cacheComplete = false;
    bool notFound = false;
    bool isArray = false;
    uint32_t group = 0;
    uint32_t gOffset = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t flags = 0;
    uint32_t dataType;
    uint32_t handle = 0;
    uint32_t readFail = 0;
    uint32_t writeFail = 0;
    datatype_flags_struct flags_struct;

    std::string type;
    std::string comment;

    symbolMetadata(){}

    //Member count
    size_t memberCount() { return _members.size(); }
    //Get the members
    std::unordered_map<std::string, std::shared_ptr<symbolMetadata>>& members() { return _members; }

    //Override the [] operator to return a reference to the member
    symbolMetadata& operator[](const std::string& key)
    {
        //Split the path into a deque
        std::deque<std::string> path = splitVarName(key, std::string(".["));
        if (path.size() == 1) {
            auto check = _members[key];
            if(check == nullptr)
                check = _members[key] = std::make_shared<symbolMetadata>();
            return *check;
        } else {
            std::string member = path.front();
            path.pop_front();
            auto check = _members[member];
            if(check == nullptr)
                check = _members[member] = std::make_shared<symbolMetadata>();            
            return (*check)[path];
        }
    }
    symbolMetadata& operator[](std::deque<std::string> path)
    {
        std::string key = path.front();
        if (path.size() == 1) {
            
            auto check = _members[path[0]];
            if(check == nullptr)
                check = _members[path[0]] = std::make_shared<symbolMetadata>();

            return *check;
        } else {
            path.pop_front();
            auto check = _members[key];
            if(check == nullptr)
                check = _members[key] = std::make_shared<symbolMetadata>();                
            return (*check)[path];
        }
    }
};

#endif // ADSPARSER_IMPL_H
