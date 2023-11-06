#include "server.h"
#include "crow_all.h"
#include "jsonserver.h"
#include "util.h"

#include "../DataSource/ADS/DataSource.h"

#include <iostream>

using namespace std;

int main(int argc, char const* argv[])
{
    adsdatasrc dataSource;
    
    crow::json::rvalue cfg = crow::json::load(getFileContents("configuration.json"));
    dataSource.setPlcCommunicationParameters(cfg["adsParameters"]["netID"].s(), cfg["adsParameters"]["port"].i());
    dataSource.readPlcData();

    jsonserver server;
    server.addDataSource(dataSource);
    server.start(8000, false);
    return 0;
}
