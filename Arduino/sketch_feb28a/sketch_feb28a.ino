#include <IRremote.hpp>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <stdlib.h>
#include <stdint.h>

#define IR_TRANS_PIN D2
#define IR_RECV_PIN D3
IRsend irsend(IR_TRANS_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;

// Update these with values suitable for your network.
const char* ssid[] = {"Redmi 10C","G5_3766","Nguyen Van Thien","Little Penguin","Phong 314"};
const char* password[] = {"19tuxuong","012345679","thien1993@","khongcopass","0394998532"};
const char* IRprotocol[] = {"UNKNOWN","PULSE_DISTANCE","PULSE_WIDTH","DENON","DISH","JVC","LG",
    "LG2",
    "NEC",
    "PANASONIC",
    "KASEIKYO",
    "KASEIKYO_JVC",
    "KASEIKYO_DENON",
    "KASEIKYO_SHARP",
    "KASEIKYO_MITSUBISHI",
    "RC5",
    "RC6",
    "SAMSUNG",
    "SHARP",
    "SONY",
    "ONKYO",
    "APPLE",
    "BOSEWAVE",
    "LEGO_PF",
    "MAGIQUEST",
    "WHYNTER"};
const char* mqtt_server = "fd3b3ad84b6246349a56fabb5dfbc4dc.s1.eu.hivemq.cloud";

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * client;
uint8_t * receive_buffer;


void setup_wifi() {
  delay(10);
  int i = 0;
  // We start by connecting to a WiFi network
 while(WiFi.status() != WL_CONNECTED){
  int n = 0;
  WiFi.mode(WIFI_STA);
  if(i < sizeof(ssid)/4){
    WiFi.begin(ssid[i], password[i]);
    String s = "Connecting to "+String(ssid[i]);
    Serial.print(s);
  }
  else i = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    n++;
    if(n>10) break;
  }
  i++;
 }
 String s = "Connected to "+String(ssid[i-1]);
   Serial.println(s);
  randomSeed(micros());
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
  receive_buffer = (uint8_t*)realloc(receive_buffer,(length-1)*sizeof(uint8_t));
  for (int i = 1; i < length; i++) {
     receive_buffer[i-1] = (uint8_t) payload[i];
     Serial.println(receive_buffer[i-1]);
  }
  irsend.sendRaw_P(receive_buffer,length-1,38);
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
      client->subscribe("TV");
      client->subscribe("Fan");
      client->subscribe("Customs");
      //client->subscribe("Learn");
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
  irrecv.enableIRIn();
  //sendcode[1] = 0xF1;
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

void sendUI(uint8_t *code_buffer)
{
  uint16_t vol = 0;
  uint16_t amp = 0;
  int bit_cnt = 0;
  for(int i = 4; i <= 68; i+=2)
  {
    if(bit_cnt < 16)
    {
      if(code_buffer[i] < 0xf && code_buffer[i+1] < 0xf) amp = amp | (0 << bit_cnt);
      else amp = amp | (1 << bit_cnt);
    }
    else 
    {
      if(code_buffer[i] < 0xf && code_buffer[i+1] < 0xf) vol = vol | (0 << (bit_cnt - 16));
      else vol = vol | (1 << (bit_cnt - 16));
    }
    bit_cnt++;
  }
  String detail = "Device detail: "+String(vol)+" milivol, "+String(amp)+" miliamperre, "+String(vol*amp)+" microWatt.";
  Serial.println(detail);
}

void loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
  uint64_t value = 0;
  if (irrecv.decode(&results)){
    //Serial.println(IRprotocol[results.decode_type]);
    //if(results.rawlen > 6) decode_more_64bit(results);
      uint8_t * code_buffer = (uint8_t*)malloc(results.rawlen*sizeof(uint8_t));
     for(int i = 0;i<results.rawlen;i++)
     {
        code_buffer[i] = results.rawbuf[i] & 0xff;
        Serial.println(code_buffer[i]);
     }
    if(code_buffer[3] > 200) sendUI(code_buffer);
    else
    {
    Serial.println(IRprotocol[results.decode_type]);
    client->publish("Learn",code_buffer,(int)results.rawlen);
    }
    irrecv.resume();
  }
}
