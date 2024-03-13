#ifndef __NETPLAN_CONFIG_MAKER__
#define __NETPLAN_CONFIG_MAKER__
#include "yaml-cpp/yaml.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <iostream>
#include <string>
#include <bitset>
#include <cstdlib>
#include <arpa/inet.h>

//-------------Netplan Device Node------------------
class Netplan_Address{
public:
    std::string ip_mask;
    std::string label;
    static Netplan_Address* parseAddress(YAML::Node& node);
};

class Netplan_Route{
public:
    std::string dst_ip_mask;
    std::string via;
    static Netplan_Route* parseRoute(YAML::Node& node);
};

typedef enum Netplan_BondMode{
    _BALANCE_RR,
    _ACTIVE_BACKUP,
    _BALANCE_XOR,
    _BROADCAST,
    _802_3AD,
    _BALANCE_TLB,  
    _BALANCE_ALB,
    _KNOWN,
}Netplan_BondMode;

class Netplan_Device{
public:
    Netplan_Device();
    ~Netplan_Device();

public:
    // name
    bool setName(const std::string& name);
    std::string& getName();

    // set dhcp4
    bool setDhcp4(bool statu);
    bool getDhcp4();

    // address  ip, mask and label
    bool addAddress(Netplan_Address* address);
    bool addAddress(const std::string& ip, const std::string& mask);
    bool delAddress(const std::string& ip, const std::string& mask);
    bool delAddress(Netplan_Address* address);
    bool delAddressByIpMask(const std::string& ip_mask);

    // vip, ip, mask and label_name
    bool addVitualAddress(const std::string& ip, const std::string& mask);
    bool addVitualAddress(Netplan_Address* address);
    bool delVitualAddress(const std::string& ip, const std::string& mask);
    bool delVitualAddress(Netplan_Address* addresss);
    bool delVitualAddressByIpMask(const std::string& ip_mask);
    bool delVitualAddressByLabel(const std::string& label);

    // routes
    bool addRoute(const std::string& ip, const std::string& mask, const std::string& via);
    bool addRoute(Netplan_Route* route);
    bool addRoute(const std::string& dst_ip_mask, const std::string& via);
    bool delRoute(const std::string& ip, const std::string& mask, const std::string& via);
    bool delRoute(const std::string& dst_ip_mask, const std::string& via);
    bool delRoute(const std::string& dst_ip_mask);
    
    // name_server
    bool addNameServers(const std::string& nameserver);
    bool delNameServers(const std::string& nameserver);

    // generate yaml nodefparseYamlNode
    static bool parseYamlNode(YAML::Node& node, Netplan_Device* device, const std::string& name);
    virtual YAML::Node generateYamlNode();

    // some static fund
    static bool isValidIpv4(const std::string& ip);
    static bool isValidMask(const std::string& mask);
    static bool isValidMask(int mask);
    static bool isValidIpMask(const std::string& ip_mask);
    static bool isValidIpMask(const std::string&ip, const std::string& mask);
    static bool isValidIpMask(const std::string&ip, int mask);

private:
    int findAddressPosition(const std::string& ip, const std::string& mask);
    int findRoutePosition(const std::string& ip, const std::string& mask, const std::string& via);
    int getMaskCodeCount(const std::string& mask);
    std::string combineIpAndMask(const std::string& ip, const std::string& mask);
    
private:
    std::string _name;
    bool _dhcp4;
    std::vector<Netplan_Address*> _addresses;
    std::vector<Netplan_Route*> _routes;
    std::vector<std::string> _nameservers;
};

class Netplan_Ethernet : public Netplan_Device{
public:
    // generate yaml node
    YAML::Node generateYamlNode() override{
        return Netplan_Device::generateYamlNode();
    }
    static bool parseYamlNode(YAML::Node& node, Netplan_Ethernet* ethernet, const std::string& name);
};

class Netplan_Bond : public Netplan_Device{
public:
    Netplan_Bond();
    ~Netplan_Bond();

    YAML::Node generateYamlNode() override;
    static bool parseYamlNode(YAML::Node& node, Netplan_Bond* bond, const std::string& name);

    bool addInterface(const std::string& interface);
    bool delInterface(const std::string& interface);
    bool hasInterface(const std::string& interface);
    bool setMode(const std::string& mode_str);
    bool setMode(Netplan_BondMode mode);
    Netplan_BondMode getMode();
    std::string getModeStr();
    std::vector<std::string> getInterfacesList();

private:
    std::string Netplan_BondModeToString(Netplan_BondMode mode);
    Netplan_BondMode StringToNetplan_BondMode(const std::string& str);

private:
    std::vector<std::string> _interfaces;
    Netplan_BondMode _mode;
    std::unordered_map<std::string, Netplan_BondMode> _stringToBondMode = {
        {"balance-rr", _BALANCE_RR},
        {"active-backup", _ACTIVE_BACKUP},
        {"balance-xor", _BALANCE_XOR},
        {"broadcast", _BROADCAST},
        {"802.3ad", _802_3AD},
        {"balance-tlb", _BALANCE_TLB},
        {"balance-alb", _BALANCE_ALB},
        {"Unknown", _KNOWN}
    };
};

class Netplan_Bridge : public Netplan_Device{
public:
    YAML::Node generateYamlNode();
    static bool parseYamlNode(YAML::Node& node, Netplan_Bridge* bridge, const std::string& name);
};

class Netplan_Vlan : public Netplan_Device{
public:
    Netplan_Vlan();
    ~Netplan_Vlan();

    YAML::Node generateYamlNode() override;
    static bool parseYamlNode(YAML::Node& node, Netplan_Vlan* vlan, const std::string& name);

    int getId();
    bool setId(int id);
    std::string getLink();
    bool setLink(const std::string& link);

private:
    int _id;
    std::string _link;
};


// ----------------NetplanConfigMaker--------------------

class NetplanConfigMaker{
public:
    NetplanConfigMaker();
    ~NetplanConfigMaker();
    bool parseConfig(const std::string& config_path);
    bool generateConfig(const std::string& config_path);
    bool applyConfig(const std::string& config_path); 

    // set func
    bool setRenderer(const std::string& renderer);

    // change address
    bool addAddress(const std::string& dev_name, const std::string& ip, const std::string& mask);
    bool delAddress(const std::string& dev_name, const std::string& ip, const std::string& mask);

    // virtual address
    bool addVitualAddress(const std::string& dev_name, const std::string& ip, const std::string& mask);
    bool delVitualAddress(const std::string& dev_name, const std::string& ip, const std::string& mask);

    // routes
    bool addRoute(const std::string& dev_name, const std::string& ip, const std::string& mask, const std::string& via);
    bool delRoute(const std::string& dev_name, const std::string& ip, const std::string& mask, const std::string& via);

    // nameserver
    bool addNameServers(const std::string& dev_name, const std::string& nameserver);
    bool delNameServers(const std::string& dev_name, const std::string& nameserver);

    // setbond
    // ..............................

    // setvlan
    // ..............................

    // set bridge
    // ..............................


private:
    // parsing the info from the YAML node
    bool parseBasicInfo(YAML::Node& node);
    Netplan_Ethernet* parseEthernetsInfo(YAML::Node& node);
    Netplan_Bond* parseBondsInfo(YAML::Node& node);
    Netplan_Bridge* parseBridgesInfo(YAML::Node& node);
    Netplan_Vlan* parseVlansInfo(YAML::Node& node);

    // generate YAML node from the local struct
    YAML::Node generateEthernetsYamlNode();
    YAML::Node generateBondsYamlNode();
    YAML::Node generateBridgesYamlNode();
    YAML::Node generateVlanYamlNode();

    // find the pointer from the dev_name
    Netplan_Ethernet* isEthernet(const std::string& dev_name);
    Netplan_Bond* isBond(const std::string& dev_name);
    Netplan_Bridge* isBridge(const std::string& dev_name);
    Netplan_Vlan* isVlan(const std::string& dev_name);
private:
   int _version;
   std::string _renderer;
   std::vector<Netplan_Ethernet*> _ethernets;
   std::vector<Netplan_Bond*> _bonds;
   std::vector<Netplan_Bridge*> _bridges;
   std::vector<Netplan_Vlan*> _vlans;
};

static bool executeShellCommand(const std::string& command);

#endif //__NETPLAN_CONFIG_MAKER__