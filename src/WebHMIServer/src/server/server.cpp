#include "server.h"
#include "crow_all.h"
#include "jsonserver.h"

#include "../DataSource/ADS/adsdatasrc.h"

#include <iostream>

using namespace std;

int main(int argc, char const *argv[])
{

  adsdatasrc dataSource;
  dataSource.getGlobalSymbolInfo();
  dataSource.getDatatypeInfo();
  dataSource.getSymbolInfo( "Main.lift1.configuration");
  dataSource.readSymbolValue( "Main.lift1.configuration");


  jsonserver server;
  server.addDataSource( dataSource );
  server.start( 80 );

  return 0;
}
  

