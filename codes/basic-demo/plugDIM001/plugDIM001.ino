/*  NETPIE ESP8266 basic sample                            */
/*  More information visit : https://netpie.io             */

#include <ESP8266WiFi.h>
#include <MicroGear.h>
#include <Wire.h>
#include <DHT.h>

#include "CMMC_Blink.hpp"
#include "CMMC_Interval.hpp"
CMMC_Blink blinker;
CMMC_Interval timer001;

const char* ssid     = "ESPERT-3020";
const char* password = "espertap";


#define APPID       "HelloNETPIE"
#define KEY         "IIHqbqzgkgy2jkQ"
#define SECRET      "XQUOQIk4KBLAKCP2gUReixMId"
#define ALIAS       "plugDIM001"


#include <WebSocketsServer.h>

WebSocketsServer webSocket = WebSocketsServer(81);


void dim(uint8_t* payload, size_t len) {
  char tmp[20];
  memcpy(tmp, payload, len);
  tmp[len] = '\0';

  String msg2 = String(tmp);
  int  b = atoi(msg2.c_str());
  if (b > 254) {
    b = 254;
  }
  if (b < 0) {
    b = 0;
  }
  Wire.beginTransmission(55); // transmit to device #8
  delay(2);
  Wire.write((uint8_t)b);              // sends one byte
  delay(2);
  Wire.endTransmission();    // stop transmitting

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  if (type == WStype_TEXT) {
    Serial.printf("[%u] get Text: %s\n", num, payload);

    dim(payload, lenght);

    // send message to client
    // webSocket.sendTXT(num, "message here");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
  }
  else if (type == WStype_CONNECTED)
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  }
}


WiFiClient client;
AuthClient *authclient;

#define DHTPIN 12
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

int timer = 0;
int relayPin = 15; //control relay pin

MicroGear microgear(client);

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
  String msg2 = String((char*)msg);
  int  b = atoi(msg2.c_str());
  if (b > 254)
    b = 254;
  if (b < 0)
    b = 0;
  Wire.beginTransmission(55); // transmit to device #8
  //delay(2);
  Wire.write((uint8_t)b);              // sends one byte
  //delay(2);
  Wire.endTransmission();    // stop transmitting

}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Found new member --> ");
  for (int i = 0; i < msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Lost member --> ");
  for (int i = 0; i < msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();
}

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  //on led when Connected to NETPIE
  analogWrite(LED_BUILTIN, 0); //LED_BUILTIN use avctive Low to On
  /* Set the alias of this microgear ALIAS */
  microgear.setName(ALIAS);
}


void setup() {
  /* Add Event listeners */
  /* Call onMsghandler() when new message arraives */
  microgear.on(MESSAGE, onMsghandler);

  /* Call onFoundgear() when new gear appear */
  microgear.on(PRESENT, onFoundgear);

  /* Call onLostgear() when some gear goes offline */
  microgear.on(ABSENT, onLostgear);

  /* Call onConnected() when NETPIE connection is established */
  microgear.on(CONNECTED, onConnected);

  pinMode(relayPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin(); // join i2c bus (address optional for master)
  Wire.setClock(400000);
  delay(100);
  dht.begin();
  blinker.init();
  Serial.begin(115200);
  Serial.println("Starting...");

  blinker.blink(50, LED_BUILTIN);
  delay(200);

  if (WiFi.begin(ssid, password)) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }

  Serial.println(WiFi.localIP());
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);


  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  blinker.blink(200, LED_BUILTIN);
  /* Initial with KEY, SECRET and also set the ALIAS here */
  microgear.init(KEY, SECRET, ALIAS);

  /* connect to NETPIE to a specific APPID */
  microgear.connect(APPID);

  // connected to netpie so turn off the led
  blinker.detach();
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  /* To check if tevhe microgear is still connected */
  /* Call this method regularly otherwise the connection may be lost */

  webSocket.loop();
  microgear.loop();
  if (microgear.connected()) {
    timer001.every_ms(2000, [&]() {
      Serial.print("Publish... ");
      //******  read DHT sensor very 2sec
      float h = 0.00f;
      float t = 0.00f;
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      t = dht.readTemperature();

      if (isnan(h) || isnan(t)) {
        Serial.println("Failed to read from DHT sensor!");
        //return;
      }
      else {
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.println(" *C ");

        /* Chat with the microgear named ALIAS which is myself */
        //microgear.chat("plug001/temp", (String)t);
        //microgear.chat("plug001/humid", (String)h);

        char topic_temp[MAXTOPICSIZE];
        char topic_humid[MAXTOPICSIZE];
        sprintf(topic_temp, "/gearname/%s/temp", ALIAS);
        sprintf(topic_humid, "/gearname/%s/humid", ALIAS);
        //retain message
        microgear.publish(topic_temp, String(t), true);
        microgear.publish(topic_humid, String(h), true);
      }
    });
  }
  else {
    Serial.println("DIS CONNECTED");
    microgear.connect(APPID);
  }

}
