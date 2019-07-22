import tweepy
import time
import random
from subprocess import call
#from datetime import datetime
from cv2 import imread
#import image_lib
#import msgformat
from api_keys import *

#These are the phrases variables that will be sent with the tweet
tweet = ['A tweet from my pi with pyronia', 'Hello!  ']
base_image_name = '/home/pyronia/libpyronia/testsuite/python-tests/edge.jpg'

#while True:
for x in range(0, 2):

    #base_image = imread(base_image_name)

    #time and date for filename
    '''
    i = datetime.now()               
    photo_name = msgformat.nice_name()
    print(photo_name)
    '''
    
    #proc = image_lib.edgify(base_image, base_image_name)
    
    # OAuth process, using the keys and tokens
    auth = tweepy.OAuthHandler(consumer_key, consumer_secret)
    auth.set_access_token(access_token, access_token_secret)
    
    # Creation of the actual interface, using authentication
    api = tweepy.API(auth)
    
    # Send photo to destination
    photo_path = base_image_name
    # Tweet text
    status = (random.choice(tweet)) #+  i.strftime('%Y/%m/%d %H:%M:%S')
    # Send the tweet with photo
    api.update_with_media(photo_path, status=status)

    #How many seconds before the script runs again
    time.sleep(2)

