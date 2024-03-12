#include <iostream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include "NetplanConfigMaker.h"

int main()
{
   NetplanConfigMaker maker;

   // parse the yaml config and add all into the internal struct
   maker.parseConfig("/etc/netplan/netplan_generate.yaml");
   // generate another yaml config
   maker.generateConfig("new.yaml");
   maker.applyConfig("new.yaml");

   return 0;
}