//
//  main.c
//  atenvc080
//

#define VERSION "0.3"

#include "mac.h"
#include "aten.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>



void usage(void);
void connectToDevice(serial_t *serialDevice, char * path, struct termios * previousSettings);
void checkSerialDevice(serial_t serialDevice);
void printInquiry(serial_t serialDevice);
void selectSet(serial_t serialDevice, int *currentPosition, char * name);
void writeEDIDToDevice(serial_t serialDevice, int currentPosition, char * path);
void writeEDIDToFile(serial_t serialDevice, int currentPosition, char * path);
void firmwareUpdate(serial_t serialDevice, char * path);



void usage(void)
{
    //               1         2         3         4         5         6         7         8
    //      12345678901234567890123456789012345678901234567890123456789012345678901234567890
    printf("usage: atenvc080 -d serial [options]\n");
    printf("\n");
    printf("       options: (options are executed from left to right)\n");
    printf("       -d device     mandatory option that must come first.\n");
    printf("                     'device' is the path to the device connected to\n");
    printf("                       ATEN VC080 RS232 port\n");
    printf("                     e.g. -d /dev/cu.usbserial-...\n");
    printf("       -q            identify device type\n");
    printf("       -s set        select and switch set\n");
    printf("                     set can be one of:\n");
    printf("                       DEFAULT  select and switch emulator to DEFAULT set\n");
    printf("                                DEFAULT  EDID can't be modified\n");    // in fact, it seems it can!
    printf("                       1-3      select and switch emulator to SET 1-3\n");
    printf("                       DISPLAY  the connected display\n");
    printf("                                -s DISPLAY option is only useful to read the\n");
    printf("                                connected display's EDID. Beware that KVM\n");
    printf("                                or similar switches may return a modified\n");
    printf("                                display EDID\n");
    printf("       -r path       read selected set, write it to EDID file at path\n");
    printf("       -w path       read EDID file at path and write it to selected device set\n");
    printf("       -C            CEC connect\n");
    printf("       -D            CEC disconnect\n");
    printf("       -F path       update device with firmware file at path\n");
    printf("       -?            print this help\n");
}



void checkSerialDevice(serial_t serialDevice)
{
    if (serialDevice < 0)
    {
        printf("serial device not open.\n");
        exit(1);
    }
}



void printInquiry(serial_t serialDevice)
{
    checkSerialDevice(serialDevice);

    int byte = atenDeviceAttached(serialDevice);
    if (byte < 0)
        printf("device didn't reply to identification request\n");
    else
        switch(byte)
        {
        case 0x10: printf("device type is \"VC010 VGA EDID emulator\"\n"); break;
        case 0x60: printf("device type is \"VC060 DVI EDID emulator\"\n"); break;
        case 0x80: printf("device type is \"VC080 HDMI EDID emulator\"\n"); break;
        default:   printf("unknown device type %u (0x%02x)\n", byte, byte); break;
        }
}



void selectSet(serial_t serialDevice, int *currentPosition, char * name)
{
    checkSerialDevice(serialDevice);

    if (strcmp(name, "default") == 0)      *currentPosition = ATEN_SET_DEFAULT;
    else if (strcmp(name, "DEFAULT") == 0) *currentPosition = ATEN_SET_DEFAULT;
    else if (strcmp(name, "1") == 0)       *currentPosition = ATEN_SET_1;
    else if (strcmp(name, "2") == 0)       *currentPosition = ATEN_SET_2;
    else if (strcmp(name, "3") == 0)       *currentPosition = ATEN_SET_3;
    else if (strcmp(name, "display") == 0) *currentPosition = ATEN_SET_DISPLAY;
    else if (strcmp(name, "DISPLAY") == 0) *currentPosition = ATEN_SET_DISPLAY;
    else
    {
        printf("unknown set name '%s'\n", name);
        exit(1);
    }

    switch(*currentPosition)
    {
    case ATEN_SET_DEFAULT:
        atenPosition(serialDevice, *currentPosition);       // dismiss errors
        printf("switched to DEFAULT\n");
        break;
    case ATEN_SET_1:
        atenPosition(serialDevice, *currentPosition);       // dismiss errors
        printf("switched to SET 1\n");
        break;
    case ATEN_SET_2:
        atenPosition(serialDevice, *currentPosition);       // dismiss errors
        printf("switched to SET 2\n");
        break;
    case ATEN_SET_3:
        atenPosition(serialDevice, *currentPosition);       // dismiss errors
        printf("switched to SET 3\n");
        break;
    case ATEN_SET_DISPLAY:
        printf("selected DISPLAY\n");
        break;
    }
}



void CECConnect(serial_t serialDevice)
{
    checkSerialDevice(serialDevice);

    atenCECConnect(serialDevice);
}



void CECDisconnect(serial_t serialDevice)
{
    checkSerialDevice(serialDevice);

    atenCECDisconnect(serialDevice);
}



void writeEDIDToDevice(serial_t serialDevice, int currentPosition, char * path)
{
    uint8_t edid[ATEN_MAX_EDID_SIZE];


    checkSerialDevice(serialDevice);

    if (currentPosition == ATEN_SET_DISPLAY)
    {
        printf("can't write display's EDID\n");
        exit(1);
    }

    if (currentPosition == ATEN_SET_DEFAULT)
    {
        // ATEN EDID Wizard does not allow this, so don't do it either, although DEFAULT is just a normal writable set
        printf("can't write DEFAULT set\n");
        exit(1);
    }

    if (atenReadEDIDFromFile(edid, path) != ATEN_NO_ERROR || edidIsValid(edid) != ATEN_NO_ERROR)
    {
        printf("invalid EDID file\n");
        exit(1);
    }

    printf("writing...\n");
    if (atenWriteEDID(serialDevice, edid) != 0)
    {
        printf("write failed\n");
        exit(1);
    }

    switch(currentPosition)
    {
    case ATEN_SET_DEFAULT:
        printf("written to DEFAULT set\n");
        break;
    case ATEN_SET_1:
        printf("written to SET 1\n");
        break;
    case ATEN_SET_2:
        printf("written to SET 2\n");
        break;
    case ATEN_SET_3:
        printf("written to SET 3\n");
        break;
    }
}



void writeEDIDToFile(serial_t serialDevice, int currentPosition, char * path)
{
    int status;
    uint8_t edid[ATEN_MAX_EDID_SIZE];


    checkSerialDevice(serialDevice);

    printf("reading...\n");
    if (currentPosition != ATEN_SET_DISPLAY)
        status = atenReadEDIDFromDevice(serialDevice, edid);
    else
        status = atenReadEDIDFromDisplay(serialDevice, edid);

    if (status != ATEN_NO_ERROR)
    {
        printf("can't read EDID\n");
        if (currentPosition == ATEN_SET_DISPLAY)
            printf("Is monitor connected?\n");
        exit(1);
    }

    status = atenWriteEDIDToFile(edid, path);
    if (status == ATEN_NO_ERROR)
    {
        printf("EDID written to '%s'\n", path);
    }
    else
    {
        printf("file write error\n");
        exit(1);
    }
}



void connectToDevice(int *serialDevice, char * path, struct termios * previousSettings)
{
    if (serialOpenPort(serialDevice, optarg, previousSettings) != serialOK)
    {
        perror(optarg);
        exit(1);
    }
    if (serialSetRate(*serialDevice, 115200, 115200) != serialOK || serialSetRTS(*serialDevice, 0) != serialOK)
    {
        printf("Can't initialize serial port\n");
        exit(1);
    }
    pauseMilliseconds(100);
    serialClearPendingBytes(*serialDevice);      // purge serial input buffer, dismiss errors

    printf("Connected using '%s'\n", path);
}



void firmwareUpdate(serial_t serialDevice, char * path)
{
    int fileDescriptor;
    size_t byteCount;
    uint8_t data[ATEN_FIRMWARE_SIZE_1 + ATEN_FIRMWARE_SIZE_2 + 2];


    fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror(path);
        exit(1);
    }

    byteCount = read(fileDescriptor, data, ATEN_FIRMWARE_SIZE_1 + ATEN_FIRMWARE_SIZE_2 + 2);
    if (byteCount != ATEN_FIRMWARE_SIZE_1 + ATEN_FIRMWARE_SIZE_2 + 2)
    {
        perror(path);
        close(fileDescriptor);
        exit(1);
    }

    printf("Updating firmware...\n");
    int status = atenUpdateFirmware(serialDevice, data, byteCount);
    switch (status)
    {
    case ATEN_NO_ERROR:
        printf("Firmware updated\n");
        break;

    case ATEN_READ_ERROR:
        printf("Firmware update failed.\n");
        printf("Is EDID emulator in firmware update mode?\n");
        break;

    case ATEN_INVALID:
        printf("%s: not a valid firmware file\n", path);
        printf("Is it still scrambled?\n");
        break;

    default:
        printf("Firmware update failed.\n");
        break;
    }
}



int main(int argc, char * const argv[])
{
    serial_t serialDevice = serialClosed;
    int currentPosition = ATEN_SET_DISPLAY;
    int character;
    serialSettings_t previousSettings;

    printf("atenvc080 v%s\n", VERSION);

    while ((character = getopt(argc, argv, "?CDF:d:qr:s:w:")) != -1)
    {
        switch(character)
        {
        case 'C':
            CECConnect(serialDevice);
            break;
        case 'D':
            CECDisconnect(serialDevice);
            break;
        case 'F':
            firmwareUpdate(serialDevice, optarg);
            break;
        case 'd':
            connectToDevice(&serialDevice, optarg, &previousSettings);
            break;
        case 'q':
            printInquiry(serialDevice);
            break;
        case 'r':
            writeEDIDToFile(serialDevice, currentPosition, optarg);
            break;
        case 's':
            selectSet(serialDevice, &currentPosition, optarg);
            break;
        case 'w':
            writeEDIDToDevice(serialDevice, currentPosition, optarg);
            break;
        case '?':
        default:
            usage();
            exit(1);
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 0)
    {
        usage();
        exit(1);
    }
    if (serialDevice != -1)
        serialClosePort(serialDevice, &previousSettings);       // dismiss errors

    return 0;
}
