# O text utils do presente código encontra-se no domínio: https://github.com/abraaosantosdeveloper/TextUtils

import os
import paho.mqtt.client as mqtt
import ssl
import time
import text_utils as utils
from text_utils import printBrightGreen, gotoxy


class SimpleSubscriber:
    def __init__(self):
        self.broker = "75632848a5f64acb9266c92a7e4f9eb2.s1.eu.hivemq.cloud"
        self.port = 8883
        self.username = "Testing_Credential"
        self.password = "Testing@1234"
        self.client_id = "Python_Sub_001"
        self.connected = False
        self.topics = ["#"]
        
    def on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.connected = True
            print("Conectado! Subscrevendo nos tópicos...")
            
            # Subscreve em tópicos específicos
            for topic in self.topics:
                result = client.subscribe(topic)
                print(f"Tentando subscrever no tópico: {topic} - Resultado: {result}")
                
        else:
            print(f"Falha na conexão: {rc}")
    
    def on_subscribe(self, client, userdata, mid, granted_qos, properties=None):
        utils.printBrightGreen(f"Subscrição confirmada! MID: {mid}, QoS: {granted_qos}")
        time.sleep(1)
        os.system('cls')
        utils.printTitleBarRoundBorder("Mensagens recebidas:", utils.BRIGHT_GREEN, utils.BRIGHT_MAGENTA)
        print("\n")

    def on_message(self, client, userdata, msg):
        message_count = 0
        topic = msg.topic
        payload = msg.payload.decode('utf-8')
        qos = msg.qos
        message_count += 1

        print(f"Tópico: {topic:50}||\t{payload:30}||\tQos: {qos:5}")
        print("-" * 120)
    
    def on_disconnect(self, client, userdata, rc):
        self.connected = False
        print("Conexão perdida!")
    
    def start(self):
        client = mqtt.Client(client_id=self.client_id, callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
        
        # Configuração TLS para HiveMQ Cloud
        client.tls_set(ca_certs=None, certfile=None, keyfile=None, 
                      cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS,
                      ciphers=None)
        
        # Usando suas credenciais
        client.username_pw_set(self.username, self.password)
        
        client.on_connect = self.on_connect
        client.on_message = self.on_message
        client.on_subscribe = self.on_subscribe
        client.on_disconnect = self.on_disconnect

        
        try:
            print(f"Tentando conectar ao broker: {self.broker}:{self.port}")
            client.connect(self.broker, self.port, 60)
            
            # Loop aguardando mensagens
            print("Aguardando mensagens... (Pressione Ctrl+C para sair)")
            time.sleep(1)
            os.system('cls')

            while True:
                client.loop(timeout=1.0)
                time.sleep(0.1)


        except KeyboardInterrupt:
            print("\nDesconectando...")
            client.disconnect()
        except Exception as e:
            print(f"Erro: {e}")
            client.disconnect()




if __name__ == "__main__":
    subscriber = SimpleSubscriber()
    subscriber.start()
