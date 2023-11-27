#include "./adsdatasrc.h"

#include <iostream>
#include "adsdatasrc.h"

#define impl ((adsdatasrc_impl*)_impl)

using namespace std;

crow::json::wvalue getVariable(std::string symbolName)
{
    if (symbolName == "test.var") {
        //Create a crow json structure to return
        crow::json::wvalue x;
        x["Configuration"].empty_object();
        x["Configuration"]["Active"] = true;
        return x;
    } else {
        crow::json::wvalue x;
        return x;
    }
}
