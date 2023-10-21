# Challenge Delta
Nom: Marques Andreotti

Prénom: João

# 🏗️ Build
To build you must first have `libpcap` installed on your machine. After installing `libpcap` run:
```bash
make
```
The tool will be saved under the name `./dns_spoofer`


# 🏃 Run
To run, first modify the `hosts.txt` file to reflect the websites you want to replace. For instance you can use:

```txt
127.0.0.1 wordreference.com
127.0.0.1 nytimes.com
```

Which is the configuration to redirect both `wordreference` and the `nytimes` to `localhost`. After mofifying the `hosts.txt` file, run:

```bash
sudo ./dns_spoofer
```

The tool is now running and will keep running until you terminate it.

# 🎭 Demo

The `hosts.txt` was configured as follows:

```txt
129.104.222.87  wireshark.org
```

The IP address which we are pointing to is that of my machine. Accessing `wireshark.org` should then redirect us to a server under my control.

We then start a simple flask app to listen for incoming connections:

```python
from flask import Flask, redirect

app = Flask(__name__)

@app.route("/")
def rick_roll():
    return redirect("https://hi.joshuakgoldberg.com/")
```

We are now able to test the tool by opening the targeted website on a virtual machine. To see the results watch [this youtube video](https://youtu.be/EZ_BdJ8uPY8).

