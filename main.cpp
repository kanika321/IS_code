#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Servo.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define DHTPIN D4
#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);


#define BUZZER_PIN D6  // Buzzer connected to D6 (GPIO12)
#define SERVO_PIN D5   // Servo connected to D3 (GPIO0)
#define LED_PIN D3 
#define LDR_PIN A0
#define CLOSE_CURTAIN 0
#define OPEN_CURTAIN 1


Servo myservo;


const char* ssid = "Hussain's OnePlus";
const char* password = "whysoserious";


ESP8266WebServer server(80);


float optimalTemp;
float criticalTemp;
float optimalHumidity;
float criticalHumidity;

bool isSystemActive = true; // Variable to track the system state
bool criticalCondition = false;
bool curtain_open = false;

void connect_wifi();
void initialize_components();
void initialize_oled_display();
void handleRoot();
void handleSetParams();
void handleSaveParams();
void handleStartSystem();
void handleStopSystem();
void handleNotFound();
void saveParams();
void loadParams();
void oled_settings();
void update_oled_display(float temperature, float humidity);
void LDR_read();
void manage_curtain(bool action_type);


void setup() {
   Serial.begin(115200);
   EEPROM.begin(512); // Initialize EEPROM
   loadParams(); // Load parameters from EEPROM
   initialize_components(); // Initialize components
   connect_wifi(); // Connect to Wi-Fi

   // Set up web server routes
   server.on("/", HTTP_GET, handleRoot);
   server.on("/setParams", HTTP_GET, handleSetParams);
   server.on("/saveParams", HTTP_POST, handleSaveParams);
   server.on("/startSystem", HTTP_GET, handleStartSystem);
   server.on("/stopSystem", HTTP_GET, handleStopSystem);
   server.onNotFound(handleNotFound);

   server.begin();
   Serial.println("HTTP server started");
}


void loop() {
   server.handleClient();
 
  if (isSystemActive) 
  {
       // Read sensor data
       sensors_event_t event;
       dht.temperature().getEvent(&event);
       float temperature = event.temperature;
       dht.humidity().getEvent(&event);
       float humidity = event.relative_humidity;

       // Check for critical conditions
       if (temperature >= criticalTemp || humidity >= criticalHumidity) 
       {
            //digitalWrite(BUZZER_PIN, HIGH);
            // Update display with critical condition message
            oled_settings();
            if(temperature > criticalTemp)
            {
                display.println(F("Critical temperature!"));
            }
            else
            {
                display.println(F("Critical humidity!"));
            }
            display.display();
            criticalCondition = true;
            //curtains
            manage_curtain(OPEN_CURTAIN);

            // myservo.write(90); // Example action

        } 
        //Check for optimal conditions
        else if(temperature >= optimalTemp && temperature < criticalTemp || humidity >= optimalHumidity && humidity < criticalHumidity)
        {
            //Open window here
            //Delete below sample code later#
            digitalWrite(BUZZER_PIN, LOW);
            //myservo.write(0); // Example action
            oled_settings();
            update_oled_display(temperature, humidity);
            // Reset critical condition flag and curtain open flag
            criticalCondition = false;
        }
        //Check for normal conditions
        else
        {
            digitalWrite(BUZZER_PIN, LOW);
            //myservo.write(0); // Example action
            oled_settings();
            update_oled_display(temperature, humidity);
            // Reset critical condition flag and curtain open flag
            criticalCondition = false;
        }

        delay(100);

        // LDR read 
        LDR_read();

   }
}

void connect_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void initialize_components() {
    initialize_oled_display(); // Initialize OLED display
    dht.begin(); // Initialize DHT sensor
    // Initialize servo
    myservo.attach(SERVO_PIN);
    myservo.write(0); // Initial position
    // Initialize buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void initialize_oled_display() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Corrected I2C address
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Don't proceed, loop forever
    }
    display.display();
    delay(2000);
    display.clearDisplay();
}

void oled_settings()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
}

void update_oled_display(float temperature, float humidity) {
    display.print(F("Temperature: "));
    display.print(temperature);
    display.println(F(" *C"));
    display.print(F("Humidity: "));
    display.print(humidity);
    display.println(F(" %"));
    display.println(F(""));
    display.println(F("Preferences"));
    display.print(F("Opt Temp: "));
    display.print(optimalTemp);
    display.println(F(" *C"));
    display.print(F("Opt Hum: "));
    display.print(optimalHumidity);
    display.println(F(" %"));
    display.print(F("Crit Temp: "));
    display.print(criticalTemp);
    display.println(F(" *C"));
    display.print(F("Crit Hum: "));
    display.print(criticalHumidity);
    display.println(F(" %"));
    display.display();    
}

void LDR_read() {
        int value = analogRead(LDR_PIN);

        //To print LDR analog values uncomment below 2 lines
        //Serial.print("LDR Value : ");
        //Serial.println(value);
        delay(1000);

        //For given resitance in room light condition LDR value is in the range of 35 - 50 and for dark conditons it's in the range of 2-7
        if(value > 30)
        {
                //Lights OFF
                digitalWrite(LED_PIN, LOW);
                if(criticalCondition == false)
                {
                    manage_curtain(OPEN_CURTAIN);
                }
                else
                {
                    //Do nothing as Critical functionalities are active
                }
                
        }
        else
        {

                //Lights ON
                digitalWrite(LED_PIN, HIGH);
                if(criticalCondition == false)
                {
                    manage_curtain(CLOSE_CURTAIN);
                }
                else
                {
                    //Do nothing as Critical functionalities are active
                }
        }
}

void manage_curtain(bool action_type) {
            int pos;
            //Serial.print("Action Type : ");
            //Serial.println(action_type);
            //Serial.print("Curtain Status : ");
            //Serial.println(curtain_open);
            if(curtain_open == false && action_type == OPEN_CURTAIN)
            {
                for(pos = 0; pos <= 180; pos+=1)
                {
                        myservo.write(pos);
                        delay(50);
                }
                curtain_open = true;
            }
            else if(curtain_open == true && action_type == CLOSE_CURTAIN)
            {
                for(pos = 180; pos >= 0; pos-=1)
                {
                        myservo.write(pos);
                        delay(50);
                }
                curtain_open = false;
            }
            else
            {
                //Do nothing as this covers all other redundant combinations
            }
    
}


void handleRoot() {
   String html = "<!DOCTYPE html>";
   html += "<html>";
   html += "<head>";
   html += "<title>Smart Thermostat</title>";
   html += "<style>";
   html += "body {font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f9;}";
   html += "header {background-color: #4CAF50; color: white; padding: 15px; text-align: center;}";
   html += "footer {background-color: #4CAF50; color: white; padding: 10px; text-align: center; position: fixed; bottom: 0; width: 100%;}";
   html += ".container {padding: 20px;}";
   html += "h1 {color: #333;}";
   html += ".menu {overflow: hidden; background-color: #333;}";
   html += ".menu a {float: left; display: block; color: white; text-align: center; padding: 14px 16px; text-decoration: none;}";
   html += ".menu a:hover {background-color: #ddd; color: black;}";
   html += "</style>";
   html += "</head>";
   html += "<body>";
   html += "<header>";
   html += "<h1>Smart Thermostat</h1>";
   html += "</header>";
   html += "<div class=\"menu\">";
   html += "<a href=\"/\">Home</a>";
   html += "<a href=\"/setParams\">Set preferences</a>";
   html += "</div>";
   html += "<div class=\"container\">";
   html += "<h1>Current Conditions</h1>";
   html += "<p>Temperature: ";
  
   sensors_event_t event;
   dht.temperature().getEvent(&event);
   html += String(event.temperature); // Corrected
   html += " *C</p>";
  
   dht.humidity().getEvent(&event);
   html += "<p>Humidity: ";
   html += String(event.relative_humidity); // Corrected
   html += " %</p>";
  
   html += "<h1>Current Preferences</h1>";
   html += "<p>Optimal Temperature: " + String(optimalTemp) + " *C</p>";
   html += "<p>Optimal Humidity: " + String(optimalHumidity) + " %</p>";
   html += "<p>Critical Temperature: " + String(criticalTemp) + " *C</p>";
   html += "<p>Critical Humidity: " + String(criticalHumidity) + " %</p>";
   html += "</div>";


   html += "<style>";
   html += ".btn {";
   html += "  display: inline-block;";
   html += "  padding: 10px 20px;";
   html += "  font-size: 16px;";
   html += "  cursor: pointer;";
   html += "  border: none;";
   html += "  border-radius: 5px;";
   html += "}";
   html += ".btn-start {";
   html += "  background-color: #4CAF50;";
   html += "  color: white;";
   html += "}";
   html += ".btn-stop {";
   html += "  background-color: #f44336;";
   html += "  color: white;";
   html += "}";
   html += "</style>";


   html += "<form action=\"/startSystem\" method=\"GET\">";
   html += "<button class=\"btn btn-start\">Start System</button>";
   html += "</form>";
   html += "<form action=\"/stopSystem\" method=\"GET\">";
   html += "<button class=\"btn btn-stop\">Stop System</button>";
   html += "</form>";


   html += "<footer>";
   html += "<p>&copy; 2024  Smart Thermostat</p>";
   html += "</footer>";
   html += "</body>";
   html += "</html>";
   server.send(200, "text/html", html);
}


void handleSetParams() {
   String html = "<!DOCTYPE html>";
   html += "<html>";
   html += "<head>";
   html += "<title>Smart Thermostat - Set Preferences</title>";
   html += "<style>";
   html += "body {font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f9;}";
   html += "header {background-color: #4CAF50; color: white; padding: 15px; text-align: center;}";
   html += "footer {background-color: #4CAF50; color: white; padding: 10px; text-align: center; position: fixed; bottom: 0; width: 100%;}";
   html += ".container {padding: 20px;}";
   html += "h1 {color: #333;}";
   html += ".menu {overflow: hidden; background-color: #333;}";
   html += ".menu a {float: left; display: block; color: white; text-align: center; padding: 14px 16px; text-decoration: none;}";
   html += ".menu a:hover {background-color: #ddd; color: black;}";
   html += "input[type=text] {width: 100%; padding: 12px 20px; margin: 8px 0; box-sizing: border-box;}";
   html += "input[type=submit] {background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; cursor: pointer; width: 100%;}";
   html += "input[type=submit]:hover {background-color: #45a049;}";
   html += "</style>";
   html += "</head>";
   html += "<body>";
   html += "<header>";
   html += "<h1>Smart Thermostat</h1>";
   html += "</header>";
   html += "<div class=\"menu\">";
   html += "<a href=\"/\">Home</a>";
   html += "<a href=\"/setParams\">Set preferenes</a>";
   html += "</div>";
   html += "<div class=\"container\">";
   html += "<h1>Set Preferences</h1>";
   html += "<form action=\"/saveParams\" method=\"POST\">";
   html += "Optimal Temperature: <input type=\"text\" name=\"optTemp\" value=\"" + String(optimalTemp) + "\"><br>";
   html += "Optimal Humidity: <input type=\"text\" name=\"optHumidity\" value=\"" + String(optimalHumidity) + "\"><br>";
   html += "Critical Temperature: <input type=\"text\" name=\"critTemp\" value=\"" + String(criticalTemp) + "\"><br>";
   html += "Critical Humidity: <input type=\"text\" name=\"critHumidity\" value=\"" + String(criticalHumidity) + "\"><br>";
   html += "<input type=\"submit\" value=\"Set Preferences\">";
   html += "</form>";
   html += "</div>";
   html += "<footer>";
   html += "<p>&copy; 2024 Smart Thermostat</p>";
   html += "</footer>";
   html += "</body>";
   html += "</html>";
   server.send(200, "text/html", html);
}


void handleSaveParams() {
   if (server.hasArg("optTemp")) {
       optimalTemp = server.arg("optTemp").toFloat();
   }
   if (server.hasArg("critTemp")) {
       criticalTemp = server.arg("critTemp").toFloat();
   }
   if (server.hasArg("optHumidity")) {
       optimalHumidity = server.arg("optHumidity").toFloat();
   }
   if (server.hasArg("critHumidity")) {
       criticalHumidity = server.arg("critHumidity").toFloat();
   }
   saveParams();
   server.send(200, "text/html", "<h1>Preferences Saved</h1><p><a href=\"/\">Go back to Home</a></p>");
}


void handleNotFound() {
   server.send(404, "text/plain", "Not Found");
}


void saveParams() {
   EEPROM.put(0, optimalTemp);
   EEPROM.put(sizeof(float), criticalTemp);
   EEPROM.put(sizeof(float) * 2, optimalHumidity);
   EEPROM.put(sizeof(float) * 3, criticalHumidity);
   EEPROM.commit();
}


void loadParams() {
   EEPROM.get(0, optimalTemp);
   EEPROM.get(sizeof(float), criticalTemp);
   EEPROM.get(sizeof(float) * 2, optimalHumidity);
   EEPROM.get(sizeof(float) * 3, criticalHumidity);
}


bool systemActive = false;


void handleStartSystem() {
   isSystemActive = true;
   String html = "<!DOCTYPE html>";
   html += "<html>";
   html += "<head>";
   html += "<title>System Control - Start</title>";
   html += "<style>";
   html += "body {font-family: Arial, sans-serif; background-color: #f4f4f9;}";
   html += "h1 {color: #333; text-align: center;}";
   html += "p {text-align: center;}";
   html += ".btn {display: block; width: 200px; margin: 10px auto; padding: 10px; font-size: 16px; color: #fff; background-color: #4CAF50; border: none; border-radius: 5px; text-align: center; text-decoration: none;}";
   html += "</style>";
   html += "</head>";
   html += "<body>";
   html += "<h1>System Started</h1>";
   html += "<p>The system is now active.</p>";
   html += "<a href=\"/\" class=\"btn\">Go Back</a>";
   html += "</body>";
   html += "</html>";
   server.send(200, "text/html", html);


   // Reinitialize components if needed
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   display.display();
   delay(2000);
   display.clearDisplay();
   dht.begin();
}


void handleStopSystem() {
   isSystemActive = false;


   // Turn off display, sensors, etc.
   display.clearDisplay();
   display.display();  // This turns off the display


   digitalWrite(BUZZER_PIN, LOW);  // Ensure buzzer is turned off
   myservo.write(90);  // Move servo to initial position (e.g., close curtains)


   String html = "<!DOCTYPE html>";
   html += "<html>";
   html += "<head>";
   html += "<title>System Control - Stop</title>";
   html += "<style>";
   html += "body {font-family: Arial, sans-serif; background-color: #f4f4f9;}";
   html += "h1 {color: #333; text-align: center;}";
   html += "p {text-align: center;}";
   html += ".btn {display: block; width: 200px; margin: 10px auto; padding: 10px; font-size: 16px; color: #fff; background-color: #FF0000; border: none; border-radius: 5px; text-align: center; text-decoration: none;}";
   html += "</style>";
   html += "</head>";
   html += "<body>";
   html += "<h1>System Stopped</h1>";
   html += "<p>The system is now inactive.</p>";
   html += "<a href=\"/\" class=\"btn\">Go Back</a>";
   html += "</body>";
   html += "</html>";
   server.send(200, "text/html", html);


   // Put the ESP8266 into deep sleep mode
   //ESP.deepSleep(0);  // The ESP8266 will remain in deep sleep until it is reset
}

