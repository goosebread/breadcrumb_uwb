#!/usr/bin/env python3

# get data from breadcrumbs and retrasnmit in ros /map topic

import rospy
import gzip
import pickle
from nav_msgs.msg import OccupancyGrid

import pylink
import time

chunk_size = 124

in_filename = "rx_tempfile.pk.gz"

def rx_file():
    startedReading = False
    doneReading = False

    filename = ''
    filesize = 0
    extra_bytes = 0

    out_file = 0
    carryover = 0

    while not doneReading:

        time.sleep(0.08)#consider decreasing read to 124 and get rid of delay

        #code to read rtt stream
        raw_bytes = jlink.rtt_read(0,2048)
        if len(raw_bytes)!=0:
            chars = [chr(c) for c in raw_bytes]
            if carryover:
                chars = [carryover]+chars
            if len(chars)%2==1:
                carryover = chars[-1]
                chars = chars[0:-1]

            c2 = [''.join(chars[i:i+2]) for i in range(0, len(chars), 2)]
            msg_bytes = [int(c,16) for c in c2] 
            #print(len(msg_bytes))  

            #first frame received
            if not startedReading:
                s1=''      
                for b in msg_bytes:
                    s1+=chr(b)
                #s1.join([chr(c) for c in msg_bytes])
                #print(s1)

                if s1.startswith("FILE_START"):
                    startedReading=True
                    info = s1.split(sep=' ')
                    filename = info[1]
                    filesize = int(info[2])
                    out_file = open("rx_"+filename, "wb")
                    print("Start Packet Received")
                    #print(info)
                    
                    extra_bytes = chunk_size - len(msg_bytes)
                    #print(extra_bytes)

            #data frames
            elif not doneReading:
                print(filesize)
                if extra_bytes>0:#get rid of extra bytes from header
                    #print("Trim extra bytes")
                    if extra_bytes>=len(msg_bytes):
                        extra_bytes = extra_bytes - len(msg_bytes)
                        break
                    else:
                        msg_bytes = msg_bytes[extra_bytes:]
                        extra_bytes = 0

                filesize = filesize-len(msg_bytes)
                if filesize<=0: #get rid of extra zeros in last frame
                    doneReading=True
                    if filesize!=0:
                        msg_bytes = msg_bytes[0:filesize]
                
                out_file.write(bytes(msg_bytes))

            #done with file
            else:
                doneReading=True
                break
    out_file.close()

def map_publisher():
    while not rospy.is_shutdown():
        #rate is controlled by how long it takes to receive a file
        rx_file()
        print("Rx done")
        with gzip.open(in_filename, "rb") as f:  
            data = pickle.load(f)
            pub.publish(data)
            print("Publishing Map")
            
def shutdown_hook():
    print("\nRospy Shutdown :( \n")

rospy.on_shutdown(shutdown_hook)

if __name__ == '__main__':
    serial_no = '760180673'
    #serial_no = '760180907'
    #serial_no = '760150725'
    #serial_no = '760150500'
    #serial_no = '760180685'
    #serial_no = '760150700'
    #serial_no = '760150575'

    #filepath for Ubuntu VM
    jlink = pylink.JLink(lib=pylink.library.Library("/opt/SEGGER/JLink_V786f/libjlinkarm.so.7.86.6"))

    jlink.open(serial_no)
    jlink.connect('nRF52832_xxAA', verbose=True)
    jlink.rtt_start()

    pub = rospy.Publisher('/map', OccupancyGrid, queue_size=10)
    rospy.init_node('map_breadcrumb_rx', anonymous=True)

    time.sleep(0.5) #need a bit of time for link to settle

    try:
        map_publisher()
        
    except rospy.ROSInterruptException:
        pass
