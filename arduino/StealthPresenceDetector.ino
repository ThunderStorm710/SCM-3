#include "MQTT.h"

void setup() {
    Serial.begin(9600);
    while (!Serial) {
    ;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED desligado (ativo-baixo)
  setOnModoPromicuo();
}

void loop() {
  if (millis() - lastChannelChange >= 2000) {
    lastChannelChange = millis();
    changeWiFiChannel();
  }

  // Desliga o LED após 1 segundo
  if (ledOn && millis() >= ledOffTime) {
    digitalWrite(LED_BUILTIN, HIGH); // Desliga o LED
    ledOn = false; // Redefine o estado do LED
  }
}


void setOffModoPromicuo(){
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  Serial.println("DESLIGUEI O MODO PROMISCUO");

  ligarARede();

  Serial.println("COMECEI MQTT");

  client.setServer(mqtt_server, mqtt_port);  // Configura o servidor MQTT
  client.setCallback(subscribeCallback);


  if (!client.connected()) {
    reconnect();            // Reconecta caso a conexão seja perdida
  }
  client.loop();            // Mantém a comunicação com o broker ativa

    static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 5000) {            // Delay de 5 segundos
    lastMsg = now;
    Serial.println("FIZ O SUBSCRIBE");
    delay(5000);
    Serial.println("VOU LIGAR O MODO PROMISCUO");
    setOnModoPromicuo();
  }


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

static void obterEnderecoMAC(char *endereco, uint8_t* dados) {
  sprintf(endereco, "%02X:%02X:%02X:%02X:%02X:%02X", dados[10], dados[11], dados[12], dados[13], dados[14], dados[15]);
}

void changeWiFiChannel() {
  if (wifi_get_channel() == 13){
    setOffModoPromicuo();
  }
  int currentChannel = (wifi_get_channel() % 13) + 1;
  Serial.printf("Canal = %d\n", currentChannel);
  wifi_set_channel(currentChannel);
}


void setOnModoPromicuo(){
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(1);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
}




