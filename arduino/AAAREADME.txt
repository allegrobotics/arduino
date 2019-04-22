2018-11-03 21:21:30
2018-12-23 12:13:03

INTRODUCTION

This is the code which runs on Arduinos (and variants like the ESP8266) for the Allegrobotics system.

It consists of a number of libraries, usually C++ classes, which define roles for the Arduino.
In many cases the roles can co-exist, and run simultaneously in a single Arduino.
Most of the CPP roles herein follow a philosophy.

DIRECTORY STRUCTURE

* The re-usable modules are C++ classes, and the .cpp and .h files are in the library directory.
* These modules normally assume that Serial.begin(...) has been called before setup()
* These normally have a 'demonstrator' application, which is just an .ino sketch with the same lower(prefix) which can be used a simple test.
* There are also sketches named after the robots they run in, which use multiple modules.
* Some modules, such as the tfminilidarsweeper have not been written as C++ modules. It is not expected that this code can co-exist with other function due to performance requirements.

PROTOCOL BETWEEN HOST AND ARDUINO

Communication between the host and Arduino is RX/TX (via the USB port in the Nano).

1. All bytes both ways are readable characters (' ' 0x20 to '~' 0x7E)
2. Bytes are sent in packets - which are complete lines - all packets are are terminated by a LF (CR may follow, but should be ignored)
3. The first character defines the module which sent the message
   Characters are
     E General protocol error
     I General information
     D General debugging
     W General warning
     F General fatal message

     B Blinker
     H Helm
     K Kangarouter (general messages)
     M Machismow
     O Orientation (Ahrs)
     P Parking sensor
     L Lidar
     R RPM meter
     S Motor controller (Sabertooth controller, HoverboardDrive etc)
     U Imu
     W Water dispenser
     Z bumper
4. Checksum etc could be written at the end, but are optional - they are treated as part of the payload, not the packet structure.

There are a number of C++ modules, but the main program is always as .ino file.

The main program has the responsibilty of calling

Clearly this requires a reader/exploder on the host.

DISCUSSION

This is not as efficient as a binary protocol, and may be unsuitable for some high-bandwidth applications.
It has been found to be a good compromise between performance and readability.
