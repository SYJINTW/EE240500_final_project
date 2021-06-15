import pyb, sensor, image, math, time
from machine import Timer

enable_lens_corr = False # turn on for straighter lines...
sensor.reset()
sensor.set_pixformat(sensor.RGB565) # grayscale is faster
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time = 2000)
clock = time.clock()



# initial UART
uart = pyb.UART(3,9600,timeout_char=1000)
uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)
send = "initial"

# flag
line_det_active = False
april_det_active = False
color_det_active = False
trigger = 0

# color Threshold
red = (0, 100, 11, 127, -128, 127)
green = (38, 100, -128, 29, 32, 127)


# All lines also have x1(), y1(), x2(), and y2() methods to get their end-points
# and a line() method to get all the above as one 4 value tuple for draw_line().
def degrees(radians):
    return (180 * radians) / math.pi

def detection(flag):
    if(line_det_active):
        print("line\n")
    elif(april_det_active):
        print("april")


def uart_detect(s):
    global line_det_active
    global april_det_active
    if(s == b'line'):
        line_det_active = True
        april_det_active = False
    elif (s == b'calib'):
        line_det_active = False
        april_det_active = True
    elif (s == b'stop'):
        line_det_active = False
        april_det_active = False
        uart.write(("/stop/run\n").encode())
    else:
        line_det_active = line_det_active
        april_det_active = april_active

#def color_detection():



def line_detection():
    global trigger
    trigger = trigger + 1
    for l in img.find_line_segments(merge_distance = 200, max_theta_diff = 200, roi = (40, 0 , 80, 40)):
        img.draw_line(l.line(), color = (255, 255, 0))
        print_args = (l.x1(), l.y1(), l.x2(), l.y2())
        uart.write(("/line_det/run %f %f %f %f\n" % print_args).encode())
        #print(("/line_det/run %f %f %f %f\n" % print_args).encode())

def tag():
    for tag in img.find_apriltags(fx=f_x, fy=f_y, cx=c_x, cy=c_y): # defaults to TAG36H11
        img.draw_rectangle(tag.rect(), color = (255, 0, 0))
        img.draw_cross(tag.cx(), tag.cy(), color = (0, 255, 0))
        # The conversion is nearly 6.2cm to 1 -> translation
        print_args = (tag.x_translation(), tag.y_translation(), tag.z_translation(), degrees(tag.x_rotation()), degrees(tag.y_rotation()), degrees(tag.z_rotation()))
        # Translation units are unknown. Rotation units are in degrees.
        uart.write(("/calib/run %f %f %f %f %f %f\n" % print_args).encode())
        print(("/calib/run %f %f %f %f %f %f\n" % print_args).encode())
        time.sleep(10)


while(True):
    send = uart.readline()
    uart_detect(send)
    if(send):
        print(send)
    clock.tick()
    img = sensor.snapshot()
    if enable_lens_corr:
        img.lens_corr(1.8) # for 2.8mm lens...

    if (line_det_active):
        time.sleep_ms(100)
        line_detection()
        if(trigger > 150):
            uart_detect(b'stop')
    elif (april_det_active):
        tag()






while(True):
   clock.tick()
   img = sensor.snapshot()
   for i in range(10):
           x = (pyb.rng() % (2*img.width())) - (img.width()//2)
           y = (pyb.rng() % (2*img.height())) - (img.height()//2)
           w = (pyb.rng() % (img.width()//2))
           h = (pyb.rng() % (img.height()//2))
           r = (pyb.rng() % 127) + 128
           g = (pyb.rng() % 127) + 128
           b = (pyb.rng() % 127) + 128

           # 如果第一个参数是scaler，那么这个方法就需要传递x,y,w，和h。
           # 否则，它需要传递一个(x,y,w,h)元组
           img.draw_rectangle(x, y, w, h, color = (r, g, b), thickness = 2, fill = False)

   red_blobs = img.find_blobs([red])
   if(red_blobs):
    print(red_blobs)
