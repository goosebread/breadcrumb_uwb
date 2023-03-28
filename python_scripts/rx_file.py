import pylink
import time

chunk_size = 124


if __name__ == '__main__':
   
    serial_no = '760180907'
   # serial_no = '760150725'
    #serial_no = '760150500'
    #serial_no = '760180685'
    #serial_no = '760150700'
    #serial_no = '760150575'

    jlink = pylink.JLink()
    #jlink = pylink.JLink(lib=pylink.library.Library("C:\Program Files (x86)\SEGGER\JLink\JLink_x64.dll"))
    jlink.open(serial_no)
    jlink.connect('nRF52832_xxAA', verbose=True)
    jlink.rtt_start()

    startedReading = False
    doneReading = False

    filename = ''
    filesize = 0

    out_file = 0

    while not doneReading:

        time.sleep(0.08)

        #code to read rtt stream
        raw_bytes = jlink.rtt_read(0,2048)
        str1 = ''
        lines = str.split(str1.join([chr(c) for c in raw_bytes]),'\n')
        for line in lines:
            if len(line)!=0:
                chars = line.split(',')
                msg_bytes = [int(c,16) for c in chars[0:-1]]      

                #first frame received
                if not startedReading:
                    s1=''      
                    for b in msg_bytes:
                        s1+=chr(b)
                    if s1.startswith("FILE_START"):
                        startedReading=True
                        info = s1.split(sep=' ')
                        filename = info[1]
                        filesize = int(info[2])
                        out_file = open("rx_"+filename, "wb")
                        print(info)
                        extra_bytes = chunk_size - len(msg_bytes)
                        #print(extra_bytes)

                #data frames
                elif not doneReading:
                    if extra_bytes>0:#get rid of extra bytes from header
                        if extra_bytes>=len(msg_bytes):
                            extra_bytes = extra_bytes - len(msg_bytes)
                            break
                        else:
                            msg_bytes = msg_bytes[extra_bytes:]

                    filesize = filesize-len(msg_bytes)
                    if filesize<=0: #get rid of extra zeros in last frame
                        doneReading=True
                        if filesize!=0:
                            msg_bytes = msg_bytes[0:filesize]
                    
                    out_file.write(bytes(msg_bytes))

                    #s1=''      
                    #for b in msg_bytes:
                    #    s1+=chr(b)
                    #print(s1,end='')

                #done with file
                else:
                    doneReading=True
                    break
    out_file.close()
