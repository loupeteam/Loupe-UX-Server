#ifndef ADSPARSER_IMPL_H
#define ADSPARSER_IMPL_H

#include <codecvt>
#include <string>
#include <vector>
#include "crow_all.h"
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
    std::unordered_map<std::string, symbolMetadata> _members;
public:
    std::string name;
    bool valid = false;
    bool cacheComplete = false;
    bool notFound = false;
    bool isArray = false;
    unsigned long group = 0;
    unsigned long gOffset = 0;
    unsigned long offset = 0;
    unsigned long size = 0;
    unsigned long flags = 0;
    unsigned long dataType;

    datatype_flags_struct flags_struct;

    std::string type;
    std::string comment;

    symbolMetadata(){}

    //Member count
    size_t memberCount() { return _members.size(); }
    //Get the members
    std::unordered_map<std::string, symbolMetadata>& members() { return _members; }

    //Override the [] operator to return a reference to the member
    symbolMetadata& operator[](const std::string& key)
    {
        //Split the path into a deque
        std::deque<std::string> path = splitVarName(key, std::string(".["));
        if (path.size() == 1) {
            return _members[key];
        } else {
            std::string member = path.front();
            path.pop_front();
            return _members[member][path];
        }
    }
    symbolMetadata& operator[](std::deque<std::string> path)
    {
        std::string key = path.front();
        if (path.size() == 1) {
            return _members[path[0]];
        } else {
            path.pop_front();
            return _members[key][path];
        }
    }
};

#endif // ADSPARSER_IMPL_H
