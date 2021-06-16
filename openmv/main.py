import pyb, sensor, image, math, time
from machine import Timer

enable_lens_corr = False # turn on for straighter lines...
sensor.reset()
sensor.set_pixformat(sensor.RGB565) # grayscale is faster
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time = 2000)
clock = time.clock()

f_x = (2.8 / 3.984) * 160 # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * 120 # find_apriltags defaults to this if not set
c_x = 160 * 0.5 # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = 120 * 0.5 # find_apriltags defaults to this if not set (the image.h * 0.5)


# initial UART
uart = pyb.UART(3,9600,timeout_char=1000)
uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)
send = "initial"

# flag
color_det_active = False
line_det_active = False
april_det_active = False

trigger = 0
color_trigger = 0

# color Threshold
red = (0, 100, 11, 127, -128, 127)
green = (38, 100, -128, 29, 32, 127)


# All lines also have x1(), y1(), x2(), and y2() methods to get their end-points
# and a line() method to get all the above as one 4 value tuple for draw_line().
def degrees(radians):
    return (180 * radians) / math.pi


def uart_detect(s):
    global color_det_active
    global line_det_active
    global april_det_active
    if(s == b'color'):
        print("color det active\n")
        color_det_active = True
        line_det_active = False
        april_det_active = False
    elif(s == b'line'):
        color_det_active = False
        line_det_active = True
        april_det_active = False
    elif (s == b'calib'):
        color_det_active = False
        line_det_active = False
        april_det_active = True
    elif (s == b'stop'):
        color_det_active = False
        line_det_active = False
        april_det_active = False
        uart.write(("/stop/run\n").encode())
    else:
        color_det_active = color_det_active
        line_det_active = line_det_active
        april_det_active = april_det_active

def color_detection():
    global color_trigger
    #img.draw_rectangle(70, 50, 20, 20, color = (0, 0, 0), thickness = 1, fill = False)
    green_blobs = img.find_blobs([green], roi = (70, 50, 20, 20))
    if(green_blobs):
        print("find green")
        color_trigger = color_trigger + 1

    else:
        color_trigger = 0
        #print("reset")


def line_detection():
    global trigger
    trigger = trigger + 1
    for l in img.find_line_segments(merge_distance = 200, max_theta_diff = 200, roi = (20, 0 , 120, 20)):
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
        #print(("/calib/run %f %f %f %f %f %f\n" % print_args).encode())
        time.sleep(8)


while(True):
    send = uart.readline()
    uart_detect(send)
    if(send):
        print(send)
    clock.tick()
    img = sensor.snapshot()

    if enable_lens_corr:
        img.lens_corr(1.8) # for 2.8mm lens...

    if(color_det_active):
        time.sleep_ms(1000)
        color_detection()
        if(color_trigger > 5):
            uart_detect(b'stop')
            color_trigger = 0
    elif (line_det_active):
        time.sleep_ms(100)
        line_detection()
        if(trigger > 40):
            uart_detect(b'stop')
            trigger = 0
    elif (april_det_active):
        tag()


