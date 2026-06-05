#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <Preferences.h>

// ================= WIFI =================

const char* ssid = "ESPTEST";
const char* password = "12345678";

// Static IP
IPAddress local_IP(10,249,244,50);
IPAddress gateway(10,249,244,1);
IPAddress subnet(255,255,255,0);

// ================= OBJECTS =================

WebServer server(80);
WiFiUDP udp;
Preferences prefs;

unsigned int udpPort = 4210;
char incomingPacket[255];

// ================= HARDWARE =================

// Relay GPIO
int relayPin[6] =
{
  23,22,21,19,18,5
};

// Manual Switch GPIO
int switchPin[6] =
{
  32,33,27,14,12,13
};

bool relayState[6];

unsigned long lastSwitchTime[6];

const int debounceDelay = 150;


// ================= RELAY =================

void applyRelayState(int i)
{
  digitalWrite(
    relayPin[i],
    relayState[i] ? HIGH : LOW
  );
}


// ================= MEMORY =================

void saveState()
{
  for(int i=0;i<6;i++)
  {
    String key="r"+String(i);
    prefs.putBool(
      key.c_str(),
      relayState[i]
    );
  }
}


void loadState()
{
  for(int i=0;i<6;i++)
  {
    String key="r"+String(i);

    relayState[i] =
    prefs.getBool(
      key.c_str(),
      false
    );
  }
}


// ================= API =================

void relayON(int i)
{
  relayState[i]=true;
  applyRelayState(i);
  saveState();

  server.send(
    200,
    "text/plain",
    "ON"
  );
}


void relayOFF(int i)
{
  relayState[i]=false;
  applyRelayState(i);
  saveState();

  server.send(
    200,
    "text/plain",
    "OFF"
  );
}


void statusAPI()
{
  String json="{";

  for(int i=0;i<6;i++)
  {
    json += "\"relay";
    json += String(i+1);
    json += "\":";
    json += relayState[i] ?
    "true":"false";

    if(i<5)
    json+=",";
  }

  json+="}";

  server.send(
    200,
    "application/json",
    json
  );
}


// ================= INFO API =================

void infoAPI()
{
 String json="{";

 json+="\"device\":\"HomeOS\",";
 json+="\"ip\":\"";
 json+=WiFi.localIP().toString();
 json+="\",";
 json+="\"relays\":6";

 json+="}";

 server.send(
  200,
  "application/json",
  json
 );
}


// ================= SETUP =================

void setup()
{

Serial.begin(115200);

prefs.begin(
 "relayState",
 false
);

loadState();


for(int i=0;i<6;i++)
{
 pinMode(
 relayPin[i],
 OUTPUT
 );

 pinMode(
 switchPin[i],
 INPUT_PULLUP
 );

 applyRelayState(i);
}


// WIFI

WiFi.mode(WIFI_STA);

WiFi.config(
 local_IP,
 gateway,
 subnet
);

WiFi.begin(
 ssid,
 password
);


Serial.println(
"Connecting..."
);


while(
WiFi.status()!=WL_CONNECTED
)
{
 delay(500);
 Serial.print(".");
}


Serial.println();
Serial.println(
"WiFi Connected"
);

Serial.println(
WiFi.localIP()
);


// MDNS

if(
MDNS.begin("homeos")
)
{
Serial.println(
"http://homeos.local"
);
}


// UDP

udp.begin(
udpPort
);


// HTTP ROUTES

server.on(
"/",
[]()
{
server.send(
200,
"text/plain",
"HomeOS Running"
);
}
);


server.on(
"/status",
statusAPI
);


server.on(
"/info",
infoAPI
);


// RELAY API

server.on("/r1on",[]{relayON(0);});
server.on("/r1off",[]{relayOFF(0);});

server.on("/r2on",[]{relayON(1);});
server.on("/r2off",[]{relayOFF(1);});

server.on("/r3on",[]{relayON(2);});
server.on("/r3off",[]{relayOFF(2);});

server.on("/r4on",[]{relayON(3);});
server.on("/r4off",[]{relayOFF(3);});

server.on("/r5on",[]{relayON(4);});
server.on("/r5off",[]{relayOFF(4);});

server.on("/r6on",[]{relayON(5);});
server.on("/r6off",[]{relayOFF(5);});


server.begin();

Serial.println(
"HomeOS Server Started"
);

}


// ================= LOOP =================


void loop()
{

server.handleClient();


// WIFI RECOVERY

if(
WiFi.status()!=WL_CONNECTED
)
{
 WiFi.reconnect();
}


// SWITCH CHECK

for(int i=0;i<6;i++)
{

if(
digitalRead(
switchPin[i]
)==LOW
)
{

if(
millis()-lastSwitchTime[i]
>
debounceDelay
)
{

relayState[i]=
!relayState[i];

applyRelayState(i);

saveState();

lastSwitchTime[i]=
millis();

}

}

}


// UDP DISCOVERY

int packetSize =
udp.parsePacket();


if(packetSize)
{

int len =
udp.read(
incomingPacket,
255
);


if(len>0)
incomingPacket[len]=0;


String msg =
String(incomingPacket);


if(
msg=="DISCOVER_HOMEOS"
)
{

String response="{";

response +=
"\"device\":\"HomeOS\",";

response +=
"\"ip\":\"";

response +=
WiFi.localIP()
.toString();

response +=
"\",";

response +=
"\"relays\":6";

response+="}";


udp.beginPacket(
udp.remoteIP(),
udp.remotePort()
);

udp.print(
response
);

udp.endPacket();

}

}

}
