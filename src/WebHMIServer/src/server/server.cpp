#include "server.h"
#include "crow_all.h"
#include "jsonserver.h"
#include "util.h"

#include "../DataSource/ADS/DataSource.h"

#include <iostream>

using namespace std;

int main(int argc, char const* argv[])
{
    // Read configuration
    std::string configurationJsonString;
    crow::json::rvalue cfg;
    if (getFileContents("configuration.json", configurationJsonString) == 0) {
        cfg = crow::json::load(configurationJsonString);
    }
    else {
        cerr << "Could not open server configuration file";
        return -1;
    }

    if (cfg["serverType"].s() == "ADS") {
        adsdatasrc dataSource;
        dataSource.setPlcCommunicationParameters(cfg["adsParameters"]["netID"].s(), cfg["adsParameters"]["port"].i());
        dataSource.readPlcData();

        jsonserver server;
        server.addDataSource(dataSource);
        server.start(8000, false);
    }
    else {
        cerr << "Unsupported server type";
    }

    return 0;
}
