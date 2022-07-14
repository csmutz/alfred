//ALFRED: garage door opener and status

//Blue light status:
//  blinking during startup: connecting to Wifi/starting server
//  On when door open
//  quick blink when activating relays

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#define HTTPPORT 80

//how long door switch button should be "pushed"
#define DOOR_SWITCH_INTERVAL 100
//how long door takes to open/close
#define DOOR_MOVE_TIME 13

#define RELAY1_PIN 13
#define RELAY2_PIN 12
#define SENSOR_PIN 15

const char* ssid     = "wifi_ssid";
const char* password = "wifi_pass";
const char* hostname = "alfred";
unsigned int wifi_last_connected = 0;

const char* html_template = "<!doctype html>\r\n\
<html lang='en'>\r\n\
<head>\r\n\
  <meta charset='utf-8'>\r\n\
  <meta http-equiv='refresh' content='5'>\r\n\
  <title>Alfred</title>\r\n\
</head>\r\n\
<body style='background-color:#%s;'>\r\n\
<p style='font-size: 150px; font-weight: 900;'>%s</p>\r\n\
<div style='text-align:center;'/>\r\n\
<form method='get'>\r\n\
    <input type='hidden' name='switch' value='%lu'/>\r\n\
    <input type='submit' value='' style='height:800px; width:800px; border-color:#000; background: #aaa;'/>\r\n\
</form>\r\n\
</div>\r\n\
<p style='font-size: 75px; font-weight: 900;'>%s: %u hours ago</p>\r\n\
</body>\r\n\
</html>";

unsigned int last_switch = 0;
bool last_switch_state = 0;
unsigned int last_open = 0;

WebServer server(HTTPPORT);

//blink LED and output to Serial while connecting to WIFI
void wifi_blink_while_connecting()
{
  Serial.println("");
  Serial.println("Connecting to Wifi");

  int i = 2;
  while (WiFi.status() != WL_CONNECTED) {
    i++;
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, i % 2);
  }
  
  Serial.println("");
  Serial.print("Connected to Wifi: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  wifi_last_connected = time(NULL);
}

void switch_door()
{
  Serial.println("Switching Door");
  last_switch_state = is_open();
  last_switch = time(NULL);
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
  digitalWrite(RELAY1_PIN, HIGH);
  delay(DOOR_SWITCH_INTERVAL);
  digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
  digitalWrite(RELAY1_PIN, LOW);
}

bool is_open()
{
  return digitalRead(SENSOR_PIN);
}

void handle_req() {
  char response[1000];
  char *state;
  char *open_time;
  char *bg;
  
  Serial.print("Request from: ");
  Serial.println(server.client().remoteIP());

  if (server.hasArg("switch"))
  {
    switch_door();
    //server.sendHeader("Location", server.uri().c_str());
    server.sendHeader("Location", "./");
    server.send(301);
  } else
  {
    if ((time(NULL) < last_switch + DOOR_MOVE_TIME) && (last_switch > 0))
    {
      if (last_switch_state)
      {
        bg = "FFB";
        if (time(NULL) > last_switch + 8)
        {
          state = "CLOSING..";
        } else if (time(NULL) > last_switch + 3)
        {
          state = "CLOSING.";
        } else
        {
          state = "CLOSING";
        }
      } else
      {
        bg = "FFB";
        if (time(NULL) > last_switch + 8)
        {
          state = "OPENING..";
        } else if (time(NULL) > last_switch + 3)
        {
          state = "OPENING.";
        } else
        {
          state = "OPENING";
        }
      }
    } else
    {
      if(is_open())
      {
        bg = "FBB";
        state = "OPEN";
      } else
      {
        bg = "BFB";
        state = "CLOSED";
      }
    }
    
    if (last_open)
    {
      snprintf(response, sizeof(response), html_template, bg, state, millis(),  "Last Open", (time(NULL) - last_open)/3600);  
    } else
    {
      snprintf(response, sizeof(response), html_template, bg, state, millis(), "Uptime", time(NULL)/3600);
    }

    server.send(200, "text/html", response);
  }
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
 
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Starting Alfred Garage Door Manager");
  wifi_blink_while_connecting();

  server.on("/", handle_req);
  server.onNotFound(handle_req);
  server.begin();

  Serial.print("HTTP server started on port: ");
  Serial.println(HTTPPORT);

  pinMode(RELAY1_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY2_PIN, LOW);

  pinMode(SENSOR_PIN, INPUT_PULLUP);
}

void loop(void) {
  server.handleClient();
  delay(10);
  if (is_open())
  {
    last_open = time(NULL);
    digitalWrite(LED_BUILTIN, HIGH);
  } else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_last_connected = time(NULL);
  } else 
  {
    if ((time(NULL) - wifi_last_connected) > 600)
    {
      WiFi.reconnect();
      wifi_blink_while_connecting();
    }
  }
}
