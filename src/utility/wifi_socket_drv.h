
#ifndef WiFi_Socket_Drv_h
#define WiFi_Socket_Drv_h

#include <inttypes.h>
#include "wifi_spi.h"
#include "IPAddress.h"
#include "WiFiUdp.h"
#include "WiFiClient.h"
#include "WiFiPreferences.h"


class WiFiSocketDrv {
public:
  static bool socketJoinGroup6(uint8_t sock, const uint8_t group[16]);
  static int socket(uint8_t domain, uint8_t type, uint8_t protocol);
  static bool socketBind(uint8_t sock, const uint8_t ip6[16], uint16_t port);
  static int socketRecvFrom(int fd, uint8_t srcIp6[16], uint16_t* srcPort, uint8_t* buffer, uint16_t bufferSize);
  static int socketSendTo(uint8_t sock, const uint8_t ip6[16], uint16_t port, const uint8_t* buffer, uint16_t size);
//  static bool close(uint8_t sock);

  friend class WiFiUDP;
};


#endif
