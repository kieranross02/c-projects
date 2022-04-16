import time
import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys, time, math
from time import sleep

global count
global firecount

# configure the serial port
try:
    ser = serial.Serial(
        port='COM7', # Change as needed
        baudrate=115200,

        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_TWO,
        bytesize=serial.EIGHTBITS
    )
    ser.isOpen()
except:
    portlist=list(serial.tools.list_ports.comports())
    print ('Available serial ports:')
    for item in portlist:
       print (item[0])
    exit()




xsize=100

def data_gen():
    t = data_gen.t
    i=1
    count = 0
    firecount = 0
    gobcount = 0
    total = 0
    while True:
       
       t+=1
       i+=1
       strin = ser.readline()
       strin=strin.decode('utf-8')
       contains_digit = any(map(str.isdigit, strin))
           
       if(contains_digit==False):
        val = 27
       else :
        val=float(strin)

       if(val>30):
           line.set_color('red')
           print('FIRE IN THE HOUSE')
           firecount = firecount +1
           count = count +1
           total = total +val
           gobcount = gobcount +1
       else:
         #  bit = 1
           line.set_color('green')
           print('Temperature is normal @', val, end = '')
           print (' degrees')
    
           
       if (count==10):
            count = 0
            avg = (total/gobcount)
            print ('')
            print ('Statistics for today')
            print ('The temperature went above the threshold' , firecount,end = '' )
            print (' times')
            print ('average value above threshold is', avg)
            print ('')
            print ('')
            print ('')
            print ('')
            print ('')
            print ('')
            print ('')
            print ('')
            print ('')
    
           # bit = 0

       yield t, val

def run(data):
    # update the data
    t,y = data
    if t>-1:
        xdata.append(t)
        ydata.append(y)
        if t>xsize: # Scroll to the left.
            ax.set_xlim(t-xsize, t)
        line.set_data(xdata, ydata)

    return line,
def on_close_figure(event):
    sys.exit(0)


data_gen.t = -1
fig = plt.figure()
fig.canvas.mpl_connect('close_event', on_close_figure)
ax = fig.add_subplot(111)
line, = ax.plot([], [] ,lw=2)
ax.set_ylim(10, 45)
ax.set_xlim(0, xsize)
ax.grid()
xdata, ydata = [], []
plt.xlabel("Time")
plt.ylabel("Temperature °C")

ani = animation.FuncAnimation(fig, run, data_gen, blit=False, interval=15, repeat=False)
plt.show()
