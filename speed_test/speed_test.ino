#include <WebServer_ESP32_W5500.h>
#include <WebSockets2_Generic.h>
#include <math.h>

#include "WiFi.h"
#include "esp_bt.h"
#include "esp_bt_device.h"

using namespace websockets2_generic;

// Enter a MAC address and IP address for your controller below.
#define NUMBER_OF_MAC      20

byte mac[][NUMBER_OF_MAC] =
{
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x02 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x04 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x05 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x06 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x07 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x08 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x09 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0A },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0B },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0C },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0D },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x0E },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0F },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x10 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x11 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x12 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x13 },
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x14 },
};

// Select the IP address according to your local network
IPAddress myIP(192, 168, 0, 232);
IPAddress myGW(192, 168, 0, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

WebServer server(80);
WebsocketsServer wsServer;

// Sine wave parameters
float sine_frequency = 100;  // Hz
float sine_amplitude = 1.0;
unsigned long prev_sine_time = 0;
unsigned long sine_interval = 0.01;  // ms

// Speed measurement variables
unsigned long last_measurement_time = 0;
unsigned long total_bytes_sent = 0;
float tx_speed_kbps = 0.0;

void handleRoot() {
  server.send(200, "text/plain", "Sine Wave + Speed Test | WS://" + ETH.localIP().toString() + ":81");
}

void calculate_speed() {
  if (millis() - last_measurement_time >= 1000) { // Update every 1 second
    tx_speed_kbps = (total_bytes_sent * 8) / 1000.0; // Convert to kbps
    total_bytes_sent = 0;
    last_measurement_time = millis();
  }
}

void setup() {
  WiFi.mode(WIFI_OFF);

  esp_bt_controller_disable();
  Serial.begin(115200);
  // ... [keep your existing setup code until ETH.config()] ...

  while (!Serial && (millis() < 5000));

  Serial.print(F("\nStart WebServer on "));
  Serial.print(ARDUINO_BOARD);
  Serial.print(F(" with "));
  Serial.println(SHIELD_TYPE);
  Serial.println(WEBSERVER_ESP32_W5500_VERSION);

  ET_LOGWARN(F("Default SPI pinout:"));
  ET_LOGWARN1(F("SPI_HOST:"), ETH_SPI_HOST);
  ET_LOGWARN1(F("MOSI:"), MOSI_GPIO);
  ET_LOGWARN1(F("MISO:"), MISO_GPIO);
  ET_LOGWARN1(F("SCK:"),  SCK_GPIO);
  ET_LOGWARN1(F("CS:"),   CS_GPIO);
  ET_LOGWARN1(F("INT:"),  INT_GPIO);
  ET_LOGWARN1(F("SPI Clock (MHz):"), SPI_CLOCK_MHZ);
  ET_LOGWARN(F("========================="));

  ///////////////////////////////////

  // To be called before ETH.begin()
  ESP32_W5500_onEvent();

  // start the ethernet connection and the server:
  // Use DHCP dynamic IP and random mac
  //bool begin(int MISO_GPIO, int MOSI_GPIO, int SCLK_GPIO, int CS_GPIO, int INT_GPIO, int SPI_CLOCK_MHZ,
  //           int SPI_HOST, uint8_t *W6100_Mac = W6100_Default_Mac);
  ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST );
  //ETH.begin( MISO_GPIO, MOSI_GPIO, SCK_GPIO, CS_GPIO, INT_GPIO, SPI_CLOCK_MHZ, ETH_SPI_HOST, mac[millis() % NUMBER_OF_MAC] );

  // Static IP, leave without this line to get IP via DHCP
  //bool config(IPAddress local_ip, IPAddress gateway, IPAddress subnet, IPAddress dns1 = 0, IPAddress dns2 = 0);
  ETH.config(myIP, myGW, mySN, myDNS);

  ESP32_W5500_waitForConnect();

  // Start WebSocket server
  wsServer.listen(81);
  Serial.println("WebSocket Server started at ws://" + ETH.localIP().toString() + ":81");

  server.on("/", handleRoot);
  server.begin();
}


void loop() {
  server.handleClient();
  static WebsocketsClient wsClient;
  
  // Accept new client
  if (!wsClient.available()) {
    wsClient = wsServer.accept();
  }

  // Generate sine wave and send data
  if (millis() - prev_sine_time >= sine_interval) {
    prev_sine_time = millis();
    float time_sec = millis() / 1000.0;
    float value = sine_amplitude * sin(2 * PI * sine_frequency * time_sec);

    // Create JSON payload
    String payload = "{\"time\":" + String(time_sec, 3) + 
                    ",\"value\":" + String(value, 4) + 
                    ",\"tx_speed\":" + String(tx_speed_kbps, 1) + "}";

    // Send and count bytes
    if (wsClient.available()) {
      wsClient.send(payload);
      total_bytes_sent += payload.length(); // Track TX bytes
    }
  }

  // Calculate speed every second
  calculate_speed();

  // Handle incoming messages (e.g., start/stop commands)
  if (wsClient.available()) {
    wsClient.poll();
    wsClient.onMessage([&](WebsocketsMessage msg) {
      if (msg.data() == "START_RECORDING") {
        wsClient.send("STATUS:RECORDING_STARTED");
      } else if (msg.data() == "STOP_RECORDING") {
        wsClient.send("STATUS:RECORDING_STOPPED");
      }
      // Optional: Count received bytes for RX speed
      // total_bytes_received += msg.data().length();
    });
  }
  delayMicroseconds(1);
}
