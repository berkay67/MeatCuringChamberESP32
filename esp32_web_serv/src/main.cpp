/* 
 * Project : Meat Curing Chamber Controller  
 * Author : Lomas Subedi 
 * Controller : ESP WROOM - 32
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include "SD.h"

#include "DHT.h"

#define     DHTPIN          4     // what digital pin we're connected to
#define     BUF_SZ          512
#define     DHTTYPE         DHT11   // DHT 11


char printBuffer[BUF_SZ];

const char* ssid = "internets";
const char* password = "CLFA4ABD38";

// const char* ssid = "yangobahal";
// const char* password = "43A74C699A";

const int freezer = 2;
bool freezerStatus = false;

// Set web server port number to 80
WiFiServer server(80);

// Web page file stored on the SD card
File webFile; 

// Variable to store the HTTP request
String httpReq;       // buffered HTTP request stored as null terminated string
char reqIndex = 0;              // index into HTTP_req buffer

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

void setNewControls() {
  if (httpReq.indexOf("freezer=1") >= 0) {
    freezerStatus = true;
    digitalWrite(freezer, HIGH);
  } else if(httpReq.indexOf("freezer=0") >= 0) {
    freezerStatus = false;
    digitalWrite(freezer, LOW);
  }

}
// Send XML file with sensor readings
void sendXMLFile(WiFiClient cl) {

  cl.print("<?xml version = \"1.0\" ?>");
  cl.print("<output>");    

  // Check Box Device status
  // For Lamp 1
  cl.print("<freezer>");

  if (freezerStatus) {
    cl.print("checked");
  } else {
    cl.print("unchecked");
  }

  cl.println("</freezer>");

  cl.print("</output>");

}

void setup() {

  Serial.begin(115200);

  // Initialize the sensor
  dht.begin();

  // Setup SD card
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  } else {
    Serial.println("SD card successfully mounted !!!");
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

  WiFi.begin(ssid, password);
  Serial.println("");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  pinMode(freezer, OUTPUT);
  digitalWrite(freezer, LOW);
}

void loop() {
   WiFiClient client = server.available();   // Listen for incoming clients
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    sprintf(printBuffer, "Humidity -> %f\tTemperature ->%f*C\t%f*f\r\n", h, t, f);
    Serial.print(printBuffer);
  }


  if (client) {  // if new client connects
    boolean currentLineIsBlank = true;
    reqIndex = 0;
    while (client.connected()) {      
      if (client.available()) {   // client data available to read
        char c = client.read(); // read 1 byte (character) from client
        httpReq += c;
        //Serial.write(c);    
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (c == '\n' && currentLineIsBlank) {

          // send a standard http response header
          client.println("HTTP/1.1 200 OK");

          Serial.println(httpReq);

          // Send XML file or Web page
          // If client already on the web page, browser requests with AJAX the latest
          // sensor readings (ESP32 sends the XML file)
          if (httpReq.indexOf("updateData") >= 0) {
            Serial.println("I am here !!");
            // send rest of HTTP header
            client.println("Content-Type: text/xml");
            client.println("Connection: keep-alive");
            client.println();
            
            setNewControls();
            // Send XML file with sensor readings
            sendXMLFile(client);
          }

          // When the client connects for the first time, send it the index.html file
          // stored in the microSD card
          else {  
            // send rest of HTTP header
            client.println("Content-Type: text/html");
            client.println("Connection: keep-alive");
            client.println();

            // send web page stored in microSD card
            webFile = SD.open("/dashboard.html");
            if (webFile) {
              while(webFile.available()) {
                // send web page to client
                client.write(webFile.read()); 
              }
              webFile.close();
            }
          }
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
        }
      } // end if (client.available())
      // Serial.println(header);
    } // end while (client.connected())
    // Clear the header variable
    httpReq = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");    
  } // end if (client)
}   