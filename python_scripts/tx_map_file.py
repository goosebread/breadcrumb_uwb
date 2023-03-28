MAX_PACKET_LENGTH = 127 #124 bytes of useful data
SLEEP_TIME = 0.08 #0.05 for one hop works

import pylink
import time
import os

chunk_size = 124 # 127 - ttl byte - crc bytes
prefix = "2" #keyword for 
ttl = 3 #ttl = 1, one byte used
num_crc = 2 #2 bytes of crc
ttl_byte = int.to_bytes(ttl,length=1,byteorder='big')

if __name__ == '__main__':
   
    #serial_no = '760150700'
    serial_no = '760150946'
    #serial_no = '760150725'
    #serial_no = '760150500'
    #serial_no = '760180685'
    #serial_no = '760150575'

    jlink = pylink.JLink()
    #jlink = pylink.JLink(lib=pylink.library.Library("/opt/SEGGER/JLink_V786e/libjlinkarm.so.7.86.5"))
    jlink.open(serial_no)
    jlink.connect('nRF52832_xxAA', verbose=True)
    jlink.rtt_start()
    time.sleep(0.5) #need a bit of time for link to settle

    in_filename = "churchillarea.txt.gz" 
    #in_filename = "test.txt" 

    #header frame
    fsize = os.path.getsize(in_filename)
    info_msg = "FILE_START "+in_filename+" "+str(fsize)+" "
    info_frame = b''.join([str.encode(prefix),ttl_byte, str.encode(info_msg),str.encode((chunk_size-len(info_msg)+2)*"\0")])
    jlink.rtt_write(0,info_frame)
    time.sleep(SLEEP_TIME)

    #data frames
    with open(in_filename, "rb") as in_file:
        while True:
            chunk = in_file.read(chunk_size)
            if chunk == b"":
                break # end of file
                
            #send data
            jlink.rtt_write(0,b''.join([str.encode(prefix),ttl_byte, chunk,str.encode(2*"\0")]))
            time.sleep(SLEEP_TIME)




