#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "NetplanConfigMaker.h"

int main()
{
   NetplanConfigMaker maker;

   // parse the yaml config and add all into the internal struct
   maker.parseConfig("input.yaml");
   // generate another yaml config
   maker.addAddress("ens33", "192.165.56.84", "255.255.255.0");
   //maker.addVitualAddress("ens33", "192.165.56.17", "255.255.255.0");
   if(!maker.addAddress("bond0", "155.155.155.1", "255.255.255.0"))
      std::cout << "Failed to add the route to bond0" << std::endl;
   
   if(!maker.delAddress("ens33", "192.165.56.84", "255.255.255.0")){
      std::cout << "Failed to delete the route" << std::endl;
   }

   maker.generateConfig("output.yaml");
   //maker.applyConfig("new.yaml");

   return 0;
}