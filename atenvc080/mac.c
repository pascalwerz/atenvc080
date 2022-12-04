//
//  mac.c
//  atenvc080
//

// macOS specific functions
// based on <https://developer.apple.com/library/archive/samplecode/SerialPortSample/Listings/SerialPortSample_SerialPortSample_c.html#//apple_ref/doc/uid/DTS10000454-SerialPortSample_SerialPortSample_c-DontLinkElementID_4>

#include "mac.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <IOKit/serial/ioss.h>
#include <time.h>



static struct termios gOriginalTTYAttrs;

serial_status_t serialOpenPort(int * serialDescriptor, const char * path)
{
    int handshake;
    struct termios options;

    *serialDescriptor = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (*serialDescriptor == -1)
        goto error;

    if (ioctl(*serialDescriptor, TIOCEXCL) == -1)
        goto error;

    if (fcntl(*serialDescriptor, F_SETFL, 0) == -1)
        goto error;

    if (tcgetattr(*serialDescriptor, &gOriginalTTYAttrs) == -1)
        goto error;

    options = gOriginalTTYAttrs;
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    cfsetspeed(&options, B115200);      // Set 115200 b/s
    options.c_cflag |= CS8 | CLOCAL;             // Use 8 bit words

//    speed_t speed = 115200;             // Set 115200 b/s
//    if (ioctl(*serialDescriptor, IOSSIOSPEED, &speed) == -1)
//        goto error;

    if (tcsetattr(*serialDescriptor, TCSANOW, &options) == -1)
        goto error;

    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
    if (ioctl(*serialDescriptor, TIOCMSET, &handshake) == -1)
        goto error;

    return serialOK;

error:
    if (*serialDescriptor != -1)
        close(*serialDescriptor);

    *serialDescriptor = -1;

    return serialError;
}



serial_status_t serialClosePort(int serialDescriptor)
{
    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver.
    // See tcsendbreak(3) <x-man-page://3/tcsendbreak> for details.
    tcdrain(serialDescriptor);

    // Traditionally it is good practice to reset a serial port back to
    // the state in which you found it. This is why the original termios struct
    // was saved.
    tcsetattr(serialDescriptor, TCSANOW, &gOriginalTTYAttrs);

    close(serialDescriptor);

    return serialOK;
}



serial_status_t serialSetRate(int serialDescriptor, unsigned long rate)
{
    speed_t speed = (speed_t) rate;

    if (ioctl(serialDescriptor, IOSSIOSPEED, &speed) == -1)
        return serialError;

    return serialOK;
}



serial_status_t serialSetRTS(int serialDescriptor, int state)
{
    int handshake;

    if (ioctl(serialDescriptor, TIOCMGET, &handshake) == -1)
        return serialError;

    if (state)
        handshake |= TIOCM_RTS;
    else
        handshake &= ~TIOCM_RTS;

    if (ioctl(serialDescriptor, TIOCMSET, &handshake) == -1)
        return serialError;

    return serialOK;
}



size_t serialPendingBytesCount(int serialDescriptor)
{
    int count;

    
    if (ioctl(serialDescriptor, FIONREAD, &count) == -1)
        return 0;

    if (count < 0)
        return 0;

    return count;
}



int serialReadByte(int serialDescriptor)
{
    uint8_t byte;
    size_t byteCount;


    if (serialPendingBytesCount(serialDescriptor) < 1)
        return  -1;

    byteCount = read(serialDescriptor, &byte, 1);
    if (byteCount < 1)
        return -1;

    return byte;
}



serial_status_t serialReadBytes(int serialDescriptor, uint8_t * bytes, size_t byteCount)
{
    size_t readBytes;


    while (byteCount > 0)
    {
        readBytes = read(serialDescriptor, bytes, byteCount);
        if (readBytes > 0)
        {
            byteCount -= readBytes;
            bytes += readBytes;
        }
    }

    return serialOK;
}



size_t serialReadPendingBytes(serial_t serialDescriptor, uint8_t * bytes, size_t maxByteCount)
{
    size_t byteCount = serialPendingBytesCount(serialDescriptor);

    if (byteCount <= 0)
        return 0;

    if (byteCount > maxByteCount)
        byteCount = maxByteCount;

    if (serialReadBytes(serialDescriptor, bytes, byteCount) != serialOK)
        return 0;

    return byteCount;
}



serial_status_t serialWriteByte(int serialDescriptor, uint8_t byte)
{
    if (write(serialDescriptor, &byte, 1) != 1)
        return serialError;

    return serialOK;
}



serial_status_t serialWriteBytes(int serialDescriptor, uint8_t * bytes, size_t byteCount)
{
    if (write(serialDescriptor, bytes, byteCount) != byteCount)
        return serialError;

    return serialOK;
}



void pauseMilliseconds(unsigned long milliSeconds)
{
    struct timespec requestedTime;
    struct timespec remainingTime;
    int sleepStatus;


    requestedTime.tv_sec = milliSeconds / 1000;
    requestedTime.tv_nsec = (milliSeconds % 1000) * 1000000;

    sleepStatus = nanosleep(&requestedTime, &remainingTime);
    while (sleepStatus != 0)
    {
        requestedTime = remainingTime;
        sleepStatus = nanosleep(&requestedTime, &remainingTime);
    }
}
