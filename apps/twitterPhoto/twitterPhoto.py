import tweepy
import time
import random
from subprocess import call
from api_keys import *
#import attacklib

#These are the phrases variables that will be sent with the tweet
tweet = 'Tomato status:'
image_name = '/home/pyronia/libpyronia/apps/twitterPhoto/tomato-status.jpg'

#while True:
num_iters = 100
#iter_times = []

for i in range(0, num_iters):
    #start = time.clock()
    #base_image = imread(base_image_name)
    
    # OAuth process, using the keys and tokens
    auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
    auth.set_access_token(access_token, access_token_secret)

    # Creation of the actual interface, using authentication
    api = tweepy.API(auth)
    
    # Send the tweet with photo
    api.update_with_media(image_name, status=tweet)
    #attacklib.unauth_server_conn(api, image_name, tweet)

    #end = time.clock()
    #iter_times.append(str(end-start))

    #How many seconds before the script runs again
    time.sleep(60)

'''
f = open('/home/pyronia/libpyronia/apps/twitterPhoto/twitterPhoto.py.data', 'a+')
f.write(' '.join(iter_times))
f.write('\n')
f.close()
'''
