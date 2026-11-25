

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Arduino.h"
#include <spi_drv.h>
#include "utility/wifi_drv.h"
#include "wifi_socket_drv.h"

#define _DEBUG_

//extern "C" {
//#include "utility/wifi_spi.h"
//#include "utility/wl_types.h"
//#include "utility/debug.h"
//}

#define SOCKET_JOIN_GROUP6 0x6B

bool WiFiSocketDrv::socketJoinGroup6(
    uint8_t sock,
    const uint8_t group[16])
{
    if (!group)
        return false;

    WAIT_FOR_SLAVE_SELECT();

    SpiDrv::sendCmd(
        SOCKET_JOIN_GROUP6, PARAM_NUMS_2);

    SpiDrv::sendParam(
        &sock, 1, NO_LAST_PARAM);

    SpiDrv::sendParam(
        group, 16, LAST_PARAM);

    // 3 header + 2 socket + 17 address + 1 END
    // = 23 bytes, padded to 24.
    SpiDrv::readChar();

    SpiDrv::spiSlaveDeselect();
    SpiDrv::waitForSlaveReady();
    SpiDrv::spiSlaveSelect();

    uint8_t result = 0;
    uint8_t len = 0;

    bool ok = SpiDrv::waitResponseCmd(
        SOCKET_JOIN_GROUP6,
        PARAM_NUMS_1,
        &result,
        &len);

    SpiDrv::spiSlaveDeselect();

    return ok && len == 1 && result == 1;
}
    
int WiFiSocketDrv::socket(uint8_t domain, uint8_t type, uint8_t protocol)
{
    uint8_t sock = 255;
    uint8_t len = 0;

    WAIT_FOR_SLAVE_SELECT();

    SpiDrv::sendCmd(SOCKET_SOCKET, PARAM_NUMS_3);

    SpiDrv::sendParam(&type, 1, NO_LAST_PARAM);
    SpiDrv::sendParam(&protocol, 1, NO_LAST_PARAM);
    SpiDrv::sendParam(&domain, 1, LAST_PARAM);

    // Total command length: 10 bytes.
    // Pad to the next multiple of four.
    SpiDrv::readChar();
    SpiDrv::readChar();

    SpiDrv::spiSlaveDeselect();
    SpiDrv::waitForSlaveReady();
    SpiDrv::spiSlaveSelect();

    bool ok = SpiDrv::waitResponseCmd(
        SOCKET_SOCKET, PARAM_NUMS_1, &sock, &len);

    SpiDrv::spiSlaveDeselect();

    return (ok && len == 1 && sock != 255) ? sock : -1;
}

bool WiFiSocketDrv::socketBind(
    uint8_t sock, const uint8_t ip6[16], uint16_t port)
{
    uint8_t portBytes[2] = {
        (uint8_t)(port >> 8),
        (uint8_t)(port & 0xff)
    };

    WAIT_FOR_SLAVE_SELECT();

    SpiDrv::sendCmd(SOCKET_BIND, PARAM_NUMS_3);

    SpiDrv::sendParam(&sock, 1, NO_LAST_PARAM);
    SpiDrv::sendParam(ip6, 16, NO_LAST_PARAM);
    SpiDrv::sendParam(portBytes, 2, LAST_PARAM);

    // Command is 26 bytes; pad to 28.
    SpiDrv::readChar();
		SpiDrv::readChar();

    SpiDrv::spiSlaveDeselect();
    SpiDrv::waitForSlaveReady();
    SpiDrv::spiSlaveSelect();

    uint8_t result = 0;
    uint8_t len = 0;

    bool ok = SpiDrv::waitResponseCmd(
        SOCKET_BIND, PARAM_NUMS_1, &result, &len);

    SpiDrv::spiSlaveDeselect();

    return ok && len == 1 && result == 1;
}

#define AF_INET6        10	// FIXME
#define BUFFER_SIZE	256
uint8_t srcIp6[16] = {0};
uint8_t srcPortBytes[2] = {0};
uint8_t buffer[256] = {0};

int WiFiSocketDrv::socketRecvFrom(
    int fd,
    uint8_t srcIp6[16],
    uint16_t* srcPort,
    uint8_t* buffer,
    uint16_t bufferSize)
{
  if (fd < 0 || fd > 255 ||
        !srcIp6 || !srcPort || !buffer)
        return -1;

  uint8_t domain = AF_INET6;
  uint8_t socket = (uint8_t)fd;
  uint8_t srcPortBytes[2] = {0};

  WAIT_FOR_SLAVE_SELECT();

  SpiDrv::sendCmd(SOCKET_RECVFROM, PARAM_NUMS_3);

  SpiDrv::sendParam(&socket, 1, NO_LAST_PARAM);

  SpiDrv::sendParam(
        (uint8_t*)&bufferSize, 2, NO_LAST_PARAM);

  SpiDrv::sendParam(&domain, 1, LAST_PARAM);

  // 11 bytes including END_CMD; pad to 12.
  SpiDrv::readChar();

  SpiDrv::spiSlaveDeselect();
  SpiDrv::waitForSlaveReady();
  SpiDrv::spiSlaveSelect();

  tDataCapacityParam params[3] = {
        {0, 16, srcIp6},
        {0, 2, srcPortBytes},
        {0, bufferSize, buffer}
  };

  bool ok = SpiDrv::waitResponseParams16(
        SOCKET_RECVFROM, 3, params);

  SpiDrv::spiSlaveDeselect();

  if (!ok ||
        params[0].dataLen != 16 ||
        params[1].dataLen != 2 ||
        params[2].dataLen > bufferSize)
        return -1;

  *srcPort =
        ((uint16_t)srcPortBytes[0] << 8) |
        srcPortBytes[1];

  return params[2].dataLen;
}

int WiFiSocketDrv::socketSendTo(
    uint8_t sock,
    const uint8_t ip6[16],
    uint16_t port,
    const uint8_t* buffer,
    uint16_t size)
{
    uint8_t portBytes[2] = {
        (uint8_t)(port >> 8),
        (uint8_t)(port & 0xff)
    };

    WAIT_FOR_SLAVE_SELECT();

    SpiDrv::sendCmd(SOCKET_SENDTO, PARAM_NUMS_4);

    // All four request parameters have 16-bit lengths.
    SpiDrv::sendBuffer(&sock, 1, NO_LAST_PARAM);
    SpiDrv::sendBuffer(ip6, 16, NO_LAST_PARAM);
    SpiDrv::sendBuffer(portBytes, 2, NO_LAST_PARAM);
    SpiDrv::sendBuffer(buffer, size, LAST_PARAM);

    // Total command length = 31 + size.
    uint32_t commandSize = 31 + size;

    while (commandSize % 4) {
        SpiDrv::readChar();
        commandSize++;
    }

    SpiDrv::spiSlaveDeselect();
    SpiDrv::waitForSlaveReady();
    SpiDrv::spiSlaveSelect();

    uint8_t result[2] = {0};
    uint8_t len = 0;

    bool ok = SpiDrv::waitResponseCmd(
        SOCKET_SENDTO, PARAM_NUMS_1, result, &len);

    SpiDrv::spiSlaveDeselect();

    if (!ok || len != 2)
        return -1;

    return ((uint16_t)result[0] << 8) | result[1];
}

//bool WiFiSocketDrv::close(uint8_t sock)
//{
//			return 0;	// FIXME
//}

