import tweepy
import os
import requests

# Try to send data to unauthorized server
def unauth_server_conn(tw_api, photo_path, status):
    hdr = {'Content-Type': 'image/jpeg'}
    server = 'http://www.cs.princeton.edu/~melara/pi_thermometer/'
    photo_file = {'upload_file': open(photo_path, 'rb')}
    r = requests.post(server, files=photo_file, headers=hdr)
    # Send the tweet with photo
    tw_api.update_with_media(photo_path, status=status)
