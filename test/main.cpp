#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"
#include "math.h"

#define minThreshold 174
#define maxThreshold 186

Ticker servo_ticker;
PwmOut pin5(D5), pin6(D6);
BufferedSerial pc(USBTX,USBRX); //tx,rx
BufferedSerial uart(D1,D0); //tx,rx
BBCar car(pin5, pin6, servo_ticker);

DigitalInOut ping(D10);
Timer t;

void calib(Arguments *in, Reply *out);
void turning(int flag); // -1 for left, 1 for right 
RPCFunction Calib(&calib, "calib");

double pwm_table0[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table0[] = {-9.646, -9.784, -9.025, -8.445, -4.882, 0.000, 5.777, 10.364, 9.885, 9.895, 9.965};
double pwm_table1[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table1[] = {-8.530, -8.132, -8.690, -8.929, -4.824, 0.000, 4.829, 8.132, 8.371, 9.849, 9.769};

void line(Arguments *in, Reply *out) {
    double x1 = in->getArg<double>();
    double y1 = in->getArg<double>();
    double x2 = in->getArg<double>();
    double y2 = in->getArg<double>();

    if (x2 > 85) {          // turn left
        car.turn(80, 0.5);
        ThisThread::sleep_for(100ms);
        car.stop();
    } else if (x2 < 75) {   // turn right
        car.turn(80, -0.5);
        ThisThread::sleep_for(100ms);
        car.stop();
    } else {                // go straight
        //car.goStraightCalib(8);
        car.goStraight(200);
        ThisThread::sleep_for(100ms);
        car.stop();
    }
}


int main() 
{
    // first and fourth argument : length of table                               
    car.setCalibTable(11, pwm_table0, speed_table0, 11, pwm_table1, speed_table1);

    char buf[256], outbuf[256];
    FILE *devin = fdopen(&uart, "r");
    FILE *devout = fdopen(&uart, "w");

    float val;

    while(1) {
        memset(buf, 0, 256);
        for (int i = 0; ; i++) {
            char recv = fgetc(devin);
            if (recv == '\n') {
                printf("\r\n");
                break;
            }
            buf[i] = fputc(recv, devout);
        }
        //Call the static call method on the RPC class
        RPC::call(buf, outbuf);
        printf("%s\r\n", outbuf);

        ping.output();
        ping = 0; wait_us(200);
        ping = 1; wait_us(5);
        ping = 0; wait_us(5);

        ping.input();
        while(ping.read() == 0);
        t.start();
        while(ping.read() == 1);
        val = t.read();
        printf("Ping = %lf\r\n", val*17700.4f);
        t.stop();
        t.reset();

        ThisThread::sleep_for(1s);
    }
}


void half_circle()
{
    static int flag = 0
    if(flag == 0)
    {
        // 90 degree
        car.turn(100, 0.01);
        ThisThread::sleep_for(135ms);
        car.stop()

        // Circle
        ThisThread::sleep_for(500ms);
        car.turn(100, -0.5);
        ThisThread::sleep_for(10s);
        car.stop();
        flag++;

        // Straight
        car.goStraight(50);
        while(1)
        {
            ping.output();
            ping = 0; wait_us(200);
            ping = 1; wait_us(5);
            ping = 0; wait_us(5);

            ping.input();
            while(ping.read() == 0);
            t.start();
            while(ping.read() == 1);
            val = t.read();
            printf("Ping = %lf\r\n", val*17700.4f);\
            t.stop();
            t.reset();
            if(val < 70) break;
        }
        car.stop();
    }
    else if(flag == 1)
    {
        car.turn(100, 0.4);
        ThisThread::sleep_for(10s);
        car.stop();
        flag--;
    }
    else;
    return;
} 

