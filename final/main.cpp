#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"


// global variable and flag

#define MVheight 120
#define MVwidth 160
#define lineDetTolerance 5
#define middleAngle 180
#define minThreshold 90
#define maxThreshold 172
#define HALF_CIRCLE_TIME 8s
int step = 0;


// basic setting

Ticker servo_ticker;
Timer t, t_line;
PwmOut pin5(D5), pin6(D6);
BufferedSerial pc(USBTX,USBRX); //tx,rx
BufferedSerial uart(D1,D0); //tx,rx
BufferedSerial xbee(A1,A0); //tx,rx
BBCar car(pin5, pin6, servo_ticker);
DigitalInOut ping(D10);

Thread thread;


// car table

double pwm_table0[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table0[] = {-9.646, -9.784, -9.025, -8.445, -4.882, 0.000, 5.777, 10.364, 9.885, 9.895, 9.965};
double pwm_table1[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table1[] = {-8.530, -8.132, -8.690, -8.929, -4.824, 0.000, 4.829, 8.132, 8.371, 9.849, 9.769};


// function

void turning(int flag); // -1 for turning left, 1 for turning right
void ninetyDegree(int flag);
void halfCircle(int flag);
void pingStraight();    

void xbee_RPC(void);

void xbee_start(Arguments *in, Reply *out);
RPCFunction Xbee_start(&xbee_start, "xbee_start");

void driver(Arguments *in, Reply *out);
RPCFunction Driver(&driver, "driver");

void line_det(Arguments *in, Reply *out);
RPCFunction Line_det(&line_det, "line_det");

void parking(Arguments *in, Reply *out);
RPCFunction Parking(&parking, "parking");

void calib(Arguments *in, Reply *out);
RPCFunction Calib(&calib, "calib");

void stop(Arguments *in, Reply *out);
RPCFunction Stop(&stop, "stop");





int main() 
{
    // Xbee thread
    thread.start(xbee_RPC);

    // set car calibtable
    car.setCalibTable(11, pwm_table0, speed_table0, 11, pwm_table1, speed_table1); 
    
    // for Xbee
    char buf[256], outbuf[256];
    FILE *devin = fdopen(&uart, "r");
    FILE *devout = fdopen(&uart, "w");

    // receive uart data from host
    while(1) 
    {
        memset(buf, 0, 256);
        for (int i = 0; ; i++) 
        {
            char recv = fgetc(devin);
            if (recv == '\n') 
            {
                printf("\r\n");
                break;
            }
            buf[i] = fputc(recv, devout);
        }
        //Call the static call method on the RPC class
        RPC::call(buf, outbuf);
        printf("%s\r\n", outbuf);
    }
}

void xbee_RPC(void) 
{
    char buf[256], outbuf[256];
    FILE *devin = fdopen(&xbee, "r");
    FILE *devout = fdopen(&xbee, "w");

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
    }
}

void xbee_start(Arguments *in, Reply *out) 
{
    char buf_xbee[8] = "\ndriver";
    xbee.write(buf_xbee, 7); // sent driver to xbee
}

void driver(Arguments *in, Reply *out)
{   
    printf("Step: %d\n", step);
    if(step == 0)
    {
        char buf_uart[6] = "color";
        uart.write(buf_uart, 5);
        step++;
    }
    else if(step == 1)
    {
        char buf_uart[5] = "line";
        uart.write(buf_uart, 4);
        step++;
    }
    else if(step == 2)
    {
        ninetyDegree(1);
        ThisThread::sleep_for(500ms);
        halfCircle(-1);
        ThisThread::sleep_for(500ms);
        pingStraight();
        ThisThread::sleep_for(500ms);
        halfCircle(1);
        
        // call next driver
        char buf_xbee[8] = "\ndriver";
        xbee.write(buf_xbee, 7);
        step++;
    }
    else if(step == 3)
    {
        char buf_xbee[9] = "\nparking";
        xbee.write(buf_xbee, 8);
        step++;
    }
    else if(step > 3 && step < 7) // only apriltag correction three times
    {
        char buf_uart[6] = "calib"; 
        uart.write(buf_uart, 5);
        step++;
    }
    else;
}


void line_det(Arguments *in, Reply *out) 
{
    static int num = 0;
    double x1 = in->getArg<double>();
    double y1 = in->getArg<double>();
    double x2 = in->getArg<double>();
    double y2 = in->getArg<double>();


    if (num == 0) 
    {
        car.goStraight(100);
        ThisThread::sleep_for(1s);
        car.stop();
        num++;
        return;
    }
    if (x2 > MVwidth/2 - lineDetTolerance) // turn left
    {          
        car.turn(80, 0.7);
        ThisThread::sleep_for(100ms);
        car.stop();
    } 
    else if (x2 < MVwidth/2 + lineDetTolerance) // turn right
    {   
        car.turn(80, -0.7);
        ThisThread::sleep_for(100ms);
        car.stop();
    } 
    else // go straight
    {                
        car.goStraight(100);
        ThisThread::sleep_for(100ms);
        car.stop();
    }
}

// stop for 3 sec and call driver
void stop(Arguments *in, Reply *out) 
{
    car.stop();
    ThisThread::sleep_for(1s);
    char buf_xbee[8] = "\ndriver";
    xbee.write(buf_xbee, 7);
}

// -1 for left, 1 for right
void ninetyDegree(int flag)
{
    // 90 degree
    car.turn(100, flag*0.01);
    ThisThread::sleep_for(1030ms);
    car.stop();
}

void halfCircle(int flag) 
{
    // Circle
    ThisThread::sleep_for(500ms);
    car.turn(100, flag*0.5);
    ThisThread::sleep_for(HALF_CIRCLE_TIME);
    car.stop();
    return;
}

void pingStraight()
{
    // Straight
    int ping_trigger = 0;
    car.goStraight(60);
    while(1)
    {
        float val;

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
        if(val*17700.4f < 40)
        {
            ping_trigger++;
            if(ping_trigger > 15) break;
        }
        else
        {
            ping_trigger = 0;
        }

        char buf_xbee[6] = "\nping";
        xbee.write(buf_xbee, 5);
    }
    car.stop();
}

void parking(Arguments *in, Reply *out) 
{
    double d1 = in->getArg<double>();
    double d2 = in->getArg<double>();
    const char *direction = in->getArg<const char *>();

    // note that the car's dimension is
    // about 12.7cm(length) * 12.3cm(width)

    car.goStraightCalib(-6);
    ThisThread::sleep_for((d2 / 6) * 1500);
    car.stop();
    
    ThisThread::sleep_for(500ms);

    if (direction[0] == 'w') 
        car.turn(-100, -0.15);
    else if (direction[0] == 'e') 
        car.turn(-100, 0.15);
    ThisThread::sleep_for(1300ms);
    car.stop();
    ThisThread::sleep_for(500ms);

    car.goStraightCalib(-6);
    ThisThread::sleep_for((d1 / 6) * 1200);
    car.stop();
    ThisThread::sleep_for(500ms);

    // call next driver
    char buf_xbee[8] = "\ndriver";
    xbee.write(buf_xbee, 7);
}



void turning(int flag) // -1 for left, 1 for right 
{
    car.turn(100, flag*0.01);
    ThisThread::sleep_for(1300ms);
    car.stop();
    ThisThread::sleep_for(500ms);
    
    car.goStraightCalib(8);
    ThisThread::sleep_for(1000ms);
    car.stop();
    ThisThread::sleep_for(500ms);

    car.turn(100, flag*(-0.01));
    ThisThread::sleep_for(1300ms);
    car.stop();
}

void calib(Arguments *in, Reply *out) 
{
    double Dx = in->getArg<double>();
    double Dy = in->getArg<double>();
    double Dz = in->getArg<double>();
    double Rx = in->getArg<double>();
    double Ry = in->getArg<double>();
    double Rz = in->getArg<double>();

    double fix_ang;
    printf("Dz = %f\n", Dz);
    if (Ry > (middleAngle - maxThreshold) && Ry < (middleAngle - minThreshold))
    {
        double D = fabs(Dz) / tan(Ry * 3.14 / 180.0f);
        if (Dx < 0)
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, -0.01);
            for (int i = 0; i < 1.5 * (90 - Ry + fix_ang); i++)
                ThisThread::sleep_for(10ms);
        }
        else
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, -0.01);
            for (int i = 0; i < 1.5 * (90 - Ry - fix_ang); i++)
                ThisThread::sleep_for(10ms);
        }
        car.stop();
        car.goStraightCalib(10);
        for (int n = 0; n < 0.5 * D; n++)
            ThisThread::sleep_for(25ms);
        car.stop();
        car.turn(100, 0.01);
        for (int i = 0; i < 90; i++) //90 degree calibration
            ThisThread::sleep_for(12ms);
        car.stop();
    }
    else if (Ry > (middleAngle + minThreshold) && Ry < (middleAngle + maxThreshold))
    {
        double D = fabs(Dz) / tan((360 - Ry) * 3.14 / 180.0f);
        if (Dx > 0)
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, 0.01);
            for (int i = 0; i < 1.4 * (90 - (360 - Ry) + fix_ang); i++) // 360 - 10 degree calibration
                ThisThread::sleep_for(10ms);
        }
        else
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, 0.01);
            for (int i = 0; i < 1.4 * (90 - (360 - Ry) - fix_ang); i++) // 360 - 10 degree calibration
                ThisThread::sleep_for(10ms);
        }
        car.stop();
        car.goStraightCalib(11);
        for (int n = 0; n < 0.5 * D; n++)
            ThisThread::sleep_for(25ms);
        car.stop();
        car.turn(100, -0.01);
        for (int i = 0; i < 98; i++) // 90 + 8 degree calibration
            ThisThread::sleep_for(12ms);
        car.stop();
    }

    // calculate length by ping
    float val;
        
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

    ThisThread::sleep_for(10ms);

    // call next driver
    char buf_xbee[8] = "\ndriver";
    xbee.write(buf_xbee, 7);

    return;

    /*
    // Basic apriltag correction
    double deg = in->getArg<double>();
    printf("%f\r\n", deg);
    // if degree is 180, then the car is facing directly to the Apriltag
    if (deg >= minThreshold && deg <= maxThreshold)
    {
        // active ping to get length
        float val;
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
        return;
    }
    // at left
    else if (deg > maxThreshold) turning(-1);
    // at right
    else if (deg < minThreshold) turning(1);
    */
    
}
