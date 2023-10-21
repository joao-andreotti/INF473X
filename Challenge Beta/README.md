# 🏗️ Challenge Beta

This challenge's objective is to gain access to the files belonging to user 'superxwebdeveloper' by interacting with his [website](https://superxwebdeveloper.epizeuxis.net/). What follows is a description of the vulnerabilities exploited in order to finish the challenge.

# 📖 Sensitive Files on Git Repository
## 🗡️ Vulnerability

Looking at the webapp's code we quickly notice that the authentication is done by the following function:

```python
from my_secrets import my_username, my_password

def login(username, password):
    return username == my_username and password == my_password
```

The first thought that might come to mind is to look for the `my_secrets` file on the `git` repository, but that'll not work since it was added to the `.gitignore` list. However, upon looking at the commit that adds the sensitive file to the ignore list, we notice that the file had been commited beforehand, making it available to us as `git` stores every intermediate state of the repository:

```python
# Please call a teacher to validate your solution.
# In the meantime, you can pursue the challenge.
my_username = "superXWebDeveloper"
my_password = "bA8wd5NkGvtMdaYgBbD3"
```

The password and user can be read in plaintext.

## 🛡️ Solution

A quick solution is to change the user and password on the server. Since the file is already on the `.gitignore` list it will not be uploaded to `GitHub`. This, however, does not fix the underlying issue: passwords should never be saved to the filesystem unencrypted.

A possible solution is to store the password hashed and salted with `bcrypt`. Example code below:
```python
# Before storing password
hashed_password = bcrypt.hashpw(password, bcrypt.gensalt(12))

# To check whether the provided password is correct
bcrypt.checkpw(provided_password, hashed_password)
```

Furthermore, using hard coded passwords is, in general, not a good idea. It would be a wise to use a database such as an SQL database to store the use and password pair(s) and to properly configure/harden it. These recommendations are not exhaustive, using a [best practices guide](https://snyk.io/learn/password-storage-best-practices/) from a reputable source is advisable.


# 🔑 Insufficient Entropy on RNG
## 🗡️ Vulnerability

Looking at the code that generates the correct balls for the game, we see that it has two sources of entropy that are used to initialise the RNG: `int(time.time())` and the user `id`.

```python
def challenge(id):
    random.seed(int(time.time()) + id)
    solution = random.sample(list(range(1,50)), 5)
    solution.sort()
    return solution
```

Firstly, `int(time.time())` is the time in seconds between now and a moment in the past. It can thus be easily predicted by an attacker (if not exactly, with sufficient accuracy). Secondly, the user `id` is taken uniformly from 1 to 1000000. This means that all combinations can be locally tested by an attacker if he has a test that discerns the correct `id`, making it easy to bruteforce.

The final piece of the vulnerability comes with the fact that the website discloses the correct result after an attempt. This makes it possible to test all possible seeds to the RNG until we find the correct one.

The final attack is then:
- submit any five balls and record the time
- look the correct balls provided by the server
- locally bruteforce id using the time and the correct solution
- predict the correct five balls for a moment in time and submit to server

Since this attack depends heavily on timing, a script can be used to automate the steps. This was done using the `selenium` library for python and can be seen in the `crack.py` script.


## 🛡️ Solution

Using a better RNG would solve the problem entirely. We could, for example, use the cryptographically secure RNG provided by the `secrets` library in python like so:

```python
def challenge():
    balls = list(range(49))
    idxs = [secrets.randbelow(49 - i) for i in range(5)]
    solution = [balls.pop(i) + 1 for i in idxs]
    solution.sort()
    return solution
```

This would make the five ball sequence almost impossible to predict. As specified by the library, the generator "provides access to the most secure source of randomness that your operating system provides". This means that performance might be compromised if this is to be used in a massive scale. If that becomes a problem, a good compromise between security and speed can be found by using a cryptographic PRNG.

# 🗄️ Arbitrary File Upload
## 🗡️ Vulnerability

After winning the game, the website provides the user with the possibity of submitting his name to be added to a leaderboard. The code that saves the user's name is as follows:

```python
# (...) AUTHENTICATION
with open(scoreboard_folder + request.form["date"], "a+") as f:
    f.write(request.form["name"])
```

We immediately notice that no filtering is done to ensure that the date provided by the user is valid. In practice, this means that the user can make a `POST` request to the page with any value on the `date` field and that it will be used to build the filename to be written to.

Since the user can also provide his name, which is the only thing written to the file, we have arbitrary file upload with directory traversal. The user can access any file in the filesystem by choosing an appropriate value for `date` (such as `../main.py`) and append any sequence of bytes to it.

This can be exploited to gain shell access by:
- First, we generate an ssh key and save it to a file named `key` (`key.pub` for the public key).
```bash
ssh-keygen -t ecdsa
```
- We then append the public key to the user's `~/.ssh/authorized_keys` file using our newly found vulnerability. Look for the scripts `push_key.sh` and `upload.py` for more details.
- We may now connect to the server using:
```bash
ssh -i keys/key web@superxwebdeveloper.epizeuxis.net
```
- We now have SSH access to the machine running the webserver.

## 🛡️ Solution

The root cause of the vulnerability is trusting the user's input. It can thus be solved by using server time instead of the user provided value. A simple solution would be as follows:

```python
# (...) AUTHENTICATION
with open(scoreboard_folder + str(int(time.time())), "a+") as f:
    f.write(request.form["name"])
```

It is still generally undesirable to allow the user to write arbitrary contents into a file in the filesystem as this can be used to facilitate the exploitation of other vulnerabilities. A better alternative would be to use a `SQL` database (properly configured and hardened).

Another issue is the hard-coded value `scoreboard_folder` which can be used to gain information on the structure of the filesystem (as it is available on the GitHub repository). A better alternative would be to put it on an environment variable as is done for the server's `secret_key`. 

# 🔓 Overly Permissive `sudoers` File
## 🗡️ Vulnerability

After logging onto SSH we are able to run:
```bash
sudo -l
```
Showing us that:
```
User web may run the following commands on c67766bab583:
    (superxwebdeveloper) NOPASSWD: /usr/bin/vim
```
This means we are able to run `vim` as the user `superxwebdeveloper` by doing "`sudo -u superxwebdeveloper vim`". Once on `vim` we can easily gain a shell as that user by inputting "`:!/bin/bash`" into the `vim` commandline. We now have access to the machine as user `superxwebdeveloper`.


## 🛡️ Solution

Simply removing the line that allows the user `web` to execute `vim` as `superxwebdeveloper` from the `sudoers` file solves the issue. One should be very careful before adding anything to the `sudoers` file as many common utilities can be used for privillege escalation. 