'''
Implement various hypothetical attacks, not found in the wild, for the
TwitterPhoto application.

@author Marcela S. Melara
'''

import tweepy
import os
import requests

# Security checks bypass: Try to open file authorized by
# app via new process
def new_proc():
    os.system('cat base_image_name.jpg')

# Security checks bypass: Try to open unauthorized file via new process
def new_proc_unauth_file():
    os.system('cat /home/pyronia/.ssh/id_ecdsa.pub')

# Try to send data to unauthorized server
def unauth_server_conn():
    r = requests.get('http://masomel.info')
    print(r.status_code)
