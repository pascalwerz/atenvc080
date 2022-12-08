//
//  aten.c
//  atenvc080
//

#include "mac.h"
#include "aten.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>



int edidVerifyChecksum(uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    int blockCount = 1 + edid[ATEN_EXTENSION_COUNT_OFFSET];

    for (int block = 0; block < blockCount; block++)
    {
        uint8_t sum = 0;
        size_t i;


        for (i = 0; i < ATEN_BLOCK_SIZE; i++)
            sum += edid[block * ATEN_BLOCK_SIZE + i];

        if (sum != 0)
            return ATEN_INVALID;
    }

    return ATEN_NO_ERROR;
}



int edidIsValid(uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    int blockCount = 1 + edid[ATEN_EXTENSION_COUNT_OFFSET];

    if (blockCount > 2)
        return ATEN_INVALID;

    return edidVerifyChecksum(edid);
}



int atenDeviceAttached(int serialDevice)
{
    int byte;

    if (serialWriteByte(serialDevice, 0x0b) != serialOK)
        return -1;

    pauseMilliseconds(1000);
    byte = serialReadByte(serialDevice);

    return byte;
}



int atenCECConnect(int serialDevice)
{
    if (serialWriteByte(serialDevice, 0x08) != serialOK)
        return ATEN_WRITE_ERROR;

    pauseMilliseconds(1000);

    return ATEN_NO_ERROR;
}



int atenCECDisconnect(int serialDevice)
{
    if (serialWriteByte(serialDevice, 0x09) != serialOK)
        return ATEN_WRITE_ERROR;

    pauseMilliseconds(1000);

    return ATEN_NO_ERROR;
}



int atenPosition(int serialDevice, int setID)
{
    serial_status_t status;

    switch (setID)
    {
    case ATEN_SET_DEFAULT: status = serialWriteByte(serialDevice, 0x01); pauseMilliseconds(1000); break;
    case ATEN_SET_1:       status = serialWriteByte(serialDevice, 0x02); pauseMilliseconds(1000); break;
    case ATEN_SET_2:       status = serialWriteByte(serialDevice, 0x03); pauseMilliseconds(1000); break;
    case ATEN_SET_3:       status = serialWriteByte(serialDevice, 0x04); pauseMilliseconds(1000); break;
    default:               status = serialError; break;
    }

    if (status != serialOK)
        return  ATEN_WRITE_ERROR;

    return  ATEN_NO_ERROR;
}



int atenGetExtensionData(int serialDevice, int extension, uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    if (serialWriteByte(serialDevice, 0x05) != serialOK)
        return ATEN_READ_ERROR;

    pauseMilliseconds(1000);

    if (serialReadBytes(serialDevice, edid + (extension + 1) * ATEN_BLOCK_SIZE, ATEN_BLOCK_SIZE) != serialOK)
        return ATEN_READ_ERROR;

    return ATEN_NO_ERROR;
}



int atenReadEDIDFromDevice(int serialDevice, uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    uint8_t byte;
    int extensionBlockCount;


    bzero(edid, 2 * ATEN_BLOCK_SIZE);

    if (serialWriteByte(serialDevice, 0x0c) != serialOK)
        return ATEN_READ_ERROR;

    pauseMilliseconds(1000);

    byte = serialReadByte(serialDevice);
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05)
        return ATEN_READ_ERROR;

    if (serialReadBytes(serialDevice, edid, ATEN_BLOCK_SIZE) != serialOK)
        return ATEN_READ_ERROR;
    extensionBlockCount = edid[ATEN_EXTENSION_COUNT_OFFSET];
    for (int extension = 0; extension < extensionBlockCount; extension++)
    {
        if (atenGetExtensionData(serialDevice, extension, edid) != ATEN_NO_ERROR)
            return ATEN_READ_ERROR;
    }

    return edidIsValid(edid);
}



int atenReadEDIDFromDisplay(int serialDevice, uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    int byte;
    int extensionBlockCount;


    bzero(edid, 2 * ATEN_BLOCK_SIZE);

    if (serialWriteByte(serialDevice, 0x07) != serialOK)
        return ATEN_READ_ERROR;

    pauseMilliseconds(1000);

    byte = serialReadByte(serialDevice);
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05)
        return ATEN_READ_ERROR;

    if (serialReadBytes(serialDevice, edid, ATEN_BLOCK_SIZE) != serialOK)
        return ATEN_READ_ERROR;
    extensionBlockCount = edid[ATEN_EXTENSION_COUNT_OFFSET];
    for (int extension = 0; extension < extensionBlockCount; extension++)
    {
        if (atenGetExtensionData(serialDevice, extension, edid) != ATEN_NO_ERROR)
            return ATEN_READ_ERROR;
    }

    return edidIsValid(edid);
}



int atenWriteEDID(int serialDevice, uint8_t edid[ATEN_MAX_EDID_SIZE])
{
    uint8_t byte;
    int extensionBlockCount;


    extensionBlockCount = edid[ATEN_EXTENSION_COUNT_OFFSET];

    if (extensionBlockCount > 1)
        return ATEN_WRITE_ERROR;

    if (serialWriteByte(serialDevice, 0x0a) != serialOK)
        return ATEN_WRITE_ERROR;

    pauseMilliseconds(1000);

    byte = serialReadByte(serialDevice);
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05)
        return ATEN_WRITE_ERROR;

    // main EDID block
    if (serialWriteBytes(serialDevice, edid, ATEN_BLOCK_SIZE) != serialOK)
        return ATEN_WRITE_ERROR;

    pauseMilliseconds(1000);

    byte = serialReadByte(serialDevice);
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
    if (byte != 0x05)
        return ATEN_WRITE_ERROR;

    for (int extensionBlock = 0; extensionBlock < extensionBlockCount; extensionBlock++)
    {
        // extension EDID block
        if (serialWriteBytes(serialDevice, edid + (extensionBlock + 1) * ATEN_BLOCK_SIZE, ATEN_BLOCK_SIZE) != serialOK)
            return ATEN_WRITE_ERROR;

        pauseMilliseconds(1000);

        byte = serialReadByte(serialDevice);
        if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
        if (byte != 0x05) { pauseMilliseconds(1000); byte = serialReadByte(serialDevice); }
        if (byte != 0x05)
            return ATEN_WRITE_ERROR;
    }
    return ATEN_NO_ERROR;
}



int atenReadEDIDFromFile(uint8_t edid[ATEN_MAX_EDID_SIZE], char * path)
{
    int fileDescriptor;
    size_t byteCount;
    int extensionBlockCount;


    fileDescriptor = open(path, O_RDONLY);
    if (fileDescriptor < 0)
        return ATEN_INVALID;

    byteCount = read(fileDescriptor, edid, ATEN_BLOCK_SIZE);
    if (byteCount != ATEN_BLOCK_SIZE)
    {
        close(fileDescriptor);
        return ATEN_INVALID;
    }

    extensionBlockCount = edid[ATEN_EXTENSION_COUNT_OFFSET];
    byteCount = read(fileDescriptor, edid + ATEN_BLOCK_SIZE, extensionBlockCount * ATEN_BLOCK_SIZE);
    close(fileDescriptor);
    if (byteCount != extensionBlockCount * ATEN_BLOCK_SIZE)
        return ATEN_INVALID;

    return edidIsValid(edid);
}



int atenWriteEDIDToFile(uint8_t edid[ATEN_MAX_EDID_SIZE], char * path)
{
    int fileDescriptor;
    size_t byteCount;
    int extensionBlockCount;


    extensionBlockCount = edid[ATEN_EXTENSION_COUNT_OFFSET];
    fileDescriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileDescriptor >= 0)
    {
        byteCount = write(fileDescriptor, edid, (extensionBlockCount + 1) * ATEN_BLOCK_SIZE);
        close(fileDescriptor);

        if (byteCount == (extensionBlockCount + 1) * ATEN_BLOCK_SIZE)
            return ATEN_NO_ERROR;
    }

    return  ATEN_WRITE_ERROR;
}



void atenAppendFirmwareModeChecksum(uint8_t * data, size_t byteCount)
{
    size_t i;
    uint8_t sum = 0;



    for (i = 0; i < byteCount - 1; i++)
        sum += data[i];

    data[i] = sum;
}



int atenVerifyFirmwareModeChecksum(uint8_t * data, size_t byteCount)
{
    size_t i;
    uint8_t sum = 0;



    for (i = 0; i < byteCount - 1; i++)
        sum += data[i];

    if (data[i] != sum)
        return ATEN_INVALID;

    return ATEN_NO_ERROR;
}



int atenSendFirmwareModeCommand(int serialDevice, uint8_t * command, size_t byteCount)
{
    atenAppendFirmwareModeChecksum(command, byteCount);
    if (serialWriteBytes(serialDevice, command, byteCount) != serialOK)
        return ATEN_WRITE_ERROR;

    return ATEN_NO_ERROR;
}



int atenGetFirmwareModeReply(int serialDevice, uint8_t * reply, size_t byteCount)
{
    if (byteCount < 2)
        return ATEN_READ_ERROR;

    while(serialPendingBytesCount(serialDevice) < 2)
        pauseMilliseconds(50);

    if (serialReadBytes(serialDevice, reply, 2) != serialOK)
        return ATEN_READ_ERROR;
    if (reply[0] != 'F' || reply[1] != 'U')
        return ATEN_READ_ERROR;

    if (serialReadBytes(serialDevice, reply + 2, byteCount - 2) != serialOK)
        return ATEN_READ_ERROR;

    if (atenVerifyFirmwareModeChecksum(reply, byteCount) != ATEN_NO_ERROR)
        return ATEN_READ_ERROR;

    return ATEN_NO_ERROR;
}



// work in progress
int atenUpdateFirmware(int serialDevice, uint8_t * data, size_t length)
{
    uint16_t expectedSum;
    uint16_t sum;
    uint8_t reply[256];
    // command size is +1 for the checksum that will be computed in atenSendFirmwareModeCommand()
    uint8_t commandff[ 5 + 1] = { 'F', 'U', 0xff, 'E', 'N' };
    uint8_t command80[27 + 1] = { 'F', 'U', 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t command90[ 6 + 1] = { 'F', 'U', 0x90, 0x00, 0x44, 0x49 };
    uint8_t commanda0[ 6 + 1] = { 'F', 'U', 0xa0, 0x00, 0x43, 0x54 };
    uint8_t commanda2[68 + 1] = { 'F', 'U', 0xa2, 0x00 };
    uint8_t commanda3[70 + 1] = { 'F', 'U', 0xa3, 0x00, 0x00, 0x00 };
    uint8_t commanda4[ 6 + 1] = { 'F', 'U', 0xa4, 0x00, 0x44, 0x54 };
    uint8_t commanda5[ 6 + 1] = { 'F', 'U', 0xa5, 0xff, 0x41, 0x44 };


    if (length != ATEN_FIRMWARE_SIZE_1 + ATEN_FIRMWARE_SIZE_2 + 2)
        goto invalidData;

    if (memcmp(data, "ATENVC060/080", 13))
        goto invalidData;

    sum = 0;
    for (size_t i = 0; i < length - 2; i += 2)
        sum += (data[i] << 8) | data[i + 1];
    expectedSum = (data[length - 2] << 8) | data[length - 1];

    if (sum != expectedSum)
        goto invalidData;

    if (serialSetRate(serialDevice, 19200, 19200) != serialOK)
        goto writeError;
    if (serialSetRTS(serialDevice, 0) != serialOK)
        goto writeError;

    pauseMilliseconds(100);
    serialClearPendingBytes(serialDevice);      // purge serial input buffer, dismiss errors

    if (atenSendFirmwareModeCommand(serialDevice, commandff, sizeof(commandff)) != ATEN_NO_ERROR)
        goto writeError;
    pauseMilliseconds(1000);
    if (serialPendingBytesCount(serialDevice) < 1)
        goto readError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 32) != ATEN_NO_ERROR)
        goto readError;
    if (reply[2] != (commandff[2] ^ 0x80))
        goto readError;

    if (atenSendFirmwareModeCommand(serialDevice, command80, sizeof(command80)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 5) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (command80[2] ^ 0x80) || reply[3] != command80[3])
        goto writeError;

    if (atenSendFirmwareModeCommand(serialDevice, command90, sizeof(command90)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 50) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (command90[2] ^ 0x80) || reply[3] != command90[3] || memcmp(reply + 4, "VC060/080", 9))
        goto writeError;
    printf("Device status before upgrade:\n");
    printf("         CPU is: %.7s\n", reply + 42);
    printf("   firmware was: v%c.%c.%c%c%c\n", reply[28], reply[29], reply[31], reply[32], reply[33]);
    printf("  microcode was: v%c.%c.%c%c%c\n", reply[35], reply[36], reply[38], reply[39], reply[40]);

    if (atenSendFirmwareModeCommand(serialDevice, commanda0, sizeof(commanda0)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 6) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (commanda0[2] ^ 0x80) || reply[3] != commanda0[3] || reply[4] != 0x00)
        goto writeError;

    memcpy(commanda2 + 4, data, ATEN_FIRMWARE_SIZE_1);
    if (atenSendFirmwareModeCommand(serialDevice, commanda2, sizeof(commanda2)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 6) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (commanda2[2] ^ 0x80) || reply[3] != commanda2[3] || reply[4] != 0x00)
        goto writeError;

    for (size_t offset = 0; offset < ATEN_FIRMWARE_SIZE_2; offset += 64)
    {
        commanda3[4] = (offset / 64) >> 8;
        commanda3[5] = (offset / 64);
        memcpy(commanda3 + 6, data + ATEN_FIRMWARE_SIZE_1 + offset, 64);
        if (atenSendFirmwareModeCommand(serialDevice, commanda3, sizeof(commanda3)) != ATEN_NO_ERROR)
            goto writeError;
        if (atenGetFirmwareModeReply(serialDevice, reply, 8) != ATEN_NO_ERROR)
            goto writeError;
        if (reply[2] != (commanda3[2] ^ 0x80) || reply[3] != commanda3[3] || reply[4] != (commanda3[4]) || reply[5] != commanda3[5] || reply[6] != 0x00)
            goto writeError;
    }

    if (atenSendFirmwareModeCommand(serialDevice, commanda4, sizeof(commanda4)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 6) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (commanda4[2] ^ 0x80) || reply[3] != commanda4[3] || reply[4] != 0x00)
        goto writeError;

    if (atenSendFirmwareModeCommand(serialDevice, commanda5, sizeof(commanda5)) != ATEN_NO_ERROR)
        goto writeError;
    if (atenGetFirmwareModeReply(serialDevice, reply, 6) != ATEN_NO_ERROR)
        goto writeError;
    if (reply[2] != (commanda5[2] ^ 0x80) || reply[3] != 0x00 || reply[4] != 0x00)
        goto writeError;

    int status = ATEN_NO_ERROR;
    goto end;

invalidData:
    status = ATEN_INVALID;
    goto end;

readError:
    status = ATEN_READ_ERROR;
    goto end;

writeError:
    status = ATEN_WRITE_ERROR;
    goto end;

end:
    serialSetRate(serialDevice, 115200, 115200);    // dismiss errors
    serialSetRTS(serialDevice, 0);                  // dismiss errors
    pauseMilliseconds(100);
    serialClearPendingBytes(serialDevice);         // purge serial input buffer, dismiss errors

    return status;
}
