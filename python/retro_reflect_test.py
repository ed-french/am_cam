import socket
import requests
import time
import numpy as np
from PIL import Image

SERVER="192.168.1.105"

PIC_PATH="/rgb"



def set_light(new_state:bool=True):
    command="flashfull" if new_state else "flashoff"
    form_data={command:"Set flash","exp":0,"gain":0}
    try:
        r=requests.post(f"http://{SERVER}",data=form_data)
    except requests.exceptions.ConnectionError as e:
        print(e)
    else:

        print(r.content)


def get_image_data()->np.array:



    s=socket.socket(socket.AF_INET,socket.SOCK_STREAM)

    s.connect((SERVER,80))

    s.send(f"GET {PIC_PATH} HTTP/1.0\r\n\r\n".encode("ASCII"))
    MSG_LEN=3*640*480

    img_array=np.array(MSG_LEN,dtype=np.ubyte)

    chunks=[]
    bytes_recd=0
    while bytes_recd<MSG_LEN:
        chunk=s.recv(MSG_LEN)#min(MSG_LEN-bytes_recd,2048))
        print("*")
        if chunk==b'':
            print("Stream ended prematurely")
            break
        chunks.append(chunk)
        bytes_recd+=len(chunk)

    img_bytes=b''.join(chunks)
    img_array=np.frombuffer(img_bytes,dtype=np.ubyte)

    print(f"Found total bytes: {len(img_bytes)}")



    

    return img_array

set_light(True)
time.sleep(0.1)
bright=get_image_data()
img=Image.frombuffer('RGB', (640,480), bright)
img.show()

brighter=bright.astype(np.int16)
print(f"Before conversion we have a length of : {len(bright)} after it's {len(brighter)}")
time.sleep(0.1)
set_light(False)
time.sleep(0.1)
dark=get_image_data()
img=Image.frombuffer('RGB', (640,480), dark)
img.show()
darker=dark.astype(np.int16)

print(f"Images are length: bright: {len(brighter)}, dark:{len(darker)}")

diff=(brighter-darker).clip(min=0)
img=Image.frombuffer('RGB',(640,480),diff.astype(np.ubyte))
img.show()
