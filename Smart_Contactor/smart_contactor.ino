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

// define topic mqtt & client id
#define TOPIC_LAMP "switch/lamp/1"
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
  <h1>Smart Contactor Config</h1>
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
String state_lamp = "false";

// relay
const int ssrPin[2] = {25, 33};

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
      client.subscribe(TOPIC_LAMP);
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
  if (String(topic) == CALLBACK_TOPIC_LAMP) {
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

    if (String(topic) == CALLBACK_TOPIC_LAMP)
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
  //relay
  pinMode(ssrPin[0], OUTPUT);
  pinMode(ssrPin[1], OUTPUT);
}

void setup(){
  Serial.begin(115200);
  init();
  wifiInit();
  mqttInit();
  webInit();
}

void toggleSSRRelay(int paramsRelay){
  if(paramsRelay == 0){
    digitalWrite(ssrPin[0], paramsRelay);
    digitalWrite(ssrPin[1], paramsRelay);
  }else{
    digitalWrite(ssrPin[0], !paramsRelay);  
    digitalWrite(ssrPin[1], paramsRelay);
  }
}

void loop(){
  mqttconnect();
  client.loop();

  if (millis() - lastTimer >= 300) {
    client.publish(TOPIC_LAMP, state_lamp.c_str());
    lastTimer = millis();
  }

  toggleSSRRelay(state_lamp == "true" ? LOW : HIGH);

  delay(200);
}
