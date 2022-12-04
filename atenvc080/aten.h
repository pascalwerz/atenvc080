//
//  aten.h
//  atenvc080
//

#ifndef aten_h
#define aten_h

#include <stdint.h>
#include <stddef.h>



typedef enum
{
    ATEN_SET_DEFAULT  = 0,
    ATEN_SET_1        = 1,
    ATEN_SET_2        = 2,
    ATEN_SET_3        = 3,
    ATEN_SET_DISPLAY  = -1,
} aten_set_id;

#define ATEN_NO_ERROR                   0
#define ATEN_INVALID                    1
#define ATEN_WRITE_ERROR                2
#define ATEN_READ_ERROR                 3

#define ATEN_BLOCK_SIZE                 128
#define ATEN_MAX_EXTENSION_COUNT        255
#define ATEN_MAX_EDID_SIZE              (ATEN_BLOCK_SIZE + ATEN_MAX_EXTENSION_COUNT * ATEN_BLOCK_SIZE)

#define ATEN_EXTENSION_COUNT_OFFSET     0x7e

#define ATEN_FIRMWARE_SIZE_1            0x40
#define ATEN_FIRMWARE_SIZE_2            0x2a40


int edidVerifyChecksum(uint8_t edid[ATEN_MAX_EDID_SIZE]);
int edidIsValid(uint8_t edid[ATEN_MAX_EDID_SIZE]);

int atenPosition(int serialDescriptor, aten_set_id setID);
int atenGetExtensionData(int serialDescriptor, int extension, uint8_t edid[ATEN_MAX_EDID_SIZE]);
int atenReadEDIDFromDisplay(int serialDescriptor, uint8_t edid[ATEN_MAX_EDID_SIZE]);
int atenCECConnect(int serialDescriptor);
int atenCECDisconnect(int serialDescriptor);
int atenWriteEDID(int serialDescriptor, uint8_t edid[ATEN_MAX_EDID_SIZE]);
int atenDeviceAttached(int serialDescriptor);       // returns -1 on error
int atenReadEDIDFromDevice(int serialDescriptor, uint8_t edid[ATEN_MAX_EDID_SIZE]);

int atenReadEDIDFromFile(uint8_t edid[ATEN_MAX_EDID_SIZE], char * path);
int atenWriteEDIDToFile(uint8_t edid[ATEN_MAX_EDID_SIZE], char * path);

int atenUpdateFirmware(int serialDescriptor, uint8_t part1[ATEN_FIRMWARE_SIZE_1], uint8_t part2[ATEN_FIRMWARE_SIZE_2]);

#endif /* aten_h */
