#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"

#define minThreshold 174
#define maxThreshold 186


// global variable and flag

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


// car table

double pwm_table0[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table0[] = {-9.646, -9.784, -9.025, -8.445, -4.882, 0.000, 5.777, 10.364, 9.885, 9.895, 9.965};
double pwm_table1[] = {-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150};
double speed_table1[] = {-8.530, -8.132, -8.690, -8.929, -4.824, 0.000, 4.829, 8.132, 8.371, 9.849, 9.769};


// function

void turning(int flag);

void xbee_RPC(void);

void xbee_start(Arguments *in, Reply *out);
RPCFunction Xbee_start(&xbee_start, "xbee_start");

void driver(Arguments *in, Reply *out);
RPCFunction Driver(&driver, "driver");

void color_det(Arguments *in, Reply *out);
RPCFunction Color_det(&color_det, "color_det");

void line_det(Arguments *in, Reply *out);
RPCFunction Line_det(&line_det, "line_det");

void circle(Arguments *in, Reply *out);
RPCFunction Circle(&circle, "circle");

void parking(Arguments *in, Reply *out);
RPCFunction Parking(&parking, "parking");

void calib(Arguments *in, Reply *out);
RPCFunction Calib(&calib, "calib");

void stop(Arguments *in, Reply *out);
RPCFunction Stop(&stop, "stop");

Thread thread;


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

void xbee_RPC(void) {
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
    xbee.write(buf_xbee, 7); // sent xbee to driver
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
        char buf_xbee[8] = "\ncircle";
        xbee.write(buf_xbee, 7);
        step++;
    }
}

int first = 0;

void line_det(Arguments *in, Reply *out) 
{

    static int num = 0;

    double x1 = in->getArg<double>();
    double y1 = in->getArg<double>();
    double x2 = in->getArg<double>();
    double y2 = in->getArg<double>();


    if (first == 0) {
        car.goStraight(100);
        ThisThread::sleep_for(1s);
        car.stop();
        first++;
        return;
    }
    if (x2 > 85) {          // turn left
        car.turn(80, 0.7);
        ThisThread::sleep_for(100ms);
        car.stop();
    } else if (x2 < 75) {   // turn right
        car.turn(80, -0.7);
        ThisThread::sleep_for(100ms);
        car.stop();
    } else {                // go straight
        car.goStraight(100);
        ThisThread::sleep_for(100ms);
        car.stop();
    }
}

void color_det(Arguments *in, Reply *out)
{

}

// stop for 3 sec and call driver
void stop(Arguments *in, Reply *out) 
{
    car.stop();
    ThisThread::sleep_for(3s);
    char buf_xbee[8] = "\ndriver";
    xbee.write(buf_xbee, 7);
}

void circle(Arguments *in, Reply *out) {
    static int flag = 0;
    if(flag == 0)
    {
        // 90 degree
        car.turn(100, 0.01);
        ThisThread::sleep_for(1030ms);
        car.stop();

        // Circle
        ThisThread::sleep_for(500ms);
        car.turn(100, -0.5);
        ThisThread::sleep_for(10s);
        car.stop();
        flag++;

        // Straight
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
            if(val*17700.4f < 70) break;

            char buf_xbee[6] = "\nloop";
            xbee.write(buf_xbee, 5);
        }
        car.stop();

        ThisThread::sleep_for(500ms);
        char buf_xbee[8] = "\ncircle";
        xbee.write(buf_xbee, 7);
    }
    else if(flag == 1)
    {
        car.turn(100, 0.4);
        ThisThread::sleep_for(10s);
        car.stop();
        flag--;

        ThisThread::sleep_for(500ms);
        char buf_xbee[9] = "\nparking";
        xbee.write(buf_xbee, 8);
    }
    return;
}

void parking(Arguments *in, Reply *out) {
    double d1 = in->getArg<double>();
    double d2 = in->getArg<double>();
    const char *direction = in->getArg<const char *>();

    // note that the car's dimension is
    // about 12.7cm(length) * 12.3cm(width)

    car.goStraightCalib(-6);
    ThisThread::sleep_for((d2 / 6) * 1500);
    car.stop();
    
    ThisThread::sleep_for(500ms);

    if (direction[0] == 'w') {
        car.turn(-100, -0.15);
    } else if (direction[0] == 'e') {
        car.turn(-100, 0.15);
    }
    ThisThread::sleep_for(1300);
    car.stop();
    ThisThread::sleep_for(500ms);

    car.goStraightCalib(-6);
    ThisThread::sleep_for((d1 / 6) * 1200);
    car.stop();
    ThisThread::sleep_for(500ms);

    char buf_uart[6] = "calib", buf_xbee[6] = "\nstop";
    uart.write(buf_uart, 5);
    xbee.write(buf_xbee, 5);
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

void calib(Arguments *in, Reply *out) {
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
    /*
    double Dx = in->getArg<double>();
    double Dy = in->getArg<double>();
    double Dz = in->getArg<double>();
    double Rx = in->getArg<double>();
    double Ry = in->getArg<double>();
    double Rz = in->getArg<double>();

    double fix_ang;
    printf("Dz = %f\n", Dz);
    if (Ry > 8 && Ry < 90)
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
        for (int i = 0; i < 90; i++) //90 + 3 degree calibration
            ThisThread::sleep_for(12ms);
        car.stop();
    }
    else if (Ry > 270 && Ry < 352)
    {
        double D = fabs(Dz) / tan((360 - Ry) * 3.14 / 180.0f);
        if (Dx > 0)
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, 0.01);
            for (int i = 0; i < 1.4 * (90 - (360 - Ry) + fix_ang); i++) //360 - 10 degree ca
                ThisThread::sleep_for(10ms);
        }
        else
        {
            fix_ang = atan(fabs(Dx) / fabs(Dz));
            car.turn(100, 0.01);
            for (int i = 0; i < 1.4 * (90 - (360 - Ry) - fix_ang); i++) //360 - 10 degree ca
                ThisThread::sleep_for(10ms);
        }
        car.stop();
        car.goStraightCalib(11);
        for (int n = 0; n < 0.5 * D; n++)
            ThisThread::sleep_for(25ms);
        car.stop();
        car.turn(100, -0.01);
        for (int i = 0; i < 98; i++) //90 + 8 degree calibration
            ThisThread::sleep_for(12ms);
        car.stop();
    }

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
    */
}

/*void dis_check(void) {
    while (1) {
        printf("hi\r\n");
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
        ping_dis = val*17700.4f;
        printf("Ping = %lf\r\n", ping_dis);
        t.stop();
        t.reset();
        ThisThread::sleep_for(10ms);
    }
}
*/