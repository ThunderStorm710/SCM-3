#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* ssid = "MEO-2A5E60";
const char* password = "70ba8221e7";
const char* mqtt_server = "192.168.1.65";
const int mqtt_port = 1883;
const char* mqtt_topic = "detector/config";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastChannelChange = 0;

unsigned long lastPhaseChange = 0;
bool mqttPhase = true;

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

void ligarARede() {
    Serial.println();
    Serial.print("A conectar à rede Wi-Fi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("A conectar ao broker MQTT...");
        if (client.connect("NodeMCU_Client", "thunder", "147258")) {
            Serial.println("Conectado!");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("Falha, rc=");
            Serial.print(client.state());
            Serial.println(" Tentar novamente em 5 segundos");
            delay(5000);
        }
    }
}

void desligarRede() {
    WiFi.disconnect();
    Serial.println("Wi-Fi desconectado");
}

void changeWiFiChannel() {
    int currentChannel = (wifi_get_channel() % 13) + 1;
    Serial.printf("Canal = %d\n", currentChannel);
    wifi_set_channel(currentChannel);
}

static void obterEnderecoMAC(char *endereco, uint8_t* dados) {
  sprintf(endereco, "%02X:%02X:%02X:%02X:%02X:%02X", dados[10], dados[11], dados[12], dados[13], dados[14], dados[15]);
}

static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;

    char addr[18];
    obterEnderecoMAC(addr, snifferPacket->data);
    String macString = String(addr); // Cria uma String a partir do char array


  if (macString == knownDevices[0] && !ledOn) {
    Serial.println("Dispositivo conhecido detetado: " + macString);
    ledOn = true;               // Marca o LED para ligar
    ledOffTime = millis() + 1000; // Define o tempo para desligar o LED após 1 segundo
    digitalWrite(LED_BUILTIN, LOW); // Liga o LED (ativo-baixo)
  }

  bool exists = false;
  for (int i = 0; i < detectedCount; i++) {
    if (detectedDevices[i] == macString) {
      exists = true;
      break;
    }
  }

  // Adicionar o endereço MAC à lista se ainda não estiver presente e houver espaço
  if (!exists && detectedCount < 10) {
    detectedDevices[detectedCount] = macString;
    detectedCount++;
    Serial.println("Novo dispositivo adicionado: " + macString);
  }

    //Serial.print("MAC: ");
    //Serial.print(addr);
    //Serial.print(" Canal: ");
    //Serial.println(wifi_get_channel());

}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Mensagem recebida no tópico: ");
    Serial.println(topic);
    Serial.print("Mensagem: ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void setOnModoPromicuo() {
    wifi_set_opmode(STATION_MODE);
    wifi_set_channel(1);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(sniffer_callback);
    wifi_promiscuous_enable(1);
    Serial.println("Modo promíscuo ativado");
}

void setOffModoPromiscuo() {
    wifi_promiscuous_enable(0);
    Serial.println("Modo promíscuo desativado");
    ligarARede();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

}

void publishDetectedDevices() {
  String msg;
  for (int i = 0; i < detectedCount; i++) {
    if (i > 0) msg += ", ";
    msg += detectedDevices[i];
  }

  Serial.print("A publicar mensagem: ");
  Serial.println(msg);
  client.publish("detected/config", msg.c_str());

  for (int i = 0; i < detectedCount; i++) {
    detectedDevices[i] = "";
  }
  detectedCount = 0;
}



void setup() {
    Serial.begin(9600);
    ligarARede();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    unsigned long currentMillis = millis();

    if (mqttPhase) {
        if (WiFi.status() != WL_CONNECTED) {
            ligarARede();
        }
        if (!client.connected()) {
            reconnect();
            publishDetectedDevices();

        }
        client.loop();

        // Verifica se já passou 30 segundos na fase MQTT
        if (currentMillis - lastPhaseChange > 30000) {
            lastPhaseChange = currentMillis;
            mqttPhase = false;
            client.disconnect();
            desligarRede();
            setOnModoPromicuo();
        }
    } else {
          if (millis() - lastChannelChange >= 2000) {
            lastChannelChange = millis();
            changeWiFiChannel();
          }
        // Muda de canal no modo promíscuo até ao canal 13

        // Se tiver passado mais 30 segundos, volta à fase MQTT
        if (currentMillis - lastPhaseChange > 30000) {
            lastPhaseChange = currentMillis;
            mqttPhase = true;
            setOffModoPromiscuo();
        }
    }
}


