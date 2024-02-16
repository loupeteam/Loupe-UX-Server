#include <lux/ads/DataSource.h>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

int main(int argc, char const* argv[]){

    std::string localAmsNetId = "192.168.125.2.1.2";
    std::string ipV4Address ="192.168.4.54";
    std::string netID = "192.168.0.46.1.1";
    int port = 851;

    lux::adsdatasrc* dataSource = new lux::adsdatasrc();
    dataSource->setLocalAms(localAmsNetId);
    dataSource->setPlcCommunicationParameters(ipV4Address, netID, port);
    dataSource->readPlcData();
    while(1){
        vector<string> symbols = {"MAIN.MyFub"};
        dataSource->readSymbolValue(symbols);    
        dataSource->readSymbolValue("MAIN.MyFub");    
        cout << dataSource->getSymbolValue("MAIN.MyFub").dump() << "\n";
#ifdef _WIN32
        Sleep(1);
#else
        sleep(1);
#endif        
    }
    return 0;
}
