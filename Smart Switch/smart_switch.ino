#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <dummy.h>

#define WRITE_MANUAL HIGH
#define WRITE_AUTO LOW

// untuk client wifi
WiFiClient espClient;
PubSubClient client(espClient);

// batas awal ganti variabel


// definisi wifi
const char *ssid = "IngfoSBM";
const char *password = "IngfoSBM";

// definisi ip
String mqtt_server = "192.168.137.1";
const int mqtt_port = 1883;

// Set your Static IP address
IPAddress local_IP(192, 168, 137, 100);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 137, 1);

// define topic mqtt
#define TOPIC_MODE "switch/mode/1"
#define TOPIC_LAMP "switch/lamp/1"
#define TOPIC_SAKLAR "switch/saklar/1"
#define CALLBACK_TOPIC_MODE "switch/callback/mode/1"
#define CALLBACK_TOPIC_LAMP "switch/callback/lamp/1"
String clientId = "LAMP-1";


// batas akhir ganti variabel


//webserver
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);
const char* queryParam = "q";
String inputMessage;
String inputParam;

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>MQTT Server Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>html { font-family:'Gill Sans', 'Gill Sans MT', Calibri, 'Trebuchet MS', sans-serif; display: inline-block; margin: 0px auto; text-align: center;}
body {
    height: 100vh;
    background-color: #f8f8ff;
    color: #12303D;
    box-sizing: border-box;
}
.hero{
    height: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    box-sizing: border-box;
    padding: 10vmin; 
}
.buttons{
    display: flex;
}
.button { 
    padding: 1.5vmin 4vmin;
    cursor: pointer;
    color: #ccc;
    background-color: #12303D;
    border-radius: 10px;
}
input[type='number']{
    width: 160px;
    height: 20px;
} 
</style>
</head>
<body>
  <div class="hero">
  <h1>Smart Switch Config</h1>
      <div class='buttons'>        
        <form action="/get">
            <label for="q"><b>Input MQTT Address</b></label>
            <input type='text' id='q' name='q'></br></br>
            <input type="submit" value="Submit" class="button">
        </form><br>
      </div>
  </div>
</body>


</html>)rawliteral";

// initial state 
String state_mode = "MANUAL";
String state_lamp = "false";
String state_saklar = "false";

// relay
const int relayPin = 27;

// ssr relay
const int ssrPin = 23;
//const int ssrPin = 4;
const int vccPin = 34;
float vccVal = 0;

// timer
unsigned long lastTimer;


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void webInit(){
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    inputMessage = request->getParam(queryParam)->value();
    mqtt_server=inputMessage;
    Serial.println(inputMessage);
    request->redirect("/");
    WiFi.reconnect();
//    request->send_P(200, "text/html", index_html);
  });
  
  server.onNotFound(notFound);
  server.begin();
}

void mqttconnect(){
  /* Loop until reconnected */
  while (!client.connected())
  {
    Serial.print("MQTT connecting ...");
    if (client.connect(clientId.c_str()))
    {
      Serial.println(" Connected");

      // led
      client.subscribe(TOPIC_MODE);
      client.subscribe(TOPIC_LAMP);
      client.subscribe(TOPIC_SAKLAR);
      client.subscribe(CALLBACK_TOPIC_MODE);
      client.subscribe(CALLBACK_TOPIC_LAMP);
    }
    else
    {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *message, unsigned int length){
  if (String(topic) == CALLBACK_TOPIC_MODE || String(topic) == CALLBACK_TOPIC_LAMP) {
    Serial.println();
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
      messageTemp += (char)message[i];
    }

    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    Serial.println(messageTemp);

    if (String(topic) == CALLBACK_TOPIC_MODE)
    {
      if (messageTemp == "AUTO") {
        toggleRelay(WRITE_AUTO);
        state_lamp = "true";
      }
      else if (messageTemp == "MANUAL") {
        toggleRelay(WRITE_MANUAL);
      }
      state_mode = messageTemp;
      Serial.println(state_mode);
    } else if (String(topic) == CALLBACK_TOPIC_LAMP)
    {
      state_lamp = messageTemp;
    }
  }
}

void mqttInit() {
  client.setServer(mqtt_server.c_str(), mqtt_port);
  client.setCallback(callback);
}

void wifiInit() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void init(){
  // relay
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, WRITE_MANUAL);

  //ssr
  pinMode(ssrPin, OUTPUT);

  // mode
  state_mode = "MANUAL";
}

void setup(){
  Serial.begin(115200);
  init();
  wifiInit();
  mqttInit();
  webInit();
  Serial.println(state_mode);
}

void toggleRelay(int paramsRelay){
  digitalWrite(relayPin, paramsRelay);
}

void toggleSSRRelay(int paramsRelay){
  digitalWrite(ssrPin, paramsRelay);
}

void ssr(){
  // read vcc
  vccVal = analogRead(vccPin);
  Serial.println();

  Serial.print("SSR Value: ");
  Serial.print(vccVal);
  Serial.print("; ");

  if (vccVal > 2000 ) {
    state_saklar = "true";
  } else {
    state_saklar = "false";
  }
}

void loop(){
  mqttconnect();
  client.loop();

  if (millis() - lastTimer >= 1000) {
    client.publish(TOPIC_MODE, state_mode.c_str());
    client.publish(TOPIC_LAMP, state_lamp.c_str());
    client.publish(TOPIC_SAKLAR, state_saklar.c_str());
    lastTimer = millis();
  }

  if (state_mode == "AUTO") {
    ssr();
    toggleSSRRelay(state_lamp == "true" ? HIGH : LOW);
  }

  delay(200);
}
