// ESP8266 WiFi <-> UART Bridge

#include <ESP8266WiFi.h>

//////////////////////////////////////////////////////////////////////////
// *** config *** //

// UART
#define UART_BAUD 9600
#define packTimeout 5         // ms (if nothing more on UART, then send packet)
#define bufferSize 8192

// WiFi mode
#define MODE_AP           // AP with DHCP
//#define MODE_STA            // ESP connects to WiFi router

// mode
#define PROTOCOL_TCP
//#define PROTOCOL_UDP

#ifdef MODE_AP
// For AP mode
const char *ssid = "semafor";         // Access Point
const char *pw = "123456789";         // and this is the password
IPAddress ip(192, 168, 0, 10);         // this IP
IPAddress netmask(255, 255, 255, 0);  // netmask
const int port = 9876;                // and this port
#endif

#ifdef MODE_STA
// For STATION mode
const char *ssid = "multimedija";      // your ROUTER SSID
const char *pw = "123456789";        // WiFi PASSWORD
const int port = 9876;
#endif

// *** config *** //
//////////////////////////////////////////////////////////////////////////


#ifdef PROTOCOL_TCP
#include <WiFiClient.h>
WiFiServer server(port);
WiFiClient client;
#endif

#ifdef PROTOCOL_UDP
#include <WiFiUdp.h>
WiFiUDP udp;
IPAddress remoteIp;
#endif


uint8_t buf1[bufferSize];
uint8_t i1 = 0;

uint8_t buf2[bufferSize];
uint8_t i2 = 0;


void setup() {

  delay(500);
  Serial.begin(UART_BAUD);

#ifdef MODE_AP
  //AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, netmask);         // configure ip address for softAP
  WiFi.softAP(ssid, pw);                      // configure ssid and password for softAP
  Serial.println();
  Serial.println("AP started and waiting for connections...");
  Serial.print("    Local IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("        SSID: ");
  Serial.println(ssid);
  Serial.print(" AP password: ");
  Serial.println(pw);
#endif

#ifdef MODE_STA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  Serial.println();
  Serial.println("ESP connecting...");
  Serial.print("        SSID: ");
  Serial.println(ssid);
  Serial.print(" AP password: ");
  Serial.println(pw);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("ESP connected!");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
#endif

#ifdef PROTOCOL_TCP
  Serial.print("Starting TCP Server...");
  server.begin();                           // start TCP server
  Serial.println("OK");
#endif

#ifdef PROTOCOL_UDP
  Serial.print("Starting UDP Server...");
  udp.begin(port);                          // start UDP server
  Serial.println("OK");
#endif
}


void loop() {

#ifdef PROTOCOL_TCP
  if (!client.connected()) {        // if client not connected
    client = server.available();    // wait for it to connect
    return;
  }

  // here we have a connected client
  //Serial.println("TCP client connected!");
  if (client.available()) {
    while (client.available()) {
      buf1[i1] = (uint8_t)client.read();  // read char from client
      if (i1 < bufferSize - 1) i1++;
    }
    // now send to UART:
    Serial.write(buf1, i1);
    i1 = 0;
  }

  if (Serial.available()) {

    // read the data until pause:
    while (1) {
      if (Serial.available()) {
        buf2[i2] = (char)Serial.read();   // read char from UART
        if (i2 < bufferSize - 1) i2++;
      } else {
        //delayMicroseconds(packTimeoutMicros);
        delay(packTimeout);
        if (!Serial.available()) {
          break;
        }
      }
    }

    // now send to WiFi:
    client.write((char*)buf2, i2);
    i2 = 0;
  }
#endif


#ifdef PROTOCOL_UDP

  // if there’s data available, read a packet
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    remoteIp = udp.remoteIP();            // store the ip of the remote device
    udp.read(buf1, bufferSize);
    // now send to UART:
    Serial.write(buf1, packetSize);
  }

  if (Serial.available()) {

    // read the data until pause:
    while (1) {
      if (Serial.available()) {
        buf2[i2] = (char)Serial.read();   // read char from UART
        if (i2 < bufferSize - 1) {
          i2++;
        }
      } else {
        //delayMicroseconds(packTimeoutMicros);
        //Serial.println("dl");
        delay(packTimeout);
        if (!Serial.available()) {
          //Serial.println("bk");
          break;
        }
      }
    }

    // now send to WiFi:
    udp.beginPacket(remoteIp, port); // remote IP and port
    udp.write(buf2, i2);
    udp.endPacket();
    i2 = 0;
  }
#endif
}
