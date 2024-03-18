#include <lux/ads/DataSource.h>
#include <lux/ads/AdsRpc.h>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std;

struct MethodParameters {
        uint32_t parameterA;
        uint32_t parameterB;
};
struct MethodReturn {
        uint32_t outputA;
        uint32_t outputB;
};
struct MethodOutputs {
        uint32_t result;
};


int main(int argc, char const* argv[]){

    std::string localAmsNetId = "192.168.125.2.1.2";
    std::string remoteAmsNetId = "192.168.125.2.1.3";
    std::string ipV4Address ="192.168.4.34";
    int port = 851;

    lux::adsdatasrc* dataSource = new lux::adsdatasrc();
    dataSource->setLocalAms(localAmsNetId);
    dataSource->setPlcCommunicationParameters(ipV4Address, remoteAmsNetId, port);

    dataSource->readPlcData();
/*
    ///RPC Test:
    MethodParameters parameters = {25, 10};
    MethodOutputs outputs;
    MethodReturn my_return;
    std::shared_ptr<AdsDevice>* route_ =  (std::shared_ptr<AdsDevice>*)dataSource->getRouter();
    AdsRpc<MethodReturn, MethodParameters, MethodOutputs> setCounter { *route_->get(), "RpcTest.RpcInterface#SetCounter"};

    my_return = setCounter.Invoke(&parameters, &outputs);
    cout << "outputA: " << my_return.outputA << "\n";
    cout << "outputB: " << my_return.outputB << "\n";
    cout << "Result: " << outputs.result << "\n";
*/

    while(1){
        vector<string> symbols = {"MAIN.PumpRun"};
        dataSource->readSymbolValue(symbols);    
        dataSource->readSymbolValue("MAIN.PumpRun");    
        cout << dataSource->getSymbolValue("MAIN.PumpRun").dump() << "\n";
#ifdef _WIN32
        Sleep(1);
#else
        sleep(1);
#endif        
    }
    return 0;
}
