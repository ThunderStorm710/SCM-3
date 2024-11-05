#ifndef StealthPresenceDetector_H
#define StealthPresenceDetector_H

#include <Arduino.h>
extern "C" {
  #include <user_interface.h>
}
#include <PubSubClient.h>      // Biblioteca para comunicação MQTT
#include <ESP8266WiFi.h>       // Biblioteca para conexão Wi-Fi


// Definições da rede Wi-Fi
//const char* ssid = "Pedro's Galaxy A53 5G";           // Insira o SSID da tua rede Wi-Fi
//const char* password = "12345678";   // Insira a password da tua rede Wi-Fi

const char* ssid = "NVS";           // Insira o SSID da tua rede Wi-Fi
const char* password = "Ns175411..";   // Insira a password da tua rede Wi-Fi

// Definições do broker MQTT
//const char* mqtt_server = "192.168.255.74";    // IP do broker MQTT na tua máquina
const char* mqtt_server = "192.168.1.5";    // IP do broker MQTT na tua máquina
const int mqtt_port = 1883;
const char *mqtt_topic = "detector/config";     // MQTT topic
             // Porta padrão do broker MQTT

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastChannelChange = 0;
unsigned long ledOffTime = 0;
bool ledOn = false;

struct RxControl {
  signed rssi:8;
  unsigned rate:4;
  unsigned is_group:1;
  unsigned:1;
  unsigned sig_mode:2;
  unsigned legacy_length:12;
  unsigned damatch0:1;
  unsigned damatch1:1;
  unsigned bssidmatch0:1;
  unsigned bssidmatch1:1;
  unsigned MCS:7;
  unsigned CWB:1;
  unsigned HT_length:16;
  unsigned Smoothing:1;
  unsigned Not_Sounding:1;
  unsigned:1;
  unsigned Aggregation:1;
  unsigned STBC:2;
  unsigned FEC_CODING:1;
  unsigned SGI:1;
  unsigned rxend_state:8;
  unsigned ampdu_cnt:8;
  unsigned channel:4;
  unsigned:12;
};

struct SnifferPacket {
  struct RxControl rx_ctrl;
  uint8_t data[112];
  uint16_t cnt;
  uint16_t len;
};

String knownDevices[] = {
  "68:4A:E9:09:04:C1"
};

int detectedCount = 0; // Contador para dispositivos detetados
int knownCount = 0;      // Contador de dispositivos conhecidos




String detectedDevices[10];


#endif