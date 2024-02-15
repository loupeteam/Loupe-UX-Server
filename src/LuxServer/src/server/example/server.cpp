#include "server.h"
#include "crow.h"
#include "jsonserver.h"
#include "util.h"
#include <AdsLib.h>
#include "AdsVariable.h"
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
    } else {
        cerr << "Could not open server configuration file";
        return -1;
    }
    AmsNetId Addr;
    Addr.b[0] = 192;
    Addr.b[1] = 168;
    Addr.b[2] = 0;
    Addr.b[3] = 46;
    Addr.b[4] = 1;
    Addr.b[5] = 1;

    auto device = std::make_shared<AdsDevice>(AdsDevice{cfg["adsParameters"]["ipV4Address"].s(), Addr, 851});

    jsonserver server;

    if (cfg["serverType"].s() == "ADS") {
#ifdef _WIN32
        adsdatasrc* dataSource = new adsdatasrc();
        dataSource->setLocalAms(cfg["adsParameters"]["localAmsNetId"].s());
        dataSource->setRouter(&device);
        dataSource->readPlcData();

        server.addDataSource(dataSource);
#else
        cerr << "ADS is not supported on this platform";
        return 0;
#endif
    } else {
        cerr << "Unsupported server type";
        return 0;
    }

    server.start(8000, false);

    return 0;
}
