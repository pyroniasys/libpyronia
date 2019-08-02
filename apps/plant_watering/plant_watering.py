from paho.mqtt.client import Client
import ssl
from select import select
import random, time
#import attacklib

# A random programmatic shadow client ID.
SHADOW_CLIENT = "myShadowClient"

# The unique hostname that AWS IoT generated for 
# this device.
HOST_NAME = "a2t65gjx7e9mk3-ats.iot.us-west-2.amazonaws.com"

path = "/home/pyronia/libpyronia/apps/plant_watering"

# The relative path to the correct root CA file for AWS IoT, 
# which you have already saved onto this device.
ROOT_CA = path+"/AmazonRootCA1.pem"

# The relative path to your private key file that 
# AWS IoT generated for this device, which you 
# have already saved onto this device.
PRIVATE_KEY = path+"/df401a2b29-private.pem.key"

# The relative path to your certificate file that 
# AWS IoT generated for this device, which you 
# have already saved onto this device.
CERT_FILE = path+"/df401a2b29-certificate.pem.cert.txt"

# A programmatic shadow handler name prefix.
SHADOW_HANDLER = "pyronia-vm"

connflag = False

def on_connect(client, userdata, flags, rc):
   global connflag
   connflag = True
   print("Connection returned result: " + str(rc))

def on_message(client, userdata, msg):
  print(msg.topic+" "+str(msg.payload))

def on_publish(client):
   print("Published. Disconnecting")
   client.disconnect()
   
# Automatically called whenever the shadow is updated.
def myShadowUpdateCallback(payload, responseStatus, token):
  print()
  print('UPDATE: $aws/things/' + SHADOW_HANDLER + 
    '/shadow/update/#')
  print("payload = " + payload)
  print("responseStatus = " + responseStatus)
  print("token = " + token)

# Create, configure, and connect a shadow client.
'''
myShadowClient = AWSIoTMQTTShadowClient(SHADOW_CLIENT)
myShadowClient.configureEndpoint(HOST_NAME, 8883)
myShadowClient.configureCredentials(ROOT_CA, PRIVATE_KEY,
  CERT_FILE)
myShadowClient.configureConnectDisconnectTimeout(10)
myShadowClient.configureMQTTOperationTimeout(5)
myShadowClient.connect()
'''

mqttc = Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.on_publish = on_publish
mqttc.tls_set(ca_certs=ROOT_CA, certfile=CERT_FILE, keyfile=PRIVATE_KEY, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_SSLv23, ciphers=None)
mqttc.connect(HOST_NAME, 8883, 60)

# Create a programmatic representation of the shadow.
#myDeviceShadow = myShadowClient.createShadowHandlerWithName(
#  SHADOW_HANDLER, True)

# Keep generating random test data until this script 
# stops running.
# To stop running this script, press Ctrl+C.
#while True:
num_iters = 100
iter_times = []

#for i in range(0, num_iters):
   #start = time.clock()

sock = mqttc.socket()
if not sock:
   raise Exception("Socket is gone")

# Generate random True or False test data to represent
# okay or low moisture levels, respectively.
moisture = random.choice([True, False])
# moisture = attacklib.confused_deputy()
#moisture = attacklib.symlink_attack()
#mqttc.loop_start()

print("Selecting for reading" + (" and writing" if mqttc.want_write() else ""))
r, w, e = select(
   [sock],
   [sock] if mqttc.want_write() else [],
   [],
   1
)

if sock in r:
   print("Socket is readable, calling loop_read")
   mqttc.loop_read()
   
if sock in w:
   print("Socket is writable, calling loop_write")
   mqttc.loop_write()
   
while connflag == False:
   print('.')
      
if moisture:
   message = '{"state":{"reported":{"moisture":"okay"}}}'
else:
   message = '{"state":{"reported":{"moisture":"low"}}}'
mqttc.publish('$aws/things/'+SHADOW_HANDLER+'/shadow/update', message, qos=1)
   #end = time.clock()
   #iter_times.append(str(end-start))

   # Wait for this test value to be added.
   #time.sleep(5)

mqttc.disconnect()

'''
f = open('/home/pyronia/libpyronia/apps/plant_watering/plant_watering.py.data', 'a+')
f.write(' '.join(iter_times))
f.write('\n')
f.close()
'''
