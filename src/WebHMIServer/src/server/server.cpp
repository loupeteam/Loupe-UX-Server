#include "server.h"
#include "crow_all.h"
#include "jsonserver.h"

#include "../DataSource/ADS/DataSource.h"

#include <iostream>

using namespace std;

int main(int argc, char const* argv[])
{
    adsdatasrc dataSource;

    NetIDType netId = {192, 168, 0, 46, 1, 1};
    dataSource.setPlcCommunicationParameters(netId, 851);
    dataSource.readPlcData();

    jsonserver server;
    server.addDataSource(dataSource);
    server.start(8000, false);
    return 0;
}
