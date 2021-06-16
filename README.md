
# EE240500 Final Project

The goal of the final project is to navigate the Boe Bot car with PING and OpenMV modules.

## Project Requirements

1. Please extend HW4's parts to design a complete task for a BB Car navigation.
2. In the task, please include XBee, line detection and location identification. You may replace the line detection task with other OpenMV image modules such as image classification with TF Lite.
3. One example task is to (1) follow the line to an object, (2) circle around the object, (3) use a AprilTag and PING to calibrate the location, (4) to follow another line to a destination. Also the car would send information with XBee at the start and end of each sub-task.


## Tech Stack

**Equipment List:** 
1. PC or notebook
2. B_L4S5I_IOT01A
3. XBee chips
4. PING
5. Encoder
6. Boe Bot Car
7. OpenMV H7 Plus board

**Server:** C++

**Host:** Python

**OpenMV:** Python

  
## Features

- Color detection - to active or not
- Line detection - go straight with line
- Distance detection - stop or go
- Parking - let BBCar parking in the correct place 
- Apriltag detection - detect and correction the angle

  
## Run Locally

### Clone the project

```bash
git clone https://github.com/SYJINTW/NTHU240500_final_project
```

### Go to the project directory

```bash
cd NTHU240500_final_project/final/
```

### Compile (Connect mbed)

```bash
mbed add https://gitlab.larc-nthu.net/ee2405_2021/bbcar
sudo mbed compile --source . --source ~/ee2405/mbed-os-build/ -m B_L4S5I_IOT01A -t GCC_ARM -f
```
  
### Running (Start host)
    
```bash
sudo python3 Xbee_host.py
```

  
## Demo

### Video

[![Demo Video](https://img.youtube.com/vi/GypTmHLBpj8/0.jpg)](https://www.youtube.com/watch?v=GypTmHLBpj8)

<!--
## FAQ

#### Question 1

Answer 1

#### Question 2

Answer 2
-->

  
## Acknowledgements

 - [OpenMV image ROI](https://book.openmv.cc/image/statistics.html)
  