#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

// Update these with values suitable for your network.
const char* ssid[] = {"G5_3766","Nguyen Van Thien"};
const char* password[] = {"012345679","thien1993@"};
const char* mqtt_server = "fd3b3ad84b6246349a56fabb5dfbc4dc.s1.eu.hivemq.cloud";

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;
byte sendcode[5];

void setup_wifi() {
  delay(10);
  int i = 0;
  // We start by connecting to a WiFi network
  //Serial.println();
//  Serial.print("Connecting to ");
 // Serial.println(ssid);
 while(WiFi.status() != WL_CONNECTED){
  int n = 0;
  WiFi.mode(WIFI_STA);
  if(i < sizeof(ssid)/2) WiFi.begin(ssid[i], password[i]);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    n++;
    if(n>10) break;
  }
  i++;
 }

  randomSeed(micros());

  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
}


void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

 // Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
   // Serial.print(".");
   now = time(nullptr);
  }
  //Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  //Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}


void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  String hexChar = "";
  for (int i = 0; i < length; i++) {
     hexChar += String((char)payload[i]);
  }
  uint16_t code = (uint16_t) strtol(hexChar.c_str(),NULL,0);
  sendcode[0] = 0xA1;
  sendcode[3] = code & 0xff;
  sendcode[2] = (code >> 8) & 0xff;
  sendcode[4] = 0x00;
  Serial.write(sendcode,5);
  //Serial.println("received");
  
}


void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    //Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "nvt1011", "Nvt101120")) {
      //Serial.println("connected");
      // Once connected, publish an announcement…
      //client->publish("Topic", "msg");
      // … and resubscribe
      client->subscribe("TV/Samsung");
      client->subscribe("Fan/Samsung");
    } else {
      //Serial.print("failed, rc = ");
      //Serial.print(client->state());
      //client->state();
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  delay(500);
  // When opening the Serial Monitor, select 9600 Baud
  Serial.begin(9600);
  sendcode[1] = 0xF1;
  //Serial1.begin(9600);
  delay(500);

  LittleFS.begin();
  setup_wifi();
  setDateTime();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  // you can use the insecure mode, when you want to avoid the certificates
 // client->setInsecure();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  //Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    //Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  client = new PubSubClient(*bear);

  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
}

void loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
}
