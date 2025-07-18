#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ===== CONFIGURAÇÕES GLOBAIS =====
struct Config {
  static const char* WIFI_SSID;
  static const char* WIFI_PASSWORD;
  static const char* MQTT_BROKER;
  static const int MQTT_PORT = 1883;
  static const char* MQTT_CLIENT_ID;
  static const char* MQTT_TOPIC_LIGHT;
  static const char* MQTT_TOPIC_DHT;
  static const char* MQTT_TOPIC_STATUS;
  static const int PHOTOSENSOR_PIN = 32;
  static const int LED_PIN = 19;
  static const int DHT_PIN = 2;
  static const int LIGHT_THRESHOLD = 800;
  static const int LCD_ADDRESS = 0x27;
  static const int LCD_COLS = 16;
  static const int LCD_ROWS = 2;
  static const int DHT_TYPE = DHT11;
};

const char* Config::WIFI_SSID = "";
const char* Config::WIFI_PASSWORD = "";
const char* Config::MQTT_BROKER = "";
const char* Config::MQTT_CLIENT_ID = "";
const char* Config::MQTT_TOPIC_LIGHT = "";
const char* Config::MQTT_TOPIC_DHT = "";
const char* Config::MQTT_TOPIC_STATUS = "";

// ===== CLASSE PARA CONTROLE DE TELAS =====
class ScreenManager {
public:
  ScreenManager() : currentScreen(0), lastScreenChange(0), screenInterval(5000) {}
  
  bool shouldChangeScreen() {
    if (millis() - lastScreenChange >= screenInterval) {
      currentScreen = (currentScreen + 1) % 3;  // Alterna entre 0, 1 e 2
      lastScreenChange = millis();
      String screenName;
      switch(currentScreen) {
        case 0: screenName = "Luz/LED"; break;
        case 1: screenName = "Temperatura/Humidade"; break;
        case 2: screenName = "Status MQTT"; break;
      }
      Serial.println("Tela alterada automaticamente para: " + screenName);
      return true;
    }
    return false;
  }
  
  int getCurrentScreen() const { return currentScreen; }
  
  void resetTimer() {
    lastScreenChange = millis();
  }

private:
  int currentScreen;
  unsigned long lastScreenChange;
  unsigned long screenInterval;
};

// ===== CLASSE PARA SENSOR DHT11 =====
class DHTSensor {
public:
  DHTSensor() : dht(Config::DHT_PIN, Config::DHT_TYPE), temperature(0.0), humidity(0.0) {}
  
  void initialize() {
    dht.begin();
    delay(2000);
  }
  
  void readSensor() {
    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();
    
    if (!isnan(newTemp) && !isnan(newHum)) {
      temperature = newTemp;
      humidity = newHum;
      Serial.printf("DHT11 - Temp: %.2f ºC | Hum: %.1f%%\n", temperature, humidity);
    } else {
      Serial.println("Erro na leitura do DHT11!");
    }
  }
  
  float getTemperature() const { return temperature; }
  float getHumidity() const { return humidity; }
  
  String getTemperatureString() const {
    return String(temperature, 2) + " ºC";
  }
  
  String getHumidityString() const {
    return String(humidity, 1) + "%";
  }

private:
  DHT dht;
  float temperature;
  float humidity;
};

// ===== CLASSE PARA GERENCIAR WIFI =====
class WiFiManager {
public:
  WiFiManager() : connected(false) {}
  
  bool connect() {
    WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    
    connected = true;
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  
  bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }
  
  void reconnect() {
    if (!isConnected()) {
      Serial.println("WiFi desconectado! Reconectando...");
      WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
      delay(5000);
    }
  }
  
  String getLocalIP() {
    return WiFi.localIP().toString();
  }

private:
  bool connected;
};

// ===== CLASSE PARA GERENCIAR LCD =====
class LCDDisplay {
public:
  LCDDisplay() : lcd(Config::LCD_ADDRESS, Config::LCD_COLS, Config::LCD_ROWS), animationStep(0) {}
  
  void initialize() {
    lcd.begin();
    lcd.backlight();
    showMessage("Iniciando...", "");
  }
  
  void showMessage(String line1, String line2 = "") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    if (line2.length() > 0) {
      lcd.setCursor(0, 1);
      lcd.print(line2);
    }
  }
  
  void showConnectingStart() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conectando WiFi");
    lcd.setCursor(0, 1);
    lcd.print("               ");
    animationStep = 0;
  }
  
  void animateConnecting() {
    lcd.setCursor(0, 1);
    lcd.print("               ");
    
    lcd.setCursor(0, 1);
    for (int i = 0; i <= animationStep; i++) {
      lcd.print("o");
    }
    
    animationStep = (animationStep + 1) % 4;
    
    if (animationStep == 3) {
      delay(300);
    }
  }
  
  void showConnected(String ip) {
    showMessage("WiFi OK!", ip);
    delay(2000);
  }
  
  void showMQTTConnecting() {
    showMessage("Conectando", "MQTT...");
  }
  
  void showMQTTConnected() {
    showMessage("MQTT OK!", "HiveMQ");
    delay(2000);
  }
  
  void showSensorData(int lightValue, bool ledState, int threshold) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Luz:" + String(lightValue));
    lcd.setCursor(0, 1);
    lcd.print("LED:" + String(ledState ? "ON " : "OFF"));
    lcd.setCursor(8, 1);
    lcd.print("T:" + String(threshold));
  }
  
  void showDHTData(float temperature, float humidity) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(temperature, 2) + " C");
    lcd.setCursor(0, 1);
    lcd.print("Humidade: " + String(humidity, 1) + "%");
  }
  
  void showMQTTStatus(bool connected, int messagesSent) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT: " + String(connected ? "OK" : "OFF"));
    lcd.setCursor(0, 1);
    lcd.print("Msgs: " + String(messagesSent));
  }
  
  void showMQTTPublishStatus(bool success) {
    lcd.setCursor(13, 0);
    if (success) {
      lcd.print("OK ");
    } else {
      lcd.print("ERR");
    }
  }
  
  void showWiFiLost() {
    showMessage("WiFi perdido!", "Reconectando...");
  }
  
  void showMQTTLost() {
    showMessage("MQTT perdido!", "Reconectando...");
  }

private:
  LiquidCrystal_I2C lcd;
  int animationStep;
};

// ===== CLASSE PARA SENSOR DE LUZ E LED =====
class LightSensor {
public:
  LightSensor() : ledState(false), lastLedState(false), lightValue(0) {
    pinMode(Config::LED_PIN, OUTPUT);
    digitalWrite(Config::LED_PIN, LOW);
  }
  
  void readSensor() {
    lightValue = analogRead(Config::PHOTOSENSOR_PIN);
    
    bool newLedState = (lightValue < Config::LIGHT_THRESHOLD);
    
    if (newLedState != ledState) {
      ledState = newLedState;
      digitalWrite(Config::LED_PIN, ledState ? HIGH : LOW);
      
      Serial.printf("LED %s - Luminosidade: %d\n", 
                   ledState ? "LIGADO" : "DESLIGADO", lightValue);
    }
    
    Serial.printf("Valor do fotossensor: %d | Threshold: %d | LED: %s\n", 
                  lightValue, Config::LIGHT_THRESHOLD, ledState ? "ON" : "OFF");
  }
  
  bool hasStateChanged() {
    bool changed = (ledState != lastLedState);
    if (changed) {
      lastLedState = ledState;
    }
    return changed;
  }
  
  int getLightValue() const { return lightValue; }
  bool getLedState() const { return ledState; }
  
  String getJsonData() const {
    JsonDocument doc;
    doc["light_value"] = lightValue;
    doc["led_state"] = ledState;
    doc["threshold"] = Config::LIGHT_THRESHOLD;
    doc["timestamp"] = millis();
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
  }

private:
  bool ledState;
  bool lastLedState;
  int lightValue;
};

// ===== CLASSE PARA COMUNICAÇÃO MQTT =====
class MQTTClient {
public:
  MQTTClient() : client(wifiClient), messagesSent(0), lastReconnectAttempt(0) {}
  
  void initialize() {
    client.setServer(Config::MQTT_BROKER, Config::MQTT_PORT);
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
      this->onMessageReceived(topic, payload, length);
    });
  }
  
  bool connect() {
    if (client.connect(Config::MQTT_CLIENT_ID)) {
      Serial.println("MQTT conectado ao HiveMQ!");
      
      // Publica mensagem de status
      publishStatus("online");
      
      // Se inscreve em tópicos de controle (opcional)
      client.subscribe("esp32/control/led");
      
      return true;
    } else {
      Serial.printf("Falha na conexão MQTT. Estado: %d\n", client.state());
      return false;
    }
  }
  
  void loop() {
    if (!client.connected()) {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > 5000) {
        lastReconnectAttempt = now;
        if (connect()) {
          lastReconnectAttempt = 0;
        }
      }
    } else {
      client.loop();
    }
  }
  
  bool publishLightData(const String& jsonData) {
    if (client.connected()) {
      bool success = client.publish(Config::MQTT_TOPIC_LIGHT, jsonData.c_str());
      if (success) {
        messagesSent++;
        Serial.println("Dados de luz publicados: " + jsonData);
      }
      return success;
    }
    return false;
  }
  
  bool publishDHTData(float temperature, float humidity) {
    if (client.connected()) {
      JsonDocument doc;
      doc["temperature"] = temperature;
      doc["humidity"] = humidity;
      doc["timestamp"] = millis();
      
      String jsonString;
      serializeJson(doc, jsonString);
      
      bool success = client.publish(Config::MQTT_TOPIC_DHT, jsonString.c_str());
      if (success) {
        messagesSent++;
        Serial.println("Dados DHT publicados: " + jsonString);
      }
      return success;
    }
    return false;
  }
  
  bool publishStatus(const String& status) {
    if (client.connected()) {
      JsonDocument doc;
      doc["status"] = status;
      doc["client_id"] = Config::MQTT_CLIENT_ID;
      doc["timestamp"] = millis();
      
      String jsonString;
      serializeJson(doc, jsonString);
      
      return client.publish(Config::MQTT_TOPIC_STATUS, jsonString.c_str());
    }
    return false;
  }
  
  bool isConnected() const {
    return client.connected();
  }
  
  int getMessagesSent() const {
    return messagesSent;
  }

private:
  WiFiClient wifiClient;
  PubSubClient client;
  int messagesSent;
  unsigned long lastReconnectAttempt;
  
  void onMessageReceived(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    
    Serial.printf("Mensagem recebida [%s]: %s\n", topic, message.c_str());
    
    // Exemplo de controle remoto do LED
    if (String(topic) == "esp32/control/led") {
      if (message == "on") {
        digitalWrite(Config::LED_PIN, HIGH);
        Serial.println("LED ligado via MQTT");
      } else if (message == "off") {
        digitalWrite(Config::LED_PIN, LOW);
        Serial.println("LED desligado via MQTT");
      }
    }
  }
};

// ===== CLASSE PRINCIPAL DO SISTEMA =====
class IoTSystem {
public:
  IoTSystem() {}
  
  void initialize() {
    Serial.begin(115200);
    
    // Inicializa componentes
    display.initialize();
    dhtSensor.initialize();
    mqttClient.initialize();
    
    // Conecta WiFi
    display.showConnectingStart();
    if (wifiManager.connect()) {
      display.showConnected(wifiManager.getLocalIP());
    }
    
    // Conecta MQTT
    display.showMQTTConnecting();
    if (mqttClient.connect()) {
      display.showMQTTConnected();
    }
    
    // Leitura inicial dos sensores
    sensor.readSensor();
    dhtSensor.readSensor();
    
    // Mostra tela inicial e inicia timer
    updateDisplay();
    screenManager.resetTimer();
    
    lastDHTPublish = millis();
  }
  
  void loop() {
    // Mantém conexão MQTT
    mqttClient.loop();
    
    // Lê sensores
    sensor.readSensor();
    dhtSensor.readSensor();
    
    // Verifica se deve alternar tela automaticamente
    bool screenChanged = screenManager.shouldChangeScreen();
    
    // Atualiza display
    updateDisplay();
    
    // Verifica conexão WiFi
    if (!wifiManager.isConnected()) {
      display.showWiFiLost();
      wifiManager.reconnect();
      screenManager.resetTimer();
      return;
    }
    
    // Verifica conexão MQTT
    if (!mqttClient.isConnected()) {
      display.showMQTTLost();
      delay(1000);
      screenManager.resetTimer();
    }
    
    // Envia dados se houve mudança no LED
    if (sensor.hasStateChanged()) {
      Serial.println("Estado do LED mudou! Enviando via MQTT...");
      
      bool success = mqttClient.publishLightData(sensor.getJsonData());
      
      // Mostra status no LCD brevemente
      display.showMQTTPublishStatus(success);
      delay(1000);
      updateDisplay();
      screenManager.resetTimer();
    }
    
    // Envia dados do DHT a cada 30 segundos
    if (millis() - lastDHTPublish > 30000) {
      mqttClient.publishDHTData(dhtSensor.getTemperature(), dhtSensor.getHumidity());
      lastDHTPublish = millis();
    }
    
    delay(1000);
  }

private:
  void updateDisplay() {
    switch(screenManager.getCurrentScreen()) {
      case 0:
        // Tela 1: Sensor de luz e LED
        display.showSensorData(sensor.getLightValue(), sensor.getLedState(), Config::LIGHT_THRESHOLD);
        break;
      case 1:
        // Tela 2: Temperatura e humidade
        display.showDHTData(dhtSensor.getTemperature(), dhtSensor.getHumidity());
        break;
      case 2:
        // Tela 3: Status MQTT
        display.showMQTTStatus(mqttClient.isConnected(), mqttClient.getMessagesSent());
        break;
    }
  }

  WiFiManager wifiManager;
  LCDDisplay display;
  LightSensor sensor;
  DHTSensor dhtSensor;
  ScreenManager screenManager;
  MQTTClient mqttClient;
  unsigned long lastDHTPublish;
};

// ===== INSTÂNCIA GLOBAL E FUNÇÕES PRINCIPAIS =====
IoTSystem iotSystem;

void setup() {
  iotSystem.initialize();
}

void loop() {
  iotSystem.loop();
}
