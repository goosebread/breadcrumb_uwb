#!/usr/bin/env python3

## ros node on robot to send map to breadcrumbs
# may need to add delay between transmissions to give other side time to process

TOPIC = '/map'

MAX_PACKET_LENGTH = 127 #124 bytes of useful data
SLEEP_TIME = 0.08 #0.05 for one hop works
ttl = 3 #one byte used

#for breadcrumb interface
import pylink
import time
import os

#for ros node
import rospy
from nav_msgs.msg import OccupancyGrid
import gzip
import pickle

#breadcrumb constants
chunk_size = 124 # 127 - ttl byte - crc bytes
prefix = "2" #keyword for 
num_crc = 2 #2 bytes of crc
ttl_byte = int.to_bytes(ttl,length=1,byteorder='big')

def callback(data):

    tx_file_u = "temp_u.pk"
    tx_file = "tempfile.pk.gz"

    #pickle
    f_uncompressed = open(tx_file_u,'wb')
    pickle.dump(data,f_uncompressed)
    #send to compressed file
    with gzip.open(tx_file, "wb") as f:
        f.write(f_uncompressed.read())#TODO this line might need to be altered to fit actual map data
    f_uncompressed.close()

    #header frame
    fsize = os.path.getsize(tx_file)
    info_msg = "FILE_START "+tx_file+" "+str(fsize)+" "
    info_frame = b''.join([str.encode(prefix),ttl_byte, str.encode(info_msg),str.encode((chunk_size-len(info_msg)+2)*"\0")])
    jlink.rtt_write(0,info_frame)
    time.sleep(SLEEP_TIME)

    #data frames
    with open(tx_file, "rb") as in_file:
        while True:
            chunk = in_file.read(chunk_size)
            if chunk == b"":
                break # end of file
                
            #send data
            jlink.rtt_write(0,b''.join([str.encode(prefix),ttl_byte, chunk,str.encode(2*"\0")]))
            time.sleep(SLEEP_TIME)

    os.remove(tx_file_u)
    os.remove(tx_file)
        
def listener():
    rospy.init_node('breadcrumb_tx', anonymous=True)
    rospy.Subscriber(TOPIC, OccupancyGrid, callback)
    rospy.spin()

if __name__ == '__main__':
    #serial_no = '760150700'
    #serial_no = '760150946'
    #serial_no = '760150725'
    #serial_no = '760150500'
    #serial_no = '760180685'
    serial_no = '760150575'

    #jlink = pylink.JLink()
    #jlink = pylink.JLink(lib=pylink.library.Library("C:\Program Files (x86)\SEGGER\JLink\JLink_x64.dll"))
    jlink = pylink.JLink(lib=pylink.library.Library("/opt/SEGGER/JLink_V786e/libjlinkarm.so.7.86.5"))
    jlink.open(serial_no)
    jlink.connect('nRF52832_xxAA', verbose=True)
    jlink.rtt_start()
    time.sleep(0.5) #need a bit of time for link to settle

    listener()


