#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* ssid = "MEO-2A5E60";           // Insira o SSID da tua rede Wi-Fi
const char* password = "70ba8221e7";   // Insira a password da tua rede Wi-Fi
const char* mqtt_server = "192.168.1.65";    // IP do broker MQTT na tua máquina
const int mqtt_port = 1883;
const char* mqtt_topic = "detector/config";
char* topic = "detected/";
const char* place = nullptr;  // Ponteiro para o local selecionado
char topic_publish[50];  // Array de char para armazenar o tópico completo

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastChannelChange = 0;

unsigned long lastPhaseChange = 0;
bool mqttPhase = true;
bool allChannels = false;

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

String knownDevices[5];

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
    if (ledOn){
      if (millis() - ledOffTime > 1000){
        ledOffTime = millis();
        digitalWrite(LED_BUILTIN, HIGH);
        ledOn = false;
      }
    }
    if (wifi_get_channel() == 13){
      allChannels = true;
    } else {
      int currentChannel = (wifi_get_channel() % 13) + 1;
      Serial.printf("Canal = %d\n", currentChannel);
      wifi_set_channel(currentChannel);
    }
}

static void obterEnderecoMAC(char *endereco, uint8_t* dados) {
  sprintf(endereco, "%02X:%02X:%02X:%02X:%02X:%02X", dados[10], dados[11], dados[12], dados[13], dados[14], dados[15]);
}

static void ICACHE_FLASH_ATTR sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *snifferPacket = (struct SnifferPacket*) buffer;

  char addr[18];
  obterEnderecoMAC(addr, snifferPacket->data);
  String macString = String(addr); // Cria uma String a partir do char array
  bool adicionar = false;
  for (int i = 0; i < 5; i++){
    if (macString == knownDevices[i] && !ledOn) {
      adicionar = true;
      Serial.printf("Dispositivo conhecido detetado: %s --> Local: %s\n", addr, place);
      ledOn = true;               // Marca o LED para ligar
      ledOffTime = millis(); // Define o tempo para desligar o LED após 1 segundo
      digitalWrite(LED_BUILTIN, LOW); // Liga o LED (ativo-baixo)
    bool exists = false;
    for (int j = 0; j < detectedCount; j++) {

      if (detectedDevices[j] == macString) {
        exists = true;
        break;
        }
    }
    if (!exists && detectedCount < 5) {
      detectedDevices[detectedCount] = macString;
      detectedCount++;
      Serial.println("Novo dispositivo adicionado: " + macString);
    }

    }
  }







    //Serial.print("MAC: ");
    //Serial.print(addr);
    //Serial.print(" Canal: ");
    //Serial.println(wifi_get_channel());

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  // Converte o payload em uma string
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Nova lista de dispositivos conhecidos recebida: ");
  Serial.println(message);

  // Verifica e remove colchetes no início e no fim, se existirem
  if (message.startsWith("[") && message.endsWith("]")) {
    message.remove(0, 1); // Remove o primeiro colchete "["
    message.remove(message.length() - 1, 1); // Remove o último colchete "]"
  } else {
    Serial.println("Formato inválido de mensagem. Espera-se uma lista.");
    return;
  }

  // Inicializa o contador de dispositivos conhecidos
  knownCount = 0;

  // Processa cada endereço MAC na lista
  int start = 0;
  int end = message.indexOf('"'); // Busca pelo próximo conjunto de aspas duplas

  while (end != -1 && knownCount < 10) {
    start = end + 1; // Ignora a primeira aspas dupla
    end = message.indexOf('"', start); // Procura a próxima aspas dupla

    if (end != -1) {
      // Extrai e armazena o endereço MAC
      String macAddress = message.substring(start, end);
      if (macAddress.length() > 0) { // Verifica se o endereço MAC não está vazio
        knownDevices[knownCount++] = macAddress;
      }
      end = message.indexOf('"', end + 1); // Move para o próximo conjunto de aspas duplas
    }
  }

  Serial.println("Dispositivos conhecidos atualizados:");
  for (int i = 0; i < knownCount; i++) {
    Serial.println(knownDevices[i]);
  }
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
    allChannels = false;

}

void publishDetectedDevices() {
  String msg;
  if (detectedDevices[0] != ""){
    for (int i = 0; i < detectedCount; i++) {
      if (i > 0) msg += ", ";
      msg += detectedDevices[i];
    }
    Serial.print("A publicar mensagem: ");
    Serial.println(msg);
    client.publish(topic_publish, msg.c_str());

    for (int i = 0; i < detectedCount; i++) {
      detectedDevices[i] = "";
    }
    detectedCount = 0;
  }
}



void setup() {
  Serial.begin(9600);
      while (!Serial) {
  ;
  }

  Serial.print("Defina o nome do dispositivo: ");
int choice = 0;  // Variável para armazenar a escolha do utilizador

    // Espera até o utilizador inserir um número válido
    while (place == nullptr) {
        if (Serial.available() > 0) {
            choice = Serial.parseInt();  // Lê um número inteiro da entrada serial

            switch (choice) {
                case 1:
                    place = "cafe";
                    break;
                case 2:
                    place = "wc";
                    break;
                case 3:
                    place = "sala 1";
                    break;
                case 4:
                    place = "sala 5";
                    break;
                default:
                    Serial.println("Escolha inválida. Insira um número entre 1 e 4.");
                    choice = 0;  // Reinicia para esperar um número válido
                    break;
            }
        }
    }

    Serial.printf("\nLocal definido para: %s\n", place);

    snprintf(topic_publish, sizeof(topic_publish), "%s%s", topic, place);

    Serial.printf("'%s'\n",topic_publish);

    ligarARede();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    for(int i = 0; i < 5; i++){
      knownDevices[i] = "";
      detectedDevices[i] = "";
    }
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
        if (allChannels || currentMillis - lastPhaseChange > 30000) {
            lastPhaseChange = currentMillis;
            mqttPhase = true;
            setOffModoPromiscuo();
        }
    }
}


