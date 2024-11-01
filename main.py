from flask import Flask, render_template, request, redirect, url_for, flash
import paho.mqtt.client as mqtt
import json
import re

app = Flask(__name__)

# Configurações do MQTT
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
CONFIG_TOPIC = "detector/config"
DETECTED_TOPIC = "detected/config"

# Listas de dispositivos conhecidos e detetados
known_devices = []
detected_devices = []

# Padrão de expressão regular para um endereço MAC válido
mac_format = re.compile(r'^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$')


# Função de callback para conexão MQTT
def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker MQTT com código " + str(rc))
    client.subscribe(DETECTED_TOPIC)  # Subscreve para receber notificações de deteção


# Função de callback para mensagens MQTT recebidas
def on_message(client, userdata, msg):
    global detected_devices
    # Atualizar a lista de dispositivos com os dados recebidos
    device_info = msg.payload.decode()
    print(device_info)
    devices = device_info.split(",")
    for device in devices:
        if device not in detected_devices:
            detected_devices.append(device)
            print(f"Novo dispositivo detectado: {device}")

# Configura e conecta o cliente MQTT

mqtt_client = mqtt.Client()
mqtt_client.username_pw_set("thunder", "147258")
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()


# Rota principal para mostrar a interface
@app.route('/')
def index():
    return render_template('index.html', known_devices=known_devices, detected_devices=detected_devices)


# Rota para adicionar um único dispositivo conhecido com verificação de formato
@app.route('/add_device', methods=['POST'])
def add_device():
    global known_devices
    mac_address = request.form['mac_address'].strip()
    # Validação do formato do MAC no backend
    if not mac_format.match(mac_address):
        flash("Invalid MAC address format. Please enter a valid format (e.g., XX:XX:XX:XX:XX:XX).")
        return redirect(url_for('index'))
    # Adicionar o MAC se for válido e não estiver duplicado
    if mac_address not in known_devices:
        known_devices.append(mac_address)
        mqtt_client.publish(CONFIG_TOPIC, json.dumps(known_devices))  # Envia lista atualizada via MQTT
    return redirect(url_for('index'))


# Rota para remover um dispositivo conhecido específico
@app.route('/remove_known/<mac_address>', methods=['POST'])
def remove_known(mac_address):
    global known_devices
    known_devices = [device for device in known_devices if device != mac_address]
    mqtt_client.publish(CONFIG_TOPIC, json.dumps(known_devices))  # Envia lista atualizada via MQTT
    return redirect(url_for('index'))


# Rota para limpar todos os dispositivos conhecidos
@app.route('/clear_devices')
def clear_devices():
    global known_devices
    known_devices = []
    mqtt_client.publish(CONFIG_TOPIC, json.dumps(known_devices))  # Envia lista vazia via MQTT
    return redirect(url_for('index'))


# Rota para remover um dispositivo detetado específico
@app.route('/remove_detected/<mac_address>', methods=['POST'])
def remove_detected(mac_address):
    global detected_devices
    detected_devices = [device for device in detected_devices if device != mac_address]
    return redirect(url_for('index'))


# Rota para limpar todos os dispositivos detetados
@app.route('/clear_detected')
def clear_detected():
    global detected_devices
    detected_devices = []
    return redirect(url_for('index'))


if __name__ == '__main__':
    app.run(debug=True)
