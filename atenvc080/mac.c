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
#include <sys/select.h>



serial_status_t serialOpenPort(serial_t * serialDevice, const char * path, struct termios * previousSettings)
{
    int handshake;
    struct termios options;

    *serialDevice = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (*serialDevice == -1)
        goto error;

    if (ioctl(*serialDevice, TIOCEXCL) == -1)
        goto error;

    if (fcntl(*serialDevice, F_SETFL, 0) == -1)
        goto error;

    if (previousSettings != NULL)
    {
        if (tcgetattr(*serialDevice, previousSettings) == -1)
            goto error;
    }

    if (tcgetattr(*serialDevice, &options) == -1)
        goto error;

    cfmakeraw(&options);
    // In noncanonical mode, if VMIN == 0 and VTIME == 0:
    // - if data is available, read(2) returns immediately, with the lesser of the number of bytes available, or the number of bytes requested.
    // - If no data is available, read(2) returns 0
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    cfsetispeed(&options, B9600);               // Set default input speed
    cfsetospeed(&options, B9600);               // Set default output speed
    options.c_cflag |= CS8 | CLOCAL;            // Use 8 bit words, ignore modem control lines

    if (tcsetattr(*serialDevice, TCSANOW, &options) == -1)
        goto error;

    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
    if (ioctl(*serialDevice, TIOCMSET, &handshake) == -1)
        goto error;

    return serialOK;

error:
    if (*serialDevice != -1)
        close(*serialDevice);

    *serialDevice = -1;

    return serialError;
}



serial_status_t serialClosePort(serial_t serialDevice, const struct termios * previousSettings)
{
    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver.
    // See tcsendbreak(3) <x-man-page://3/tcsendbreak> for details.
    tcdrain(serialDevice);

    // Traditionally it is good practice to reset a serial port back to
    // the state in which you found it. This is why the original termios struct
    // was saved.
    if (previousSettings != NULL)
        tcsetattr(serialDevice, TCSANOW, previousSettings);

    close(serialDevice);

    return serialOK;
}



serial_status_t serialSetRate(serial_t serialDevice, unsigned long inputRate, unsigned long outputRate)
{
#if 0
    speed_t ispeed = (speed_t) inputRate;
    speed_t ospeed = (speed_t) outputRate;


    if (ioctl(serialDevice, IOSSIOSPEED, &ispeed) == -1)
        return serialError;
#else
    struct termios options;


    if (tcgetattr(serialDevice, &options) == -1)
        return serialError;

    cfsetispeed(&options, inputRate);               // Set default input speed
    cfsetospeed(&options, outputRate);              // Set default output speed

    if (tcsetattr(serialDevice, TCSANOW, &options) == -1)
        return serialError;
#endif

    return serialOK;
}



serial_status_t serialSetRTS(int serialDevice, int state)
{
    int handshake;

    if (ioctl(serialDevice, TIOCMGET, &handshake) == -1)
        return serialError;

    if (state)
        handshake |= TIOCM_RTS;
    else
        handshake &= ~TIOCM_RTS;

    if (ioctl(serialDevice, TIOCMSET, &handshake) == -1)
        return serialError;

    return serialOK;
}



size_t serialPendingBytesCount(int serialDevice)
{
    int count;

    
    if (ioctl(serialDevice, FIONREAD, &count) == -1)
        return 0;

    if (count < 0)
        return 0;

    return count;
}



int serialReadByte(int serialDevice)
{
    uint8_t byte;
    size_t byteCount;


    if (serialPendingBytesCount(serialDevice) < 1)
        return  -1;

    byteCount = read(serialDevice, &byte, 1);
    if (byteCount < 1)
        return -1;

    return byte;
}



serial_status_t serialReadBytes(int serialDevice, uint8_t * bytes, size_t byteCount)
{
    size_t readBytes;


    while (byteCount > 0)
    {
        readBytes = read(serialDevice, bytes, byteCount);
        if (readBytes > 0)
        {
            byteCount -= readBytes;
            bytes += readBytes;
        }
    }

    return serialOK;
}



size_t serialReadPendingBytes(serial_t serialDevice, uint8_t * bytes, size_t maxByteCount)
{
    size_t byteCount = serialPendingBytesCount(serialDevice);

    if (byteCount <= 0)
        return 0;

    if (byteCount > maxByteCount)
        byteCount = maxByteCount;

    if (serialReadBytes(serialDevice, bytes, byteCount) != serialOK)
        return 0;

    return byteCount;
}



serial_status_t serialClearPendingBytes(serial_t serialDevice)
{
    uint8_t dummy[256];


    while (serialPendingBytesCount(serialDevice))
        if (serialReadPendingBytes(serialDevice, dummy, sizeof(dummy)) != serialOK)
            return serialError;

    return serialOK;
}



serial_status_t serialWriteByte(int serialDevice, uint8_t byte)
{
    if (write(serialDevice, &byte, 1) != 1)
        return serialError;

    return serialOK;
}



serial_status_t serialWriteBytes(int serialDevice, uint8_t * bytes, size_t byteCount)
{
    if (write(serialDevice, bytes, byteCount) != byteCount)
        return serialError;

    return serialOK;
}



size_t serialWaitForAvailableBytes(serial_t serialDevice, uintmax_t milliseconds)
{
    fd_set set;
    struct timeval tv;


    FD_ZERO(&set);
    FD_SET(serialDevice, &set);
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    select(serialDevice + 1, &set, NULL, NULL, &tv);    // ignore return status

    return serialPendingBytesCount(serialDevice);
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
