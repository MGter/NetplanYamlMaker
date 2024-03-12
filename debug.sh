#!/bin/bash

# 检查是否为Ubuntu
if [[ -f /etc/os-release && $(grep -c "Ubuntu" /etc/os-release) -gt 0 ]]; then
    export LD_LIBRARY_PATH=./deps/libs/ubuntu
# 检查是否为CentOS
elif [[ -f /etc/os-release && $(grep -c "CentOS" /etc/os-release) -gt 0 ]]; then
    export LD_LIBRARY_PATH=./deps/libs/centos
else
    echo "Unknown Linix Release, choose the CentOS ver"
    export LD_LIBRARY_PATH=./deps/libs/centos
fi

echo "ld_library_path = ${LD_LIBRARY_PATH}"

rm ./a.out -f
sh makefile.sh
gdb ./a.out
