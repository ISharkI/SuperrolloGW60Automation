#include <ESP8266WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include "FS.h" // Allow to store variable

// WiFi
const char* ssid = "WIFI";
const char* wifi_password = "PASSWORD";
WiFiClient wifiClient;

// Variables
int dir = 0, shadeposition = 0, count = 0, maxcount, newpercentage = 0;
float percentage = 0;
unsigned long previousMillis, currentMillis; // Create a timer
bool remote = false;

// Check if Numeric
boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}

// function for up
void up()
{
      Serial.println("Bringing Shaders up");
      digitalWrite(D2, LOW);
      delay(100);
      digitalWrite(D2, HIGH);    
}

// function for down
void down()
{  
      Serial.println("Bringing Shaders down");
      digitalWrite(D1, LOW);
      delay(100);
      digitalWrite(D1, HIGH);  
}

// MQTT
const char* mqtt_server = "192.168.178.4";
const char* mqtt_username = "gw60-1";
const char* mqtt_password = "aIcQ4tEd";
const char* mqtt_clientID = "Wemos-Livingroom";
char message_buff[100];

// MQTT Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  int i = 0;
  
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  // convert to String
  String message = String(message_buff);
  Serial.print(message);
  
  Serial.println();
  if (message.equals("up")) {
    up();
    // button was pressed, reset button
    mqttSend("Livingroom/GW60-1/Button", "Idle");
    
  } else if (message.equals("down")) {
    down();
    // button was pressed, reset button
    mqttSend("Livingroom/GW60-1/Button", "Idle");
    
  } else if (message.equals("stop")) {
    Serial.println("Stopping Shaders");
    if (dir == -1) {
      down();    
    } else if (dir == 1) {
      up();
    }
  } else if (isNumeric(message))
  {
    int tempnewpercentage = message.toInt();
    // implement deadzone to prevent problems with network/server lag
    if ( ( tempnewpercentage > String(percentage).toInt() + 5 ) || ( tempnewpercentage < String(percentage).toInt() - 5 ) )
    {
      newpercentage = tempnewpercentage;
      remote = true;
      Serial.print("Received new Percentage: ");
      Serial.println(String(newpercentage));   
    }
  }
}

// MQTT Initialize 
PubSubClient client(mqtt_server, 1883, callback, wifiClient); // 1883 is the listener port for the Broker

// MQTT Reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_clientID, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Resubscribe
      client.subscribe("Livingroom/GW60-1/Button");
      Serial.println("Subscribed: Livingroom/GW60-1/Button");
      client.subscribe("Livingroom/GW60-1/Position-Percent");
      Serial.println("Subscribed: Livingroom/GW60-1/Position-Percent");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    Serial.println("");
  }
}


// MQTT Send routine
void mqttSend(char* mqtt_topic,char* mqtt_content) {
  if (client.publish(mqtt_topic, mqtt_content)) {
    Serial.println("Message sent!");
  }
  else {
    Serial.println("Message failed to send. Reconnecting ...");
      if (client.connected()) {
        client.publish(mqtt_topic, mqtt_content);
        Serial.println("Message sent.");
      }
      else {
        reconnect();
      }
    Serial.println("");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  Serial.println("");

  SPIFFS.begin();
  // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
  //Serial.println("Please wait 30 secs for SPIFFS to be formatted");
  //SPIFFS.format();
  //Serial.println("Spiffs formatted");
  // endlines

  // load maxcount from storage
  // open file for reading
  File f = SPIFFS.open("/maxcount.txt", "r");
  if (!f) {
      Serial.println("File open failed, setting maxcount to 1");
      maxcount = 1;
  } else {
    Serial.println("Reading from SPIFFS file");
    maxcount = f.readStringUntil('\n').toInt();
    Serial.print("Loaded maxcount:");
    Serial.println(String(maxcount));
  }
  Serial.println("");

  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  // Set MQTT callback function
  client.setCallback(callback);
  
  // setup pins
  // further pins needed: sun internal activated, timer internal activated, activate sun, activate timer?
  pinMode(D0, INPUT);   // Counting reed contact
  pinMode(D5, INPUT_PULLUP);   // Save maxcount
  pinMode(D6, INPUT_PULLUP);   // Motor direction up
  pinMode(D7, INPUT_PULLUP);   // Motor direction down
  pinMode(D1, OUTPUT);  // Push button for down
  digitalWrite(D1, HIGH);// Set button high for now
  pinMode(D2, OUTPUT);  // Push button for up
  digitalWrite(D2, HIGH);// Set button high for now

  // going up to initialize at count 0
  Serial.println("Initializing - Opening completely...");
  digitalWrite(D2, LOW);
  delay(100);
  digitalWrite(D2, HIGH);  
  // wait 60 seconds to get up completely
  Serial.println("Waiting 60 seconds...");
  Serial.println("");
  delay(60000);

  // publish MQTT control topics once for remote devices to see
  if (!client.connected()) {
    reconnect();
  }
  // virtual button
  mqttSend("Livingroom/GW60-1/Button", "Idle");
  mqttSend("Livingroom/GW60-1/Position", "0");
  mqttSend("Livingroom/GW60-1/Position-Percent", "0");
  Serial.println("Published MQTT Control Topics");
  Serial.println("");
  

  Serial.println("Startup finished!");
  Serial.println("");
}

void loop() {
  // check if MQTT is connected, if not: connect
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // ***************** Current Count *******************
  if (digitalRead(D0) == LOW && digitalRead(D6) == HIGH && dir == 1 ) {
    count++;
    Serial.println("New Count: " + String(count));
    delay(100);
  } else if (digitalRead(D0) == LOW && digitalRead(D7) == HIGH && dir == -1) {
    if (count > 1)
    {
      count--;
    }
    Serial.println("New Count: " + String(count));
    delay(100);
  }

  // ****************** Set Maxcount *******************
  if (digitalRead(D5) == LOW && count != maxcount) {
    maxcount = count;
    Serial.println("New Maxcount: " + String(maxcount));
    // open file for writing
    File f = SPIFFS.open("/maxcount.txt", "w");
    if (!f) {
        Serial.println("file open failed");
    }
    Serial.println("Writing to SPIFFS file");
    f.print(String(maxcount));
    f.close();
    delay(1000);
  }
  
  // *************** Current Direction *****************
  // publish direction of travel after it changed
  if (digitalRead(D7) == LOW && dir != 1) {
    mqttSend("Livingroom/GW60-1/Direction", "Closing");
    Serial.println("New Status: Closing");
    dir = 1;
  } else if (digitalRead(D6) == LOW && dir != -1) {
    mqttSend("Livingroom/GW60-1/Direction", "Opening");
    Serial.println("New Status: Opening");
    dir = -1;
  } else if (digitalRead(D7) == HIGH && digitalRead(D6) == HIGH && dir != 0) {
    mqttSend("Livingroom/GW60-1/Direction", "Idle");
    Serial.println("New Status: Idle");
    dir = 0;
    // delay 1 s to debounce
    delay(1000);
  }

  // *************** Current Position *****************
  if (shadeposition != count) {
    char ccount[4];
    itoa( count, ccount, 10 );
    mqttSend("Livingroom/GW60-1/Position", ccount);
    Serial.println("Current Position: " + String(count));

    // calculate in percent
    percentage = count / (maxcount * 0.01);
    char cpercentage[5];
    itoa( percentage, cpercentage, 10 );
    mqttSend("Livingroom/GW60-1/Position-Percent", cpercentage);
    Serial.println("Current Position Percent: " + String(percentage));

    // store current position
    shadeposition = count;
  }
    
  // *************** New Position *****************
  // routine to check for new percentage by outside source
  // if newpercentage larger than percentage and currently stopped then start now
  if ( ( newpercentage > String(percentage).toInt() )  && dir == 0 && remote == true )
  {
    // start
    down();
    Serial.println("Newpercentage differs, going down");
    
  // if percentage larger than newpercentage and currently downwards then stop now
  } else if ( ( String(percentage).toInt() > newpercentage )  && dir == 1 && remote == true )
  {
    // stop now
    up();
    Serial.println("Newpercentage matches, stopping now");
    newpercentage = percentage;
    remote = false;
    Serial.println(remote);
    
  // if newpercentage smaller than percentage and currently stopped then start now
  } else if ( ( newpercentage < String(percentage).toInt() ) && dir == 0 && remote == true )
  {
    // start
    up();
    Serial.println("Newpercentage differs, going up");
    
  // if percentage smaller than newpercentage and currently upwards then stop now
  } else if ( ( String(percentage).toInt() < newpercentage )  && dir == -1 && remote == true )
  {
    // stop now
    down();
    Serial.println("Newpercentage matches, stopping now");
    newpercentage = percentage;
    remote = false;
    Serial.println(remote);
  }
}
