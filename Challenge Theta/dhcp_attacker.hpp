#ifndef DHCP_ATTACKER_
#define DHCP_ATTACKER_

#include <chrono>
#include <cstdlib>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_set>
#include <unordered_map>

#include <tins/ethernetII.h>
#include <tins/ip.h>
#include <tins/dhcp.h>

class DHCPAttacker {
public:
    DHCPAttacker(std::string iface_name);
    ~DHCPAttacker();

    void initiate_starvation(size_t num_threads = 4);
    void initiate_server();

    // Pushes options that will be used by server when responding to requests
    void push_server_option(std::function<void(Tins::DHCP&)> op);


    // Type used to store leases
    struct DHCPLease {
        // Lease address
        Tins::IPv4Address addr;

        // Total lease time
        uint32_t lease_time; // in seconds

        // Time when lease was granted
        typedef std::chrono::time_point<std::chrono::steady_clock> time_point;
        time_point acquisition_time;

        // Decides if lease is still valid
        bool is_valid() const;

        uint32_t remaining_lease() const;

        // For use on set
        bool operator<(const DHCPLease &l) const;
    };

    // Adds lease to pool
    void push_lease(DHCPLease &lease);
private:
    // Server
    void server_worker();
    void treat_discovery(Tins::DHCP discover);

    // Starver
    void starvation_worker();


    // MAC address prefix
    std::vector<uint8_t> mac_prefix_ = {0x14, 0xDD, 0xA9};

    // Source and destionation ports
    const uint16_t dport = 67;
    const uint16_t sport = 68;

    // Timeout for total transaction time
    const uint32_t transaction_timeout_ = 15; // in secs.

    // Lease time requested to server
    uint32_t requested_lease_time = 3600;

    // Network device to be used
    std::string iface_name_;

    // DHCP requests being treated by server
    std::mutex treating_ids_mutex_{};
    std::unordered_set<uint32_t> treating_ids_;

    // Thread running server
    std::thread server_worker_;

    // Options server responds with
    std::mutex server_options_mutex_{};
    std::vector<std::function<void(Tins::DHCP&)>> server_options_;

    // Server HW address
    Tins::EthernetII::address_type server_hw_address_;

    // Threads running starvation
    std::vector<std::thread> starvation_workers_;


    // Leases acquired from legit. server
    std::mutex leases_acquired_mutex_{};
    std::set<DHCPLease> leases_acquired_;

    // Stores transaction IDs used by the starvation attack
    // This prevents the server responding to them
    std::mutex starvation_ids_mutex_{};
    std::unordered_set<uint32_t> starvation_ids_;
};

#endif // DHCP_ATTACKER_