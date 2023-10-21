# Challenge Theta
Nom: Marques Andreotti

Prénom: João

# 🏗️ Build
To build you must first have `libtins` installed on your machine. For installation instructions, look at the `libtins` wiki. 
After installing `libtins` run:
```bash
make
```
The tool will be saved under the name `./tcp_hijack`. 


# 🏃 Run
To run, supply the interface and TCP configurations to be used as an argument to `tcp_hijack`:

```bash
sudo ./tcp_hijack <client> <server> <port> [interface]
```

Where the parameters are:
-`client`: TCP client network address.
-`server`: TCP server network address.
-`port`: TCP server listening port.
-`interface`: Interface to be used.

# 🎭 Demo

Two virtual machines were setup on two different host only networks. On each machine a route to the other was specified going through the attacker machine (for this demo it's my real machine). This way we are effectively in a MITM position.

We can then establish a TCP connection between the two machines and test that it works until the attack is run.

Aften running the atack, we gain control over the TCP session. The original client can no longer send messages and the attacker can now send anything over the connection. We can also receive packets through the connection.

The steps described above are all displayed on [this youtube video](https://youtu.be/iujrG03brfU).

A very similar test can be run to prove that we are able to steal a telnet session. A demonstration of the attack can be seen [here](https://youtu.be/NqkyB0Welag).
