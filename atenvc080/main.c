//
//  main.c
//  atenvc080
//

#define VERSION "0.1"

#include "mac.h"
#include "aten.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>



void usage(void);
void connectToDevice(serial_t *serialDescriptor, char * path);
void checkSerialDevice(serial_t serialDescriptor);
void printInquiry(serial_t serialDescriptor);
void selectSet(serial_t serialDescriptor, int *currentPosition, char * name);
void writeEDIDToDevice(serial_t serialDescriptor, int currentPosition, char * path);
void writeEDIDToFile(serial_t serialDescriptor, int currentPosition, char * path);
void firmwareUpdate(serial_t serialDescriptor, char * path);



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
    printf("       -w path       read EDID file at path and write it to selected set\n");
    printf("       -C            CEC connect\n");
    printf("       -D            CEC disconnect\n");
    printf("       -F path       Update device with firmware at path\n");
    printf("       -?            print this help\n");
}



void checkSerialDevice(serial_t serialDescriptor)
{
    if (serialDescriptor < 0)
    {
        printf("serial device not open.\n");
        exit(1);
    }
}



void printInquiry(serial_t serialDescriptor)
{
    checkSerialDevice(serialDescriptor);

    int byte = atenDeviceAttached(serialDescriptor);
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



void selectSet(serial_t serialDescriptor, int *currentPosition, char * name)
{
    checkSerialDevice(serialDescriptor);

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
        atenPosition(serialDescriptor, *currentPosition);       // dismiss errors
        printf("switched to DEFAULT\n");
        break;
    case ATEN_SET_1:
        atenPosition(serialDescriptor, *currentPosition);       // dismiss errors
        printf("switched to SET 1\n");
        break;
    case ATEN_SET_2:
        atenPosition(serialDescriptor, *currentPosition);       // dismiss errors
        printf("switched to SET 2\n");
        break;
    case ATEN_SET_3:
        atenPosition(serialDescriptor, *currentPosition);       // dismiss errors
        printf("switched to SET 3\n");
        break;
    case ATEN_SET_DISPLAY:
        printf("selected DISPLAY\n");
        break;
    }
}



void CECConnect(serial_t serialDescriptor)
{
    checkSerialDevice(serialDescriptor);

    atenCECConnect(serialDescriptor);
}



void CECDisconnect(serial_t serialDescriptor)
{
    checkSerialDevice(serialDescriptor);

    atenCECDisconnect(serialDescriptor);
}



void writeEDIDToDevice(serial_t serialDescriptor, int currentPosition, char * path)
{
    uint8_t edid[ATEN_MAX_EDID_SIZE];


    checkSerialDevice(serialDescriptor);

    if (currentPosition == ATEN_SET_DISPLAY)
    {
        printf("can't write display's EDID\n");
        exit(1);
    }

    if (currentPosition == ATEN_SET_DEFAULT)
    {
        printf("can't write DEFAULT set\n");
        exit(1);
    }

    if (atenReadEDIDFromFile(edid, path) != ATEN_NO_ERROR || edidIsValid(edid) != ATEN_NO_ERROR)
    {
        printf("invalid EDID file\n");
        exit(1);
    }

    printf("writing...\n");
    if (atenWriteEDID(serialDescriptor, edid) != 0)
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



void writeEDIDToFile(serial_t serialDescriptor, int currentPosition, char * path)
{
    int status;
    uint8_t edid[ATEN_MAX_EDID_SIZE];


    checkSerialDevice(serialDescriptor);

    printf("reading...\n");
    if (currentPosition != ATEN_SET_DISPLAY)
        status = atenReadEDIDFromDevice(serialDescriptor, edid);
    else
        status = atenReadEDIDFromDisplay(serialDescriptor, edid);

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



void connectToDevice(int *serialDescriptor, char * path)
{
    if (serialOpenPort(serialDescriptor, optarg) != serialOK)
    {
        perror(optarg);
        exit(1);
    }
    printf("Connected using '%s'\n", path);
}



void firmwareUpdate(serial_t serialDescriptor, char * path)
{
    int fileDescriptor;
    size_t byteCount;
    uint8_t part1[ATEN_FIRMWARE_SIZE_1];
    uint8_t part2[ATEN_FIRMWARE_SIZE_2];


    fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror(path);
        exit(1);
    }

    byteCount = read(fileDescriptor, part1, ATEN_FIRMWARE_SIZE_1);
    if (byteCount != ATEN_FIRMWARE_SIZE_1)
    {
        perror(path);
        close(fileDescriptor);
        exit(1);
    }

    byteCount = read(fileDescriptor, part2, ATEN_FIRMWARE_SIZE_2);
    close(fileDescriptor);
    if (byteCount != ATEN_FIRMWARE_SIZE_2)
    {
        perror(path);
        exit(1);
    }

    printf("Updating firmware...\n");
    int status = atenUpdateFirmware(serialDescriptor, part1, part2);
    switch (status)
    {
    case ATEN_NO_ERROR:
        printf("Firmware updated\n");
        break;

    case ATEN_READ_ERROR:
        printf("Failed. Is device in firmware update mode?\n");
        printf("Keep SELECT/LEARN button pressed while powering on EDID emulator,\n");
        printf("The DEFAULT LED will flash.\n");
        break;

    default:
        printf("Firmware update failed.\n");
        break;
    }
}



int main(int argc, char * const argv[])
{
    serial_t serialDescriptor = serialClosed;
    int currentPosition = ATEN_SET_DISPLAY;
    int character;


    printf("atenvc080 v%s\n", VERSION);

    while ((character = getopt(argc, argv, "?CDF:d:qr:s:w:")) != -1)
    {
        switch(character)
        {
        case 'C':
            CECConnect(serialDescriptor);
            break;
        case 'D':
            CECDisconnect(serialDescriptor);
            break;
        case 'F':
            firmwareUpdate(serialDescriptor, optarg);
            break;
        case 'd':
            connectToDevice(&serialDescriptor, optarg);
            break;
        case 'q':
            printInquiry(serialDescriptor);
            break;
        case 'r':
            writeEDIDToFile(serialDescriptor, currentPosition, optarg);
            break;
        case 's':
            selectSet(serialDescriptor, &currentPosition, optarg);
            break;
        case 'w':
            writeEDIDToDevice(serialDescriptor, currentPosition, optarg);
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
    if (serialDescriptor != -1)
        serialClosePort(serialDescriptor);      // dismiss errors

    return 0;
}
