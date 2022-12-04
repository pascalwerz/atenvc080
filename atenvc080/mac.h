//
//  mac.h
//  atenvc080
//

// macOS specific functions

#ifndef serial_h
#define serial_h

#include <stdint.h>
#include <stddef.h>


typedef int serial_t;
#define serialClosed        ((serial_t) -1)

typedef enum
{
    serialOK = 0,
    serialError,
} serial_status_t;



serial_status_t serialOpenPort(serial_t * serialDescriptor, const char * path);
serial_status_t serialClosePort(serial_t serialDescriptor);
serial_status_t serialSetRate(serial_t serialDescriptor, unsigned long rate);
serial_status_t serialSetRTS(serial_t serialDescriptor, int state);
size_t serialPendingBytesCount(serial_t serialDescriptor);
int serialReadByte(serial_t serialDescriptor);              // returns -1 if no byte is available
serial_status_t serialReadBytes(serial_t serialDescriptor, uint8_t * bytes, size_t byteCount);
size_t serialReadPendingBytes(serial_t serialDescriptor, uint8_t * bytes, size_t maxByteCount);
serial_status_t serialWriteByte(serial_t serialDescriptor, uint8_t byte);
serial_status_t serialWriteBytes(serial_t serialDescriptor, uint8_t * bytes, size_t byteCount);

void pauseMilliseconds(unsigned long milliSeconds);

#endif /* serial_h */
