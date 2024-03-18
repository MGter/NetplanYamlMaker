#include "NetplanConfigMaker.h"

//-------------Some basic struct parsing func------------------//
Netplan_Address* Netplan_Address::parseAddress(YAML::Node& node){
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }

    std::string ip_mask = "";
    std::string label = "";
    if(node.IsScalar()){
        ip_mask = node.as<std::string>();
        label = "";
    }
    else if(node.IsMap()){
        auto iter = node.begin();
        if(iter->first.IsScalar()){
            ip_mask = iter->first.as<std::string>();
        }
        if(ip_mask != "" && node[ip_mask].IsMap()){
            auto iter = node[ip_mask].begin();
            label = iter->second.as<std::string>();
        }
        
    }

    if(Netplan_Device::isValidIpMask(ip_mask)){
        Netplan_Address* address = new Netplan_Address;
        address->ip_mask = ip_mask;
        address->label = label;
        return address;
    }
    else{
        return nullptr;
    }
}

Netplan_Route* Netplan_Route::parseRoute(YAML::Node& node){
    std::cout << "get into parseRoute " << std::endl;
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }
    std::string dst_ip_mask = "";
    std::string via = "";

    if(node.IsMap()){
        auto iter = node.begin();
        dst_ip_mask = iter->second.as<std::string>();
        iter++;
        via = iter->second.as<std::string>();

        // make sure the form is right
        if((dst_ip_mask == "default" || Netplan_Device::isValidIpMask(dst_ip_mask)) && Netplan_Device::isValidIpv4(via)){
            Netplan_Route* route = new Netplan_Route;
            route->dst_ip_mask = dst_ip_mask;
            route->via = via;
            return route;
        }

    }

    return nullptr;
}

//-------------Netplan Device Node------------------//

Netplan_Device::Netplan_Device(){
    _name = "";
    _dhcp4 = false;
}

// clear two  pointer list
Netplan_Device::~Netplan_Device(){
    if(!_addresses.empty()){
        for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++){
            Netplan_Address* address = (*iter);
            delete address;
        }
    }
    _addresses.clear();

    if(!_routes.empty()){
        for(auto iter = _routes.begin(); iter != _routes.end(); iter++){
            Netplan_Route* route = (*iter);
            delete route;
        }
    }
    _routes.clear();
}

bool Netplan_Device::setName(const std::string& name){
    _name = name;
    return true;
}

std::string& Netplan_Device::getName(){
    return _name;
}

bool Netplan_Device::setDhcp4(bool statu){
    _dhcp4 = statu;
    return true;
}

bool Netplan_Device::getDhcp4(){
    return _dhcp4;
}

bool Netplan_Device::addAddress(const std::string& ip, const std::string& mask){
    // ip or mask has wrong form
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return false;
    }
    // allready exist
    if(findAddressPosition(ip, mask) != -1){
        return false;
    }
    Netplan_Address* address = new Netplan_Address;
    std::string ip_mask = combineIpAndMask(ip, mask);
    address->ip_mask = ip_mask;
    address->label = "";
    _addresses.push_back(address);
    return true;
}

bool Netplan_Device::addAddress(Netplan_Address* address){
    if(address == nullptr || !isValidIpMask(address->ip_mask)){
        return false;
    }
    
    _addresses.push_back(address);
    return true;
}

bool Netplan_Device::delAddress(const std::string& ip, const std::string& mask){
    // ip or mask has wrong form
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return false;
    }
    // can't find
    int position = findAddressPosition(ip, mask);
    if( position == -1){
        return false;
    }
    // delete and erase it from the list
    auto iter = _addresses.begin() + position;
    delete *iter;
    _addresses.erase(iter);
    return true;
}

bool Netplan_Device::delAddress(Netplan_Address* address){
    if(address == nullptr || _addresses.empty()){
        return false;
    }

    for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++){
        auto m_address = *iter;
        if(m_address->ip_mask == address->ip_mask){
            _addresses.erase(iter);
            iter--;
            return true;
        }
    }
    return false;

}

bool Netplan_Device::delAddressByIpMask(const std::string& ip_mask){
    if(!isValidIpMask(ip_mask)){
        return false;
    }

    for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++){
        Netplan_Address* address = *iter;
        if(address->ip_mask == ip_mask){
            _addresses.erase(iter);
            return true;
        }
        else{
            continue;
        }
    }
    return false;
}

bool Netplan_Device::addVitualAddress(const std::string& ip, const std::string& mask){
    // ip or mask has wrong form
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return false;
    }
    // allready exist
    if(findAddressPosition(ip, mask) != -1){
        return false;
    }
    Netplan_Address* address = new Netplan_Address;
    std::string ip_mask = combineIpAndMask(ip, mask);
    address->ip_mask = ip_mask;
    address->label = _name + ':' + std::to_string(_addresses.size() - 1);
    _addresses.push_back(address);
    return true;
}

bool Netplan_Device::addVitualAddress(Netplan_Address* address){
    if(address == nullptr || address->label == "" || !isValidIpMask(address->ip_mask)){
        return false;
    }
    _addresses.push_back(address);
    return true;
}

bool Netplan_Device::delVitualAddress(const std::string& ip, const std::string& mask){
    // ip or mask has wrong form
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return false;
    }
    // can't find
    int position = findAddressPosition(ip, mask);
    if( position == -1){
        return false;
    }
    // delete and erase it from the list
    auto iter = _addresses.begin() + position;
    delete *iter;
    _addresses.erase(iter);
    return true;
}

bool Netplan_Device::delVitualAddress(Netplan_Address* address){
    if(address == nullptr || address->label == "" || _addresses.empty()){
        return false;
    }

    // del it by two ways: ip_mask ir label
    if(delVitualAddressByIpMask(address->ip_mask) || delVitualAddressByLabel(address->label)){
        return true;
    }
    else{
        return false;
    }
}

bool Netplan_Device::delVitualAddressByIpMask(const std::string& ip_mask){
    if(_addresses.empty() || !isValidIpMask(ip_mask)){
        return false;
    }
    for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++){
        Netplan_Address* address = *iter;
        if(ip_mask == address->ip_mask){
            _addresses.erase(iter);
            return true;
        }
    }
    return false;
}

bool Netplan_Device::delVitualAddressByLabel(const std::string& label){
    if(_addresses.empty() || label == ""){
        return false;
    }
    for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++){
        Netplan_Address* address = *iter;
        if(label == address->label){
            _addresses.erase(iter);
            return true;
        }
    }
    return false;
}

// find the ip and mask in list, return the position in address list
int Netplan_Device::findAddressPosition(const std::string& ip, const std::string& mask){
    // ip or mask has wrong form
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return false;
    }

    std::string ip_mask = combineIpAndMask(ip, mask);
    // for loop to find it. 
    int position = 0;
    for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++, position++){
        if(ip_mask == (*iter)->ip_mask){
            return position;
        }
    }
    return -1;
}

bool Netplan_Device::addRoute(const std::string& ip, const std::string& mask, const std::string& via){
    if(ip == "default" && isValidIpv4(via)){
        ;
    }
    else if(!isValidIpv4(ip) || !isValidMask(mask) || !isValidIpv4(via) ){
        return false;
    }
    // can't find it, return false
    std::string dst_ip_mask;
    if(ip == "default"){
        dst_ip_mask = ip;
    }
    else{
        dst_ip_mask = combineIpAndMask(ip, mask);
    }
     
    if(findRoutePosition(ip, mask, via) == -1){
        Netplan_Route* route = new Netplan_Route;
        route->dst_ip_mask = dst_ip_mask;
        route->via = via;
        _routes.push_back(route);
        return true;
    }
    // find it, return true;
    else{
        return false;
    }
}

bool Netplan_Device::addRoute(Netplan_Route* route){
    if(route == nullptr){
        std::cout << "Get a null route ptr, so can;t add" << std::endl;
        return false;
    }

    std::string dst_ip_mask = route->dst_ip_mask;
    std::string via = route->via;
    if(!isValidIpv4(via)){
        std::cout << "Get a invalid via" << via << std::endl;
        return false;
    }
    else if(dst_ip_mask != "default" && !isValidIpMask(dst_ip_mask)){
        std::cout << "Get a invalid dst_ip_mask:" << dst_ip_mask << std::endl;
        return false;
    }

    _routes.push_back(route);
    return true;
}

bool Netplan_Device::addRoute(const std::string& dst_ip_mask, const std::string& via){
    if(dst_ip_mask != "default" && !isValidIpMask(dst_ip_mask)){
        return false;
    }
    else if(!isValidIpv4(via)){
        return false;
    }
    Netplan_Route* route = new Netplan_Route;
    route->dst_ip_mask = dst_ip_mask;
    route->via = via;
    _routes.push_back(route);
    return true;
}

bool Netplan_Device::delRoute(const std::string& ip, const std::string& mask, const std::string& via){
    if(_routes.empty()){
        return false;
    }
    else if(ip == "default" &&  isValidIpv4(via)){
        ;
    }
    else if(!isValidIpv4(ip) || !isValidMask(mask) || !isValidIpv4(via)){
        return false;
    }
    // can't find it, return false
    std::string dst_ip_mask;
    if(ip == "default"){
        dst_ip_mask = ip;
    }
    else{
        dst_ip_mask = combineIpAndMask(ip, mask);
    }

    int position = findRoutePosition(ip, mask, via);
    // find it, do delete
    if(position != -1){
        auto iter = _routes.begin() + position;
        delete *iter;
        _routes.erase(iter);
        return true;
    }
    // can't find it, return false
    else{
        return false;
    }
}

bool Netplan_Device::delRoute(const std::string& dst_ip_mask, const std::string& via){
    if(_routes.empty()){
        return false;
    }
    for(auto iter = _routes.begin(); iter != _routes.end(); iter++){
        Netplan_Route* route = *iter;
        if(dst_ip_mask == route->dst_ip_mask && via == route->via){
            _routes.erase(iter);
            return true;
        }
    }
    return false;
}

bool Netplan_Device::delRoute(const std::string& dst_ip_mask){
    if(_routes.empty()){
        return false;
    }
    for(auto iter = _routes.begin(); iter != _routes.end(); iter++){
        Netplan_Route* route = *iter;
        if(dst_ip_mask == route->dst_ip_mask){
            _routes.erase(iter);
            return true;
        }
    }
    return false;
}

// find the route position, can't find, return -1
int Netplan_Device::findRoutePosition(const std::string& ip, const std::string& mask, const std::string& via){
    if(ip == "default"){
        ;
    }
    else if(!isValidIpv4(ip) || !isValidMask(mask) || !isValidIpv4(via)){
        return false;
    }

    std::string dst_ip_mask;
    if(ip == "default"){
    }

    // for loop to find the position
    auto iter = _routes.begin();
    int position = 0;
    for(; iter != _routes.end(); iter++){
        // find the same dst ip and mask
        if((*iter)->dst_ip_mask == dst_ip_mask){
            return position;
        }
    }
    // don't find it
    if(iter == _routes.end()){
        return -1;
    }
    else{
        return -1;
    }
}

bool Netplan_Device::addNameServer(const std::string& nameserver){
    if(!isValidIpv4(nameserver)){
        return false;
    }
    auto iter = std::find(_nameservers.begin(), _nameservers.end(), nameserver);
    if(iter == _nameservers.end()){
        _nameservers.push_back(nameserver);
        return true;
    }
    else{
        return false;
    }
}

bool Netplan_Device::delNameServers(const std::string& nameserver){
    if(!isValidIpv4(nameserver)){
        return false;
    }
    auto iter = std::find(_nameservers.begin(), _nameservers.end(), nameserver);
    if(iter == _nameservers.end()){
        return false;
    }
    else{
        _nameservers.erase(iter);
        return true;
    }
}

bool Netplan_Device::isValidIpv4(const std::string& ip){
    int iRet = -1;
    // 将IP地址由“点分十进制”转换成 “二进制整数”
    struct in_addr s;
    iRet = inet_pton(AF_INET, ip.c_str(), &s); 
    return iRet;
}

bool Netplan_Device::isValidMask(const std::string& mask){
    int iRet = -1;
    
    // 将IP地址由“点分十进制”转换成 “二进制整数”
    struct in_addr s;
    iRet = inet_pton(AF_INET, mask.c_str(), &s); 
    
    // 转换成功返回1，说明是有效的IP地址
     if (iRet == 1) {
        // 从网络字节顺序转换为主机字节顺序
        unsigned int addr = ntohl(s.s_addr);
         
        // 转换为二进制字符串
        std::bitset<32> b((int)addr);
        std::string strMask = b.to_string();
        
        // 查找二进制字符串中的"01"，如果不存在，说明是有效的子网掩码
        return (strMask.find("01") == std::string::npos);
    }
    
    return false;
}

bool Netplan_Device::isValidMask(int mask){
    return (mask >= 0 && mask <= 32);
}

bool Netplan_Device::isValidIpMask(const std::string&ip, const std::string& mask){
    return(isValidIpv4(ip) && isValidMask(mask));
}

bool Netplan_Device::isValidIpMask(const std::string&ip, int mask){
    return(isValidIpv4(ip) && isValidMask(mask));
}

bool Netplan_Device::isValidIpMask(const std::string& ip_mask){
    if(ip_mask == "default"){
        return true;
    }
    char ip[16];
    int mask = 0;
    int get_param = 0;
    get_param = sscanf(ip_mask.c_str(), "%[^/]/%d", ip, &mask);
    std::string ip_str = ip;
    if(get_param == 2 && isValidIpv4(ip_str) && mask >=0 && mask <= 32){
        return true;
    }
    else{
        return false;
    }
}

// something wrong here
int Netplan_Device::getMaskCodeCount(const std::string& mask){
    int count = 0;
    int iRet = -1;
    // 将IP地址由“点分十进制”转换成 “二进制整数”
    struct in_addr s;
    iRet = inet_pton(AF_INET, mask.c_str(), &s); 
    
    // 转换成功返回1，说明是有效的IP地址
    if (iRet == 1) {
        // 从网络字节顺序转换为主机字节顺序
        unsigned int addr = ntohl(s.s_addr);
         
        // 转换为二进制字符串
        std::bitset<32> b((int)addr);
        std::string strMask = b.to_string();
        
        for(int i = 0; i < 32; i++){
            if(strMask[i] == '1'){
                count++;
            }
            else{
                return count;
            }
        }
        return count;
    }
    else{
        return -1;
    }
}

std::string Netplan_Device::combineIpAndMask(const std::string& ip, const std::string& mask){
    std::string ip_mask = "";
    if(!isValidIpv4(ip) || !isValidMask(mask)){
        return ip_mask;
    }

    int mask_count = getMaskCodeCount(mask);
    if(mask_count < 0){
        ip_mask = "";
    }
    else{
        ip_mask = ip + '/' + std::to_string(mask_count);
    }
    return ip_mask;
}

// genderate yaml node for netcard
YAML::Node Netplan_Device::generateYamlNode(){
    // for debug
    // std::cout << "Use the device generate" << std::endl;
    
    YAML::Node yaml_node;
    // dhcp4 node
    yaml_node["dhcp4"] = _dhcp4;

    //address list node
    if(!_addresses.empty()){
        auto addresses_yaml_node = yaml_node["addresses"];
        int i = 0;
        for(auto iter = _addresses.begin(); iter != _addresses.end(); iter++, i++){
            Netplan_Address* address = (*iter);
            YAML::Node address_yaml_node;       // address node
            if(address->label != ""){
                YAML::Node label_yaml_node;
                label_yaml_node["label"] = address->label;
                address_yaml_node[address->ip_mask.c_str()] = label_yaml_node;
            }
            else{
                address_yaml_node = address->ip_mask.c_str();
            }
            addresses_yaml_node.push_back(address_yaml_node);
        }
    }
    if(!_routes.empty()){
        auto routes_yaml_node = yaml_node["routes"];
        for(auto iter = _routes.begin(); iter != _routes.end(); iter++){
            Netplan_Route* route = (*iter);
            YAML::Node route_yaml_node;
            route_yaml_node["to"] = route->dst_ip_mask;
            route_yaml_node["via"] = route->via;
            routes_yaml_node.push_back(route_yaml_node);
        }
    }
    if(!_nameservers.empty()){
        auto address_yaml_node = yaml_node["nameservers"]["addresses"];
        for(auto iter: _nameservers){
            address_yaml_node.push_back(iter);
        }
    }

    return yaml_node;
}

// parse yaml node from node, and add into the struct
bool Netplan_Device::parseYamlNode(YAML::Node& node, Netplan_Device* device, const std::string& name){
    if(device == nullptr){
        return false;
    }

    device->setName(name);

    if(!node.IsDefined() || node.IsNull()){
        std::cout << "parse a null node" << std::endl;
        return false;
    }

    // set dhcp4
    if(node["dhcp4"].IsDefined() && !node["dhcp4"].IsNull()){
        device->setDhcp4(node["dhcp4"].as<bool>());
    }

    // set address
    YAML::Node addresses_yaml_node = node["addresses"];
    if(addresses_yaml_node.IsDefined() && !addresses_yaml_node.IsNull()){
        // addressse is a sequence
        if(addresses_yaml_node.IsSequence()){
            int size = addresses_yaml_node.size();
            for(int i = 0; i < size; i++){
                YAML::Node address_yaml_node = addresses_yaml_node[i];
                Netplan_Address* address = Netplan_Address::parseAddress(address_yaml_node);
                if(address != nullptr){
                    device->addAddress(address);
                }
                else{
                    std::cout << "get a null address ptr" << std::endl;
                }
            }
        }
        else{
            std::cout << "address is not a sequence" << std::endl;
        }
    }

    // set routes
    YAML::Node routes_yaml_node = node["routes"];
    if(routes_yaml_node && !routes_yaml_node.IsNull()){
        // addressse is a sequence
        if(routes_yaml_node.IsSequence()){
            int size = routes_yaml_node.size();
            for(int i = 0; i < size; i++){
                YAML::Node route_yaml_node = routes_yaml_node[i];
                Netplan_Route* route = Netplan_Route::parseRoute(route_yaml_node);
                if(route != nullptr){
                    if(!device->addRoute(route)){
                        std::cout << "Failed the add the route" << std::endl;
                    }
                }
                else{
                    std::cout << "get a null address ptr" << std::endl;
                }
            }
        }
        else{
            std::cout << "address is not a sequence" << std::endl;
        }
    }

    // set nameservers
    YAML::Node nameservers_yaml_node = node["nameservers"];
    if(nameservers_yaml_node.IsDefined() && !nameservers_yaml_node.IsNull()){
        static int i = 0;
        if(nameservers_yaml_node.IsMap()){
            auto address_yaml_node = nameservers_yaml_node["addresses"];
            if(address_yaml_node.IsDefined() && !address_yaml_node.IsNull()){
                if(address_yaml_node.IsSequence()){
                    int size = address_yaml_node.size();
                    for(int i = 0; i < size; i++){
                        std::string nameserver_address = address_yaml_node[i].as<std::string>();
                        if(isValidIpv4(nameserver_address)){
                            device->addNameServer(nameserver_address);
                        }
                    }
                }
                else if(address_yaml_node.IsScalar()){
                    std::string nameserver_address = address_yaml_node.as<std::string>();
                    if(isValidIpv4(nameserver_address)){
                        device->addNameServer(nameserver_address);
                    }
                }
            }
        }
    }

    return true;
}

// ----------------Netplan Ethernets--------------------//
// ----------------for derived device--------------------//
bool Netplan_Ethernet::parseYamlNode(YAML::Node& node, Netplan_Ethernet* ethernet, const std::string& name){
    if(ethernet == nullptr){
        return false;
    }

    if(!node.IsDefined() || node.IsNull()){
        return false;
    }

    Netplan_Device* ethernet_to_device =  (Netplan_Device*)ethernet;

    return Netplan_Device::parseYamlNode(node, ethernet_to_device, name);

}


// ----------------Netplan Bonds--------------------//
Netplan_Bond::Netplan_Bond(){
    // bond mod default set as 802.3ad 
    _mode = Netplan_BondMode::_802_3AD;
}

Netplan_Bond::~Netplan_Bond(){
    ;
}

bool Netplan_Bond::parseYamlNode(YAML::Node& node, Netplan_Bond* bond, const std::string& name){
    if(bond == nullptr){
        return false;
    }

    if(!node.IsDefined() || node.IsNull()){
        return false;
    }

    if(node["mode"] && node["mode"].IsScalar()){
        std::string mode_str = node["mode"].as<std::string>();
        if(!bond->setMode(mode_str)){
            std::cout << "Failed to set the bond mode";
        }
    }
    else{
        std::cout << "Don't find the bond mode" << std::endl;
    }

    if(node["interfaces"] && node["interfaces"].IsSequence()){
        YAML::Node interfaces_yaml_node = node["interfaces"];
        int size = interfaces_yaml_node.size();
        for(int i = 0; i < size; i++){
            if(interfaces_yaml_node[i] && interfaces_yaml_node[i].IsScalar()){
                std::string interface_str = interfaces_yaml_node[i].as<std::string>();
                if(!bond->addInterface(interface_str)){
                    std::cout << "Failed the add the interface from the config" << std::endl;
                }
            }
        }
    }
    else{
        std::cout << "Failed the find the interfaces sequences" << std::endl;
    }

    Netplan_Device* bond_to_device =  (Netplan_Device*)bond;

    return Netplan_Device::parseYamlNode(node, bond_to_device, name);

}

YAML::Node Netplan_Bond::generateYamlNode(){
    YAML::Node node = Netplan_Device::generateYamlNode();
    if(_mode != Netplan_BondMode::_KNOWN){
        std::string mode_str = getModeStr();
        node["mode"] = mode_str;
    }

    if(!_interfaces.empty()){
        for(auto interface : _interfaces){
            node["interfaces"].push_back(interface);
        }
    }
    return node;
}

bool Netplan_Bond::addInterface(const std::string& interface){
    if(interface == ""){
        return false;
    }
    _interfaces.push_back(interface);
    return true;
}

bool Netplan_Bond::delInterface(const std::string& interface){
    if(interface == "" || _interfaces.empty()){
        return false;
    }

    auto iter = _interfaces.begin();
    for(; iter != _interfaces.end(); iter++){
        std::string m_interface = *iter;
        if(m_interface == interface){
            _interfaces.erase(iter);
            return true;
        }
    }
    return false;
}

bool Netplan_Bond::hasInterface(const std::string& interface){
    if(interface == "" || _interfaces.empty()){
        return false;
    }

    auto iter = _interfaces.begin();
    for(; iter != _interfaces.end(); iter++){
        std::string m_interface = *iter;
        if(m_interface == interface)
            return true;
    }
    return false;
}

bool Netplan_Bond::setMode(const std::string& mode_str){
    Netplan_BondMode mode = StringToNetplan_BondMode(mode_str);
    if(mode == Netplan_BondMode::_KNOWN){
        return false;
    }
    else{
        _mode = mode;
        return true;
    }
}

bool Netplan_Bond::setMode(Netplan_BondMode mode){
    if(mode == Netplan_BondMode::_KNOWN){
        return false;
    }
    else{
        return true;
    }
}

Netplan_BondMode Netplan_Bond::getMode(){
    return _mode;
}

std::string Netplan_Bond::getModeStr(){
    std::string mode_str = Netplan_BondModeToString(_mode);
    return mode_str;
}

std::string Netplan_Bond::Netplan_BondModeToString(Netplan_BondMode mode) {
    switch (mode) {
        case _BALANCE_RR:
            return "balance-rr";
        case _ACTIVE_BACKUP:
            return "active-backup";
        case _BALANCE_XOR:
            return "balance-xor";
        case _BROADCAST:
            return "broadcast";
        case _802_3AD:
            return "802.3ad";
        case _BALANCE_TLB:
            return "balance-tlb";
        case _BALANCE_ALB:
            return "balance-alb";
        default:
            return "Unknown";
    }
}

Netplan_BondMode Netplan_Bond::StringToNetplan_BondMode(const std::string& str) {
    auto it = _stringToBondMode.find(str);
    if (it != _stringToBondMode.end()) {
        return it->second;
    } else {
        return _KNOWN;  // 或者返回一个适当的默认值
    }
}

std::vector<std::string> Netplan_Bond::getInterfacesList(){
    return _interfaces;
}

// ----------------Netplan Bridges--------------------//
// still writting
bool Netplan_Bridge::parseYamlNode(YAML::Node& node, Netplan_Bridge* bridge, const std::string& name){
    if(bridge == nullptr){
        return false;
    }

    if(!node.IsDefined() || node.IsNull()){
        return false;
    }

    Netplan_Device* bridge_to_device =  (Netplan_Device*)bridge;

    return Netplan_Device::parseYamlNode(node, bridge_to_device, name);

}

YAML::Node Netplan_Bridge::generateYamlNode(){
    ;
}

// ----------------Netplan Vlans--------------------//
Netplan_Vlan::Netplan_Vlan(){
    _id = -1;
    _link = "";
}

Netplan_Vlan::~Netplan_Vlan(){
    ;
}

YAML::Node Netplan_Vlan::generateYamlNode(){
    YAML::Node node =  Netplan_Device::generateYamlNode();
    node["link"] = _link;
    node["id"] = _id;
    return node;
}

bool Netplan_Vlan::parseYamlNode(YAML::Node& node, Netplan_Vlan* vlan, const std::string& name){
    if(vlan == nullptr){
        return false;
    }

    if(!node.IsDefined() || node.IsNull()){
        return false;
    }

    if(node["id"] && node["id"].IsScalar()){
        int id = node["id"].as<int>();
        if(!vlan->setId(id)){ 
            std::cout << "Failed to set the vlan id" << std::endl;
        }
    }
    else{
        std::cout << "Failed to find the vlan id" << std::endl;
    }

    if(node["link"] && node["link"].IsScalar()){
        std::string link = node["link"].as<std::string>();
        if(!vlan->setLink(link)){
            std::cout << "Failed to set the link" << std::endl;
        }
    }
    else{
        std::cout << "Failed to find the link" << std::endl;
    }

    Netplan_Device* vlan_to_device =  (Netplan_Device*)vlan;

    return Netplan_Device::parseYamlNode(node, vlan_to_device, name);

}

std::string Netplan_Vlan::getLink(){
    return _link;
}

bool Netplan_Vlan::setLink(const std::string& link){
    if(link != ""){
        _link = link;
        return true;
    }
    else {
        return false;
    }
}

int Netplan_Vlan::getId(){
    return _id;
}

bool Netplan_Vlan::setId(int id){
    if(id>=0 && id<= 4096){
        _id = id;
        return true;
    }
    else{
        std::cout << "vlan id is out of range" << std::endl;
        return false;
    }
}

// ----------------NetplanConfigMaker--------------------//

NetplanConfigMaker::NetplanConfigMaker(){
    _version = 2;
    _renderer = "networkd";
}

NetplanConfigMaker::~NetplanConfigMaker(){
    _renderer = "";
    for(auto iter = _ethernets.begin(); iter != _ethernets.end(); iter++){
        delete *iter;
    }
    for(auto iter = _bonds.begin(); iter != _bonds.end(); iter++){
        delete *iter;
    }
    for(auto iter = _bridges.begin(); iter != _bridges.end(); iter++){
        delete *iter;
    }
    for(auto iter = _vlans.begin(); iter != _vlans.end(); iter++){
        delete *iter;
    }

    _ethernets.clear();
    _bonds.clear();
    _bridges.clear();
    _vlans.clear();
}

bool NetplanConfigMaker::parseConfig(const std::string& config_path){
    YAML::Node config = YAML::LoadFile(config_path.c_str());
    if(!config){
        return false;
        // for debug
        std::cout << "Failed to parse the config" << std::endl;
    }
    if(!config["network"].IsDefined() || config["network"].IsNull()){
        return false;
    }
    auto network_yaml_node = config["network"];

    // parse version, when can't find, default: 2
    if(network_yaml_node["version"].IsDefined() && !network_yaml_node["version"].IsNull())
        _version = network_yaml_node["version"].as<int>();
    else{
        std::cout << "cant't find the version" << std::endl;
    }

    // parse renderer, when can't find, default: netwokrd
    if(network_yaml_node["renderer"].IsDefined() && !network_yaml_node["renderer"].IsNull()){
        std::string render = network_yaml_node["renderer"].as<std::string>();
        if(render != "networkd" || render != "NetworkManager"){
            _renderer = "networkd";
        }
    }
    else{
        std::cout << "Can't find the renderer, default: networkd" << std::endl;
    }

   // parse ethernets
    if(network_yaml_node["ethernets"].IsDefined() && !network_yaml_node["ethernets"].IsNull()){
        auto ethernets_yaml_node = network_yaml_node["ethernets"];
        if(ethernets_yaml_node.IsMap()){
            for(auto iter = ethernets_yaml_node.begin(); iter != ethernets_yaml_node.end(); iter++){
                static int i = 0;
                std::cout << "It's the " << ++i << " times" << std::endl;

                std::string name = iter->first.as<std::string>();
                YAML::Node ethernet_yaml_node = ethernets_yaml_node[name];

                Netplan_Ethernet* ethernet = new Netplan_Ethernet;
                if(!Netplan_Ethernet::parseYamlNode(ethernet_yaml_node, ethernet, name)){
                    delete ethernet;
                }
                else{
                    _ethernets.push_back(ethernet);
                }
            }
        }
    }

    // bond
    if(network_yaml_node["bonds"].IsDefined() && !network_yaml_node["bonds"].IsNull()){
        auto bonds_yaml_node = network_yaml_node["bonds"];
        if(bonds_yaml_node.IsMap()){
            for(auto iter = bonds_yaml_node.begin(); iter != bonds_yaml_node.end(); iter++){
                static int i = 0;
                std::cout << "It's the " << ++i << " times" << std::endl;

                std::string name = iter->first.as<std::string>();
                YAML::Node bond_yaml_node = bonds_yaml_node[name];

                Netplan_Bond* bond = new Netplan_Bond;
                if(!Netplan_Bond::parseYamlNode(bond_yaml_node, bond, name)){
                    delete bond;
                }
                else{
                    _bonds.push_back(bond);
                }
            }
        }
    }

    // bridge
    if(network_yaml_node["bridges"].IsDefined() && !network_yaml_node["bridges"].IsNull()){
        auto bridges_yaml_node = network_yaml_node["bridges"];
        if(bridges_yaml_node.IsMap()){
            for(auto iter = bridges_yaml_node.begin(); iter != bridges_yaml_node.end(); iter++){
                static int i = 0;
                std::cout << "It's the " << ++i << " times" << std::endl;

                std::string name = iter->first.as<std::string>();
                YAML::Node bridge_yaml_node = bridges_yaml_node[name];

                Netplan_Bridge* bridge = new Netplan_Bridge;
                if(!Netplan_Bridge::parseYamlNode(bridge_yaml_node, bridge, name)){
                    delete bridge;
                }
                else{
                    _bridges.push_back(bridge);
                }
            }
        }
    }

    // vlan
    if(network_yaml_node["vlans"].IsDefined() && !network_yaml_node["vlans"].IsNull()){
        auto vlans_yaml_node = network_yaml_node["vlans"];
        if(vlans_yaml_node.IsMap()){
            for(auto iter = vlans_yaml_node.begin(); iter != vlans_yaml_node.end(); iter++){
                static int i = 0;
                std::cout << "It's the " << ++i << " times" << std::endl;

                std::string name = iter->first.as<std::string>();
                YAML::Node vlan_yaml_node = vlans_yaml_node[name];

                Netplan_Vlan* vlan = new Netplan_Vlan;
                if(!Netplan_Vlan::parseYamlNode(vlan_yaml_node, vlan, name)){
                    delete vlan;
                }
                else{
                    _vlans.push_back(vlan);
                }
            }
        }
    }

    return true;
}

bool NetplanConfigMaker::generateConfig(const std::string& config_path){
    std::ofstream output_file(config_path.c_str());

    YAML::Node yaml_node;
    auto network_yaml_node = yaml_node["network"];
    network_yaml_node["version"] = _version;
    network_yaml_node["renderer"] = _renderer;
    
    // for debuging
    /*
    Netplan_Ethernet* ethernet = new Netplan_Ethernet;
    ethernet->setName("eth0");
    _ethernets.push_back(ethernet); 
    */

    // generate the device node
    if(!_ethernets.empty()){
        network_yaml_node["ethernets"] = generateEthernetsYamlNode();
    }
    if(!_bonds.empty()){
        network_yaml_node["bonds"] = generateBondsYamlNode();
    }
    if(!_bridges.empty()){
        network_yaml_node["bridges"] = generateBridgesYamlNode();
    }
    if(!_vlans.empty()){
        network_yaml_node["vlans"] = generateVlanYamlNode();
    }

    output_file << yaml_node ;
    output_file.close();
    return true;
}

bool NetplanConfigMaker::applyConfig(const std::string& config_path){
    std::string base_path = "/etc/netplan";
    std::string backup_file = "/etc/netplan/backup";
    
    // backup the origin file
    executeShellCommand("mkdir /etc/netplan/backup");
    executeShellCommand("mv /etc/netplan/*yaml " + backup_file);

    // mv the new file
    executeShellCommand("cp -f " + config_path + " " + base_path );

    // trying to apply
    if(!executeShellCommand("netplan generate")){
        executeShellCommand("rm /etc/netplan/*yaml -f");
        executeShellCommand("mv /etc/netplan/back/*yaml /etc/netplan");
        std::cout << "Failed to generate the netplan conf" << std::endl;
        return false;
    }
    if(!executeShellCommand("netplan apply")){
        executeShellCommand("rm /etc/netplan/*yaml -f");
        executeShellCommand("mv /etc/netplan/back/*yaml /etc/netplan");
        std::cout << "Failed to apply the netplan conf" << std::endl;
        return false;
    }
    return true;
}

bool NetplanConfigMaker::setRenderer(const std::string& renderer){
    if(renderer != "networkd" && renderer != "NetworkManager"){
        return false;
    }
    _renderer = renderer;
    return true;
}

YAML::Node NetplanConfigMaker::generateEthernetsYamlNode(){
    YAML::Node yaml_node;
    for(auto iter = _ethernets.begin(); iter != _ethernets.end(); iter++){
        Netplan_Ethernet* ethernet = (*iter);
        std::string name = ethernet->getName();
        yaml_node[name.c_str()] = ethernet->generateYamlNode();
    }
    return yaml_node;
}

YAML::Node NetplanConfigMaker::generateBondsYamlNode(){
    YAML::Node yaml_node;
    for(auto iter = _bonds.begin(); iter != _bonds.end(); iter++){
        Netplan_Bond* bond = (*iter);
        std::string name = bond->getName();
        yaml_node[name.c_str()] = bond->generateYamlNode();
    }
    return yaml_node;
}

YAML::Node NetplanConfigMaker::generateBridgesYamlNode(){
    YAML::Node yaml_node;
    for(auto iter = _bridges.begin(); iter != _bridges.end(); iter++){
        Netplan_Bridge* bridge = (*iter);
        std::string name = bridge->getName();
        yaml_node[name.c_str()] = bridge->generateYamlNode();
    }
    return yaml_node;
}

YAML::Node NetplanConfigMaker::generateVlanYamlNode(){
    YAML::Node yaml_node;
    for(auto iter = _vlans.begin(); iter != _vlans.end(); iter++){
        Netplan_Vlan* vlan = (*iter);
        std::string name = vlan->getName();
        yaml_node[name.c_str()] = vlan->generateYamlNode();
    }
    return yaml_node;
}

Netplan_Ethernet* NetplanConfigMaker::parseEthernetsInfo(YAML::Node& node){
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }
    
    Netplan_Ethernet* ethernets = new Netplan_Ethernet;
    auto iter = node.begin();
    std::string name = iter->first.as<std::string>();
    ethernets->setName(name);
    
    // add addresses when yaml-cpp can find one
    if(!node["dhcp4"].IsDefined()){
        std::cout << "dhcp4 is not defined" << std::endl;
    }
    if(!node["dhcp4"].IsNull()){
        std::cout << "dhcp4 is null" << std::endl;
    }

    int size = node.size();
    for(int i = 0; i < size; i++){
        YAML::Node ethernet_yaml_node = node[i];
        Netplan_Ethernet* ethernet = new Netplan_Ethernet;
        bool ans =  Netplan_Ethernet::parseYamlNode(ethernet_yaml_node, ethernet, name);

        if(ans){
            _ethernets.push_back(ethernet);
        }
        else{
            delete ethernet;
        }

    }

    return ethernets;
}

Netplan_Bond* NetplanConfigMaker::parseBondsInfo(YAML::Node& node){
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }
    
    Netplan_Bond* bonds = new Netplan_Bond;
    auto iter = node.begin();
    std::string name = iter->first.as<std::string>();
    bonds->setName(name);
    
    // add addresses when yaml-cpp can find one
    if(!node["dhcp4"].IsDefined()){
        std::cout << "dhcp4 is not defined" << std::endl;
    }
    if(!node["dhcp4"].IsNull()){
        std::cout << "dhcp4 is null" << std::endl;
    }

    int size = node.size();
    for(int i = 0; i < size; i++){
        YAML::Node bond_yaml_node = node[i];
        Netplan_Bond* bond = new Netplan_Bond;
        bool ans =  Netplan_Bond::parseYamlNode(bond_yaml_node, bond, name);

        if(ans){
            _bonds.push_back(bond);
        }
        else{
            delete bond;
        }

    }

    return bonds;
}

Netplan_Bridge* NetplanConfigMaker::parseBridgesInfo(YAML::Node& node){
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }
    
    Netplan_Bridge* bridges = new Netplan_Bridge;
    auto iter = node.begin();
    std::string name = iter->first.as<std::string>();
    bridges->setName(name);
    
    // add addresses when yaml-cpp can find one
    if(!node["dhcp4"].IsDefined()){
        std::cout << "dhcp4 is not defined" << std::endl;
    }
    if(!node["dhcp4"].IsNull()){
        std::cout << "dhcp4 is null" << std::endl;
    }

    int size = node.size();
    for(int i = 0; i < size; i++){
        YAML::Node bridge_yaml_node = node[i];
        Netplan_Bridge* bridge = new Netplan_Bridge;
        bool ans =  Netplan_Bridge::parseYamlNode(bridge_yaml_node, bridge, name);

        if(ans){
            _bridges.push_back(bridge);
        }
        else{
            delete bridge;
        }

    }

    return bridges;
}

Netplan_Vlan* NetplanConfigMaker::parseVlansInfo(YAML::Node& node){
    if(!node.IsDefined() || node.IsNull()){
        return nullptr;
    }
    
    Netplan_Vlan* vlans = new Netplan_Vlan;
    auto iter = node.begin();
    std::string name = iter->first.as<std::string>();
    vlans->setName(name);
    
    // add addresses when yaml-cpp can find one
    if(!node["dhcp4"].IsDefined()){
        std::cout << "dhcp4 is not defined" << std::endl;
    }
    if(!node["dhcp4"].IsNull()){
        std::cout << "dhcp4 is null" << std::endl;
    }

    int size = node.size();
    for(int i = 0; i < size; i++){
        YAML::Node vlan_yaml_node = node[i];
        Netplan_Vlan* vlan = new Netplan_Vlan;
        bool ans =  Netplan_Vlan::parseYamlNode(vlan_yaml_node, vlan, name);

        if(ans){
            _vlans.push_back(vlan);
        }
        else{
            delete vlan;
        }

    }

    return vlans;
}


bool executeShellCommand(const std::string& command){
    if(std::system(command.c_str()) == -1){
        std::cout << "Command: " << command << ", Failed!" << std::endl;
        return false;
    }
    else{
        std::cout << "Command: " << command << ", succeed!" << std::endl;
        return true;
    }
}

bool NetplanConfigMaker::addAddress(const std::string& dev_name, const std::string& ip, const std::string& mask){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        if(!dev_p->addAddress(ip, mask))
            return false;
        else
            return true;
    }
    else
        return false;

}


bool NetplanConfigMaker::delAddress(const std::string& dev_name, const std::string& ip, const std::string& mask){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        std::cout << "Find a Ethernet" << std::endl;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        std::cout << "Find a Bond" << std::endl;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        std::cout << "Find a Bridge" << std::endl;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        std::cout << "Find a Vlan" << std::endl;

    if(dev_p){
        if(!dev_p->delAddress(ip, mask))
            return false;
        else
            return true;
    }
    else
        return false;
}

bool NetplanConfigMaker::addVitualAddress(const std::string& dev_name, const std::string& ip, const std::string& mask){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        if(!dev_p->addVitualAddress(ip, mask))
            return false;
        else
            return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::delVitualAddress(const std::string& dev_name, const std::string& ip, const std::string& mask){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        if(!dev_p->delVitualAddress(ip, mask))
            return false;
        else
            return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::delVitualAddress(const std::string& ip, const std::string& mask){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        if(!dev_p->delVitualAddress(ip, mask))
            return false;
        else
            return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::addRoute(const std::string& dev_name,  const std::string& ip, const std::string& mask, const std::string& via){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        if(!dev_p->addRoute(ip, mask, via))
            return false;
        else
            return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::delRoute(const std::string& dev_name,  const std::string& ip, const std::string& mask, const std::string& via){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        dev_p->delRoute(ip, mask, via);
        return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::addNameServer(const std::string& dev_name, const std::string& nameserver){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        dev_p->addNameServer(nameserver);
        return true;
    }
    else{
        return false;
    }
}

bool NetplanConfigMaker::delNameServer(const std::string& dev_name, const std::string& nameserver){
    if(dev_name.empty())
        return false;

    Netplan_Device* dev_p; 

    // 查找设备，并放入dev_p
    if ((dev_p = isEthernet(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBond(dev_name)) != nullptr)
        ;
    else if ((dev_p = isBridge(dev_name)) != nullptr)
        ;
    else if ((dev_p = isVlan(dev_name)) != nullptr)
        ;

    if(dev_p){
        dev_p->delNameServers(nameserver);
        return true;
    }
    else{
        return false;
    }
}

// 查看当前dev_name是否存在
Netplan_Ethernet* NetplanConfigMaker::isEthernet(const std::string& dev_name){
    if(dev_name == "" || _ethernets.empty())
        return nullptr;

    for(auto iter = _ethernets.begin(); iter != _ethernets.end(); iter++){
        auto ethernet_p = *iter;
        if(dev_name == ethernet_p->getName())
            return ethernet_p;
    }
    return nullptr;
}

// 查看当前dev_name是否存在
Netplan_Bond* NetplanConfigMaker::isBond(const std::string& dev_name){
    if(dev_name == "" || _bonds.empty())
        return nullptr;

    for(auto iter = _bonds.begin(); iter != _bonds.end(); iter++){
        auto bond_p = *iter;
        if(dev_name == bond_p->getName())
            return bond_p;
    }
    return nullptr;
}

// 查看当前dev_name是否存在
Netplan_Bridge* NetplanConfigMaker::isBridge(const std::string& dev_name){
    if(dev_name == "" || _bridges.empty())
        return nullptr;

    for(auto iter = _bridges.begin(); iter != _bridges.end(); iter++){
        auto bridge_p = *iter;
        if(dev_name == bridge_p->getName())
            return bridge_p;
    }
    return nullptr;
}

// 查看当前dev_name是否存在
Netplan_Vlan* NetplanConfigMaker::isVlan(const std::string& dev_name){
    if(dev_name == "" || _vlans.empty())
        return nullptr;

    for(auto iter = _vlans.begin(); iter != _vlans.end(); iter++){
        auto vlan_p = *iter;
        if(dev_name == vlan_p->getName())
            return vlan_p;
    }
    return nullptr;
}