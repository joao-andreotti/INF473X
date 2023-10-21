# Challenge Theta
Nom: Marques Andreotti

Prénom: João

# 🏗️ Build
To build you must first have `libtins` installed on your machine. For installation instructions, look at the `libtins` wiki. 
After installing `libtins` run:
```bash
make
```
The tool will be saved under the name `./dhcp_usurp`. 

If you want to change settings relating to the DHCP server (such as DNS servers provided or other options) edit the file `dhcp_usurp.cpp` before building.


# 🏃 Run
To run, supply the interface to be used as an argument to `dhcp_usurp`

```bash
sudo ./dhcp_usurp <interface>
```

# 🎭 Demo

A virtual machine running `lubuntu` was setup using `VirtualBox`. The machine is then connected to a host only adapter with DHCP server enabled.

We are then able to run our tool on that network and observe on Wireshark the DHCP packets sent and received. Upon noticing that the server is not able to provide any more addresses we can then renew the lease on the virtual machine to verify the server functionality of our code.

To renew the lease, two commands were used:

```bash
sudo dhclient -r
sudo dhclient
```

The first one releases the current lease and the second acquires a new one.

We can then verify that the VM is using the configuration provided by our tool (IP address on interface and DNS server).

The steps described above are all displayed on [this youtube video](https://youtu.be/zcCyBxu8iRU).

Some explanations regarding the somewhat odd setup:
- The VM is already connected to the network before the attack begins. This is because `VirtualBox` does not turn on the network interface until a VM is on it (it will show up on `ifconfig` but no packets are sent).
- Why not use a NAT and let us see that we can exploit the DNS server setting capabilities on the internet? Unfortunately `VirtualBox` does not let us configure several NAT networks (neithes does it allow us to change its DHCP configuration). As you might have noticed on the video, I had to create MANY networks to debug and to record the video. Using the single NAT network would not be practical.
- A little more on why I had to create an enourmous ammount of networks: deleting a network and recreating it does not reset the DHCP server's leases, so the only option to run the attack a second time is to create a new network.
- All in all, `VirtualBox` is a pain in the ass.