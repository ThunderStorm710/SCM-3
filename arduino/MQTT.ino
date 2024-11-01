#include "StealthPresenceDetector.h"

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


void subscribeCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);

  if (String(topic) == "detector/config") {
    String message;
    for (unsigned int i = 0; i < length; i++) {
      message += (char)payload[i];
    }

    Serial.print("Nova lista de dispositivos conhecidos recebida: ");
    Serial.println(message);

    knownCount = 0;
    int start = 0;
    int end = message.indexOf(',');

    while (end != -1 && knownCount < 10) {
      knownDevices[knownCount++] = message.substring(start, end);
      start = end + 1;
      end = message.indexOf(',', start);
    }
    if (knownCount < 10 && start < message.length()) {
      knownDevices[knownCount++] = message.substring(start);
    }

    Serial.println("Dispositivos conhecidos atualizados:");
    for (int i = 0; i < knownCount; i++) {
      Serial.println(knownDevices[i]);
    }
  }
}


void reconnect() {
  while (!client.connected()) {  // Tenta reconectar enquanto a conexão não estiver estabelecida
    Serial.print("A conectar ao broker MQTT...");
    if (client.connect("NodeMCU_Client", "thunder", "147258")) {   // Conecta com o ID "NodeMCU_Client"
      Serial.println("Conectado!");
      client.subscribe("detector/config");       // Subscrição ao tópico
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentar novamente em 5 segundos");
      delay(5000);                           // Espera antes de tentar novamente
    }
  }
}