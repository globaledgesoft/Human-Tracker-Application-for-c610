#Project Title: Human Tracker Application for c610
<!-- TABLE OF CONTENTS -->
## Table of Contents

* [About the Project](#about-the-project)
* [Prerequisites](#prerequisites)
* [Getting Started](#getting-started)
  * [Steps to build OpenCV Library](#Steps-to-build-OpenCV-Library)
  * [Steps to Build Boto3](#Steps-to-Build-Boto3)
  * [Set up AWS IoT Thing Object](#Set-up-AWS-IoT-Thing-Object)
  * [Steps to Build Main Application](#Steps-to-Build-Main-Application)
* [Usage](#usage)
* [Contributors](#contributors)

<!-- ABOUT THE PROJECT -->
## About The Project

Human Tracker Application is a project designed to utilize the Qualcomm’s SNPE on Thundercomm TurboX (based on the Qualcomm QCS 610 SoC). This project is designed to detect any human presence inside a Region of Interest (Python script included for defining the Region of interest) and publish MQTT messages to AWS IoT on frequent intervals. 
On starting the application, DLC model is loaded (Here SSD MobileNet V2) onto specified runtime (CPU, GPU and DSP Supported). Next the application takes camera input (using gstreamer plugins) and feeds the frame to NPE to get inference. After post processing, the total count of the people in the frame and also the count of people inside the region of interest is printed. Depending on the frame interval count given, the application publishes a MQTT message to AWS IoT.

## Prerequisites

1. Install Android Platform tools (ADB, Fastboot) 

2.  Flash the  firmware image onto the board.

3. Setup the Wifi (Page 12 of [Linux User Guide](https://www.thundercomm.com/app_en/product/1593776185472315))

4. Download and Install the App Tool chain SDK. (Setup instructions can be found in [Application SDK User Manual](https://www.thundercomm.com/app_en/product/1593776185472315)).

5. Setup the [SNPE SDK](https://developer.qualcomm.com/docs/snpe/setup.html) in the host system. You can download SNPE SDK [here](https://developer.qualcomm.com/software/qualcomm-neural-processing-sdk).


<!-- GETTING STARTED -->
## Getting Started

To get a local copy up and running follow these steps.
1. Clone the  project repository from the github to host system.
```sh
$ git clone <project-repo>
$ cd qcs610-human-tracker
```

2. Create a directory where we will place executable and other necessary files for running the application and push to board.
```sh
$ mkdir human-tracker-build
```

3. Create a directory named opencv and libboto3 for keeping header and library files of opencv and boto3 inside human-tracker-build.
```sh
$ cd human-tracker-build
$ mkdir opencv
$ mkdir libboto3
```

### Steps to build OpenCV Library
To build opencv library for the target board in yocto environment , utilize the meta recipes for opencv present in the file poky/meta-openembedded/meta-oe/recipes-support/opencv/opencv_3.4.5.bb.
1. To integrate an opencv, first we have to source the cross compilation environment  in the host system. For that navigate to yocto working directory and execute below commands:
```sh
<working directory>$ export  SHELL=/bin/bash
<working directory>$ source poky/qti-conf/set_bb_env.sh
```

2. Select “qcs610-odk meta-qti-bsp” from the available machines menu. On selecting the machine, the next ncurses menu starts. Select “qti-distro-fullstack-perf meta-qti-bsp” from the Available Distributions  menu. After this shell prompt will move to following directory(Build directory): <working directory>/build-qti-distro-fullstack-perf 

3. As mentioned earlier bb file of opencv is already present and we just need to build the image by running bitbake opencv:
```sh
<build_directory>/$ bitbake opencv
```

4. After the completion of the build process, copy the header files and library files to the opencv directory which we have created earlier in the project directory.
```sh
$ cp -r <build_directory>/tmp-glibc/sysroots-components/armv7ahf-neon/opencv/usr/lib/ <build_directory>/tmp-glibc/sysroots-components/armv7ahf-neon/opencv/usr/include/  ~/qcs610-human-tracker/human-tracker-build/opencv
```
### Note: 
 - For more reference refer to the “QCS610/QCS410 Linux Platform Development Kit Quick Start Guide document”.
 - Also make sure install the all the dependency library from the yocto build to the system (ex: libgphoto2, libv4l-utils) 
 - bb recipes of above dependent libraries are available inside meta-oe layer you can directly run bitbake command

### Steps to Build Boto3
To install the boto3 package on qcs610, you need to build and install the following python3 packages:

- python3-boto3
- python3-botocore
- python3-jmespath
- python3-s3transfer

Steps to build Boto3 are as follows:
1. Meta recipes of the packages mentioned above can be found in meta-recipe of source directory. Place the above mentioned bb recipe in the given directory. "poky/poky/meta-openembedded/meta-python/python/".

2. Run the bitbake command for installing boto3 packages.
```sh
<working directory>/build-qti-distro-fullstack-perf $ bitbake python3-boto3
```

3. Once the build is complete, the shared library and header files will be available in “./tmp-glibc/sysroots-components/armv7ahf-neon/python3-boto3/usr”.  Copy these files to the bot3 directory in human-tracker-build.
```sh
$ cp ./tmp-glibc/sysroots-components/armv7ahf-neon/python3-boto3/usr/lib ~/qcs610-human-tracker/human-tracker-build/boto3
```
4. Make sure you have copied and transfer all the dependency library require for building the boto3 package. Required libraries are mentioned in ‘RDEPENDS’ section of bitbake file(.bb file or .inc file).

### Set up AWS IoT Thing Object
1. Refer the following link for setting up AWS IoT Thing Object.
https://docs.aws.amazon.com/iot/latest/developerguide/iot-quick-start.html
Select python as the device SDK in the console.

Note down the aws access key id, secret access key, region and endpoint and update it in the aws_config.py file in the project directory. Also add a topic name for publishing the MQTT message.

Now copy aws_config.py and aws_send.py to the human-tracker-build folder.
```sh
$ cp aws_config.py aws_send.py human-tracker-build
```

### Steps to Build Main Application
1. Move to the project directory in the host system.
```sh
$ cd  ~/qcs610-human-tracker
```
2. Run the makefile. 
Note: Make sure  App Toolchain SDK for cross compilation is ready. This can be done by execuiting below code.
```
   $ source /usr/local/oecore-x86_64/environment-setup-armv7ahf-neon-oe-linux-gnueabi
```
Check if SNPE_ROOT environment variable is set to root folder of snpe by executing the following command. If not, setup SNPE as specified in the prerequisite.
```sh
$ echo $SNPE_ROOT
$ make -f Makefile.arm-oe-linux-gcc8.2h 
```
3. If build is successful, copy the executable and assets directory to human-tracker-build directory.
```sh
$ cp obj/local/arm-oe-linux-gcc8.2hf/human-tracker ./human-tracker-build
$ cp -r assets ./human-tracker-build
```
4. Now push this new directory containing executable and other necessary files to the target board.
```sh
$ adb push human-tracker-build /data
```

<!-- USAGE -->
## Usage
Human tracker application can be run in 3 modes. One for getting reference image, another for testing a single image and the last for running with camera stream. Explanations for flags supported by the application are as follows:
Required Argument:
-m <Model-Path>: Give path of DLC File. 
### Note: 
 - You can avoid this while running to get a reference frame.

Optional Arguments:
-r <Runtime> :Specify the Runtime to run network Eg, CPU(0), GPU(1), DSP(2), AIP(3)
-f <Reference-Image-Name>: A reference image will be stored with the given name.
-c <Count>: Specify the Frame interval to send info to aws (default: 15).
-i <Image-Path>: Specify the path of the test frame. If not given, takes camera stream.

Before running the application set the board environment by following steps:
1. In the host system, enter following adb commands
```sh
$ adb root
$ adb remount
$ adb shell mount -o remount
```

2. Enable Wifi connectivity on the board by executing following commands
```
$ adb shell
/# wpa_supplicant -Dnl80211 -iwlan0 -c /etc/misc/wifi/wpa_supplicant.conf -ddddt &
/# dhcpcd wlan0
```

3. Set the latest UTC time and date in the following format.
```
/# date -s '2021-02-20 10:07:00'
```

4. Export the shared library to the LD_LIBRARY_PATH
```   
   /# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/human-tracker-build/opencv
   /# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/human-tracker-build/libboto3
```

For defining the region of interest, we need to get a reference image from the camera of TurboX. Steps for obtaining this are as follows:
1. Access the board through adb
```sh
$ adb shell
```

2. Move to human-tracker-app directory
```
/#  cd /data/human-tracker-build
```

3. Run the application with -f flag
```
/#  ./human-tracker -f reference_frame.jpg
```
4. Pull the reference_frame.jpg from board to host system
```
/# exit
```

5. Back to the project directory in the host system.
```sh
$ adb pull   /data/human-tracker-build/reference_frame.jpg .
```

Now that we have a reference image, we need to define the coordinates of the Region of interest in config.json. This can be done by running mark_roi.py python script.
1. In host system run
```sh
$ python3 mark_roi.py -p <path-to-sample-image>
```

2. Now mark an area using the mouse. Press ‘s’ to save config.json and ‘ESC’ to quit the script.

3. Now push the config.json to target
```sh
$ adb push config.json /data/human-tracker-build
```

Steps for running the main application are as follows:

1. Access the board through adb
```sh
$ adb shell
```

2. Move to human-tracker-app directory
```
/# cd /data/human-tracker-build
```
3. To run the application taking camera stream as input and publish MQTT message containing the people count and time in the interval of 10 frames use the following
```
/# ./human-tracker -m ./assets/mobilenet_ssd.dlc <runtime> -c 10
```
Note: if -c is not specified, the application sends info to aws on every 15 frames.
Each frame containing info such as total count of people, count of people inside region of interest and time will be stored in video.mp4 

4. In case you need to test with a single test image, use the following command. Output file named output.jpg will be created in the present working directory. A test image named input_frame.jpg is given in the project directory for testing.
```
./human-tracker -m ./assets/mobilenet_ssd.dlc <runtime> -i <image-path>
```

<!-- ## Contributors -->
## Contributors
* [Rakesh Sankar](s.rakesh@globaledgesoft.com)
* [Steven P](ss.pandiri@globaledgesoft.com)
* [Ashish Tiwari](t.ashish@globaledgesoft.com)
* [Arunraj A P](ap.arunraj@globaledgesoft.com)






