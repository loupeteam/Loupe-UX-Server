
#include "../DataSource.h"

#include <iostream>
#ifdef WIN32
// #include <windows.h>
#include <AdsLib.h>
#else
#include <unistd.h>
#include <ads/AdsLib.h>
#endif

using namespace std;

int main(int argc, char const* argv[])
{
    adsdatasrc* dataSource = new adsdatasrc();
    dataSource->setPlcCommunicationParameters("192.168.0.46.1.1", 851);
    dataSource->readPlcData();
    AmsNetId localAmsNetId;
    localAmsNetId.b[0] = 192;
    localAmsNetId.b[1] = 168;
    localAmsNetId.b[2] = 0;
    localAmsNetId.b[3] = 46;
    localAmsNetId.b[4] = 1;
    localAmsNetId.b[5] = 1;
    bhf::ads::AddLocalRoute(localAmsNetId,  "192.168.0.46");

    localAmsNetId.b[5] +=1;
    bhf::ads::SetLocalAddress(localAmsNetId);


    bool run=true;
    while (1)
    {
        //Measure the time
        auto start = std::chrono::high_resolution_clock::now();
        //Read the value of the symbol
        crow::json::wvalue val = dataSource->getSymbolValue("MAIN.isFirstCycle");
        if(run){
            for(int i=0;i<5;i++){
                dataSource->readSymbolValue("MAIN.isFirstCycle");
                val = dataSource->getSymbolValue("MAIN.isFirstCycle");
            }
        }
        //Take time reading the value
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Total Time to read: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
        std::cout << "Per Time to read: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()/5 << "ms\n";
        run = false;

        cout << val.dump() << "\n";
        #ifdef WIN32
        Sleep(1000);
        #else
        sleep(1);
        #endif
    }
    
    return 0;
}
