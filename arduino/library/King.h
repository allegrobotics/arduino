//-*- mode: c -*-
/* 
 * NAME
 *     King - the "can't we all just get along" of Arduino modules.
 * PURPOSE
 *     Top level module for defining Arduino modules.
 *     All pseudo-self-contained Arduino modules for the Allegrobotics modules SHOULD extend this.
 *     If everyone plays nicely, many modules can co-exist in a single Arduino Nano, and then we can all just get along.
 *     There are exceptions which do not conform to this - such as the lidar lite module, and hence cannot co-exist in the same Arduino. WTF. Let the riots begin.
 * AUTHOR
 *     Scott BARNES 2019. IP freely on non-commercial applications.
 * PROTOCOL FROM HOST
 *     Each module has an assigned character, and SHOULD only respond to lines (packets) which start with that letter.
 * PROTOCOL TO HOST
 *     Any packet/line responses to the host MUST start with the assigned letter.
 * REQUIREMENTS
 *     The main .ino program MUST
 *     Create the object (obviously)
 *     Open Serial (with an appropriate baud rate, probably 19200) in its setup(), and ensure that the Serial is set up before ..
 *     Call object.setup() in its setup() for each of the co-existing objects.
 *     Call object.loop(now) in its loop() function for each of the co-existing objects.
 *     Call object.command(commandLine) whenever it receives a line/packet from the host for each of the co-existing objects.
 */

#ifndef King_h
#define King_h

class King {
public:
    King() {};
    virtual void setup() = 0;
    virtual void loop(uint32_t now) = 0;
    virtual void command(char *commandLine) = 0;
};

#endif /* King_h */
