#include "IRmqttSerial.h"
#include "IRmqttGlobal.h"

#define WAITTING_TIME 2000  // MQTT 连接等待时间
#define HEARTBEAT_TIME 30000  // MQTT 重连间断时间

mqttClient::mqttClient(WiFiClient& wifi_client){
  this->_wifi_client = &wifi_client;
  this->_mqtt_client = new PubSubClient(*this->_wifi_client);
  _mqtt_client->setCallback(callback);
  _topic_head = String("/IRmqtt/" + getChipId());
} 

void mqttClient::reloadSettings() {
  _mqtt_client->disconnect();
  MQTT mqtt = user_settings.getMQTT();
  DEBUGLN("mqtt reload");
  _user = mqtt.user.c_str();
  _password = mqtt.password.c_str();
  _host = mqtt.host.c_str();
  _port = mqtt.port.toInt();
  Connect();

}

boolean mqttClient::Connect() {
  // mqtt 连接或重连
  if (_host != nullptr) {
    _mqtt_client->setServer(_host, _port);
    String client_id = getChipId();
    unsigned long start_time = millis();
    unsigned long end_time = millis();
    LOG("[Attempting MQTT connection]: ");
    while (end_time - start_time < WAITTING_TIME && !_mqtt_client->connected()) {
      if (_mqtt_client->connect(client_id.c_str(), _user, _password))
        _mqtt_client->subscribe((_topic_head + "/#").c_str());
      end_time = millis();
      LOG(". ");
      delay(500);
    }
    LOGLN(_mqtt_client->state());
  }
  return _mqtt_client->connected();
}

void mqttClient::Loop() {
  if ((int)(system_now_time - system_setup_time) % HEARTBEAT_TIME == 0) {
    if (!_mqtt_client->connected()) {
      if (!Connect())
        ESP.restart();  // 这只是暂时的选择，后期会改进重连的问题
    }
  }
  _mqtt_client->loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String payload_str = "";
  String topic_str(topic);
  for (unsigned int i = 0; i < length; i++) {
    payload_str += (char)payload[i];
  }
  DEBUGLN(topic);
  DEBUGLN(payload_str);
  irmqtt_ir.sendIR(topic_str, payload_str, false);
}
