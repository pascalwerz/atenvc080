//
//  mac.h
//  atenvc080
//

// macOS specific functions

#ifndef serial_h
#define serial_h

#include <stdint.h>
#include <stddef.h>
#include <sys/termios.h>



typedef int serial_t;
#define serialClosed        ((serial_t) -1)

typedef struct termios serialSettings_t;

typedef enum
{
    serialOK = 0,
    serialError,
} serial_status_t;



serial_status_t serialOpenPort(serial_t * serialDevice, const char * path, serialSettings_t * previousSettings);
serial_status_t serialClosePort(serial_t serialDevice, const serialSettings_t * previousSettings);
serial_status_t serialSetRate(serial_t serialDevice, unsigned long inputRate, unsigned long outputRate);
serial_status_t serialSetRTS(serial_t serialDevice, int state);
size_t serialPendingBytesCount(serial_t serialDevice);
int serialReadByte(serial_t serialDevice);              // returns -1 if no byte is available
serial_status_t serialReadBytes(serial_t serialDevice, uint8_t * bytes, size_t byteCount);
size_t serialReadPendingBytes(serial_t serialDevice, uint8_t * bytes, size_t maxByteCount);
serial_status_t serialClearPendingBytes(serial_t serialDevice);
serial_status_t serialWriteByte(serial_t serialDevice, uint8_t byte);
serial_status_t serialWriteBytes(serial_t serialDevice, uint8_t * bytes, size_t byteCount);
size_t serialWaitForAvailableBytes(serial_t serialDevice, uintmax_t milliseconds);

void pauseMilliseconds(unsigned long milliSeconds);

#endif /* serial_h */
