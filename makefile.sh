src_path="./src"

#!/bin/bash

# 检查是否为Ubuntu
if [[ -f /etc/os-release && $(grep -c -i "Ubuntu" /etc/os-release) -gt 0 ]]; then
    g++ $src_path/main.cpp  $src_path/NetplanConfigMaker.cpp  -o a.out   -I./deps/include -I./include -L./deps/libs/ubuntu  -lyaml-cpp   -std=c++11 -g
# 检查是否为CentOS
elif [[ -f /etc/os-release && $(grep -c -i "CentOS" /etc/os-release) -gt 0 ]]; then
    g++ $src_path/main.cpp  $src_path/NetplanConfigMaker.cpp  -o a.out   -I./deps/include -I./include -L./deps/libs/centos  -lyaml-cpp   -std=c++11 -g
else
    echo "Unknown Linux Release, Choose the Ubuntu version" 
    g++ $src_path/main.cpp  $src_path/NetplanConfigMaker.cpp  -o a.out   -I./deps/include -I./include -L./deps/libs/ubuntu  -lyaml-cpp   -std=c++11 -g
fi
