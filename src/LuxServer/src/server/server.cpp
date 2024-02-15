#include "server.h"
#include "crow.h"
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
    } else {
        cerr << "Could not open server configuration file";
        return -1;
    }

    jsonserver server;

    // Add data sources to the server
    //  Currently only supports ADS
    try{
        if (cfg["serverType"].s() == "ADS") {
            std::string localAmsNetId = "127.0.0.1.1.1";
            std::string ipV4Address;
            std::string netID;
            int port = 851;
            try{
                localAmsNetId = cfg["adsParameters"]["localAmsNetId"].s();
            }catch(std::exception e){
                cerr << "Could not read localAmsNetId from configuration file. Using default 127.0.0.1.1.1";
            }   
            try{
                ipV4Address = cfg["adsParameters"]["ipV4Address"].s();
                }catch(std::exception e){
                cerr << "Could not read target ipV4Address from configuration file";
                return 0;
            }
            try{
                netID = cfg["adsParameters"]["netID"].s();
            }catch(std::exception e){
                cerr << "Could not read netID from configuration file";
                return 0;
            }
            try{
                port = cfg["adsParameters"]["port"].i();
            }catch(std::exception e){
                cerr << "Could not read port from configuration file, using default port 851";                
            }

            adsdatasrc* dataSource = new adsdatasrc();
            dataSource->setLocalAms(localAmsNetId);
            dataSource->setPlcCommunicationParameters(ipV4Address, netID, port);
            dataSource->readPlcData();
            server.addDataSource(dataSource);
        } else {
            cerr << "Unsupported server type";
            return 0;
        }
    }catch(std::runtime_error e){
        cerr << "Could not setup data source, check configuration file for errors.\n";
        cerr << e.what();
        cerr << "\n";
        return 0;
    }
    catch(std::exception e){
        cerr << "Could not setup data source, check configuration file for errors.\n";
        cerr << e.what();
        cerr << "\n";
        return 0;
    }

    // Start the websocket server
    int port = 8000;
    try{
        port = cfg["webSocket"]["port"].i();
    }catch(std::exception e){
        cerr << "Could not read port from configuration file, using default port 8000";
    }
    server.start(port, false);

    return 0;
}
