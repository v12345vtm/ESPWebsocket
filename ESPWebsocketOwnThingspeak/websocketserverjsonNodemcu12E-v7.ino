/*
   ESP8266 Web server with Web Socket to control an LED.

   The web server keeps all clients' LED status up to date and any client may
   turn the LED on or off.

   For example, clientA connects and turns the LED on. This changes the word
   "LED" on the web page to the color red. When clientB connects, the word
   "LED" will be red since the server knows the LED is on.  When clientB turns
   the LED off, the word LED changes color to black on clientA and clientB web
   pages.
   All data via websocket is interpret in the browser as JSON format.

   mini thingspeak server  : eg :http://192.168.1.48/ownthingspeak?var=stroomverbruik&val=15 stores values that you send to a paga
   

  A ridiculous amount of coffee was consumed in the process of building this project. Add some fuel if you'd like to keep me going!
  if you the video helped you , buy me a coffee :) https://www.paypal.me/v12345vtm/3


 References:
   https://github.com/v12345vtm/ESPWebsocket/tree/master/ESPWebsocketOwnThingspeak

   testserver  : http://192.168.1.48/

*/

#include <FS.h>   // Include the SPIFFS library
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <credentials.h> // C:\Users\vith\Documents\Arduino\libraries\Credentials\credentials.h

#define D0   16 //input  via knop
#define D1    5// browser test led Ajax ledon ledoff
#define D2   4//LAN verbonde,
#define D3   0//ACCES POINT  er is 1 client verbonden 
#define D4   2 // COMPORT LOGGER UART2 ESP12e txt , naar RX (witte draad van de pl2302 usb/ttl interface)
#define D5   14 // led&buzzer on when communicating with connection to peha telent sessie open
#define D6   12 //timout of host niet gevonden in telnet 4sec geen gebabbel gehad met peha
#define D7   13//een socketverbinding met een browser is aktief 
#define D8   15
#define D9   3 //RX
#define D10    1 //TX
#define D11    9 //S3 noemt da op de pcb
#define D12    10//S2 op de pcb

String versie = "websocketserverjsonNodemcu12E-v6";
String json = "" ;
String    payloadstring;
String getContentType(String filename); // convert the file extension to the MIME type
const int LEDPIN = 14;
bool LEDStatus;
const char* ssid = mySSID;
const char* password = myPASSWORD;
MDNSResponder mdns;
static void writeLED(bool);
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void writeThingspeakDotJSON(String data) {
  updateStatusdotJsonVariable();
  // this opens the file   in read-mode
  File f = SPIFFS.open("/thingspeak.json", "r");
  if (!f) {
    Serial.println("File doesn't exist yet. Creating it");
    // open the file in write mode
    File f = SPIFFS.open("/thingspeak.json", "w");
    if (!f) {
      Serial.println("file thingspeak.json creation failed");
    }
    // now write  key/value style with json characters
    f.println(data);
    Serial.println("saved thingspeak.json" + data);
  }
  else
  { //update append  file in write mode
    File f = SPIFFS.open("/thingspeak.json", "a");
    f.println(data);
    Serial.println("upd thingspeak.json" + data);
  }
f.close();
}
///////////////////////////

/////////////////////////////////


void writeStatusDotJSON() {
  updateStatusdotJsonVariable();
  // this opens the file   in read-mode
  File f = SPIFFS.open("/status.json", "r");
  if (!f) {
    Serial.println("File doesn't exist yet. Creating it");
    // open the file in write mode
    File f = SPIFFS.open("/status.json", "w");
    if (!f) {
      Serial.println("file creation failed");
    }
    // now write  key/value style with json characters
    f.println(json);
    Serial.println("saved json" + json);
  }
  else
  { //update file in write mode
    File f = SPIFFS.open("/status.json", "w");
    f.println(json);
    Serial.println("upd status.json" + json);
  }
f.close();
}
///////////////////////////
void loadstatus()
 {    // this opens the file  in read-mode
  File f = SPIFFS.open("/status.json", "r");
 if (!f) {
    Serial.println("\n \n\nFile status.json; doesn't exist yet");
  }
  else 
  {
    //file exists we could open the file and read it
    while (f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('n');
      Serial.println("\n\n\n uit memory gehaald: ");
        Serial.println(line);
        f.close();
    }    
    }
  }

void updateStatusdotJsonVariable()
{
  json = "";
  json += '{';
  json +=   "\"Led\":\"" ; //
  json +=  String(LEDStatus); //
  json +=   "\",\"ESPtimer\":\"" ; //
  json += millis();
  json +=   "\",\"versie\":\"" ; //
  json += versie; //runtime van de esp in seconden
  json += "\"}";
  // Serial.print(json);
}


static const char PROGMEM THINGSPEAK_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
 <title>thingspeak v12345vtm</title>
<style>
body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }
</style>
 <script>
var websock;
var vt_tjson;

function wsjsonrecieved(evt)
{
 var vt_t = evt.data;
 try {
   vt_tjson = JSON.parse(vt_t);
  console.log(vt_tjson);
console.log("hierboven?");
  } catch (e) {
    // error  
    // console.log('geen json format gezien');
    return;
  }
var vt_tJSONtostring = JSON.stringify(vt_tjson); //js tostring
document.getElementById("jsonhtml").innerHTML = vt_tJSONtostring;

  var keys = Object.keys(vt_tjson);
  var waardes = Object.values(vt_tjson);
  var txt = "";
 txt += "<table border='1'>"
  for (var key in keys ) {
   txt += "<tr><td>" +  keys[key] + "</td><td>" +  waardes[key] + "</td></tr>";
      //  console.log(key + " -> " + keys[key]  + "-" + waardes[key]);
    }
    txt += "</table>" 
   document.getElementById("tabel").innerHTML = txt;
    
   var e = document.getElementById('ledstatus');
    if (vt_tjson.Led === "1") {e.style.color = 'red';}
    else 
 if (vt_tjson.Led === "0") {e.style.color = 'black';}
  }

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
 // websock = new WebSocket('ws://192.168.1.48:81/');
  websock.onopen = function(evt) { 
  console.log('websock open'); 
   wsjsonrecieved(evt);//parse json to html  
  };
  
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (true) {
      console.log('socket from esp recieved');
      //console.log(evt);
 wsjsonrecieved(evt);//parse updated json to html
    }
  };
}

function buttonclick(e) {
 // console.log ("export buttonklik js");
  websock.send(  e.id);
 console.log (e.id);
}

</script>
</head>
<body onload="javascript:start();">
<h1>own thingspeak </h1>

<p>, usage : http://192.168.1.48/ownthingspeak.html?var=stroomverbruik&val=15</p>

<a href="/ownthingspeak.html?var=stroomverbruik&val=15">send test data to OwnThingspeak : stroomverbruik :  15</a><br>

the server stores the arguments from the url to SPiffs or SD card  eg. : stroomverbruik and 15
<p id="jsonhtml"></p>
 

<a href="/thingspeak.json">view onw thingspeak.json</a><br>
<a href="https://github.com/v12345vtm/ESPWebsocket">GIT-sourcecode templates</a>

 <script src="https://apis.google.com/js/platform.js"></script>
<div class="g-ytsubscribe" data-channel="v12345vtm" data-layout="default" data-count="default"></div>

</body>
</html>
)rawliteral";



////////////////////////////////////////////////////////////////////////////////////////////////////




static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
 <title>ESP8266 v12345vtm WebSocket 2 Javascript JSON</title>
<style>
body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }
</style>
 <script>
var websock;
var vt_tjson;

function wsjsonrecieved(evt)
{
 var vt_t = evt.data;
 try {
   vt_tjson = JSON.parse(vt_t);
  console.log(vt_tjson);
console.log("hierboven?");
  } catch (e) {
    // error  
    // console.log('geen json format gezien');
    return;
  }
var vt_tJSONtostring = JSON.stringify(vt_tjson); //js tostring
document.getElementById("jsonhtml").innerHTML = vt_tJSONtostring;

  var keys = Object.keys(vt_tjson);
  var waardes = Object.values(vt_tjson);
  var txt = "";
 txt += "<table border='1'>"
  for (var key in keys ) {
   txt += "<tr><td>" +  keys[key] + "</td><td>" +  waardes[key] + "</td></tr>";
      //  console.log(key + " -> " + keys[key]  + "-" + waardes[key]);
    }
    txt += "</table>" 
   document.getElementById("tabel").innerHTML = txt;
    
   var e = document.getElementById('ledstatus');
    if (vt_tjson.Led === "1") {e.style.color = 'red';}
    else 
 if (vt_tjson.Led === "0") {e.style.color = 'black';}
  }

function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
 // websock = new WebSocket('ws://192.168.1.48:81/');
  websock.onopen = function(evt) { 
  console.log('websock open'); 
   wsjsonrecieved(evt);//parse json to html  
  };
  
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (true) {
      console.log('socket from esp recieved');
      //console.log(evt);
 wsjsonrecieved(evt);//parse updated json to html
    }
  };
}

function buttonclick(e) {
 // console.log ("export buttonklik js");
  websock.send(  e.id);
 console.log (e.id);
}

</script>
</head>
<body onload="javascript:start();">
<h1>ESP8266 WebSocket to Javascript JSON</h1>
<p id="jsonhtml"></p>
<br> C:\Users\vith\Documents\Arduino\libraries\Credentials\credentials.h
<hr>
<div id="ledstatus"><b>LED</b></div>
<button id="Led:on"  type="button" onclick="buttonclick(this);">On</button> 
<button id="Led:off" type="button" onclick="buttonclick(this);">Off</button>
 <p  class="absolute" id="tabel"></p> 

<a href="/status.json">ESP-status</a><br>
<a href="/ownthingspeak.html">Local and own thingspeak service</a><br>
<a href="https://github.com/v12345vtm/ESPWebsocket">GIT-sourcecode templates</a>

 <script src="https://apis.google.com/js/platform.js"></script>
<div class="g-ytsubscribe" data-channel="v12345vtm" data-layout="default" data-count="default"></div>

</body>
</html>
)rawliteral";

/////////////////////websocket template
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
           updateStatusdotJsonVariable(); 
        webSocket.sendTXT(num,  json );//send 1e json to connected client
      }
      break;
    case WStype_TEXT:
      { //jump to case label [-fpermissive]
        Serial.printf("[%u] get Text: %s\r\n", num, payload);
        for (int i = 0; i < length; i++) {
              payloadstring = payloadstring + (char)payload[i];
        }
        int pos = payloadstring.indexOf(':');
    String   plkey  =  payloadstring.substring(0, pos);
    String    plvalue = payloadstring.substring(pos + 1);

     Serial.print("browser socket sended: ");//start stream
      Serial.print (plkey);//start stream
  Serial.print("and:");//start stream
      Serial.println(plvalue);//start stream

if (plvalue.equals("off") ) {
 Serial.println("ledonpl dedrukt");//start stream
          writeLED(false);
           writeStatusDotJSON();
        }
        
   else if (plvalue.equals("on") ) {
Serial.println("ledonpl dedrukt +writeStatusDotJSON");//start stream
          writeLED(true);
          //updateStatusdotJsonVariable(); 
          writeStatusDotJSON();
        }
      
      else  if (strcmp("Ba", (const char *)payload) == 0) {
          Serial.println("Ba knop dedrukt");//get still
        }
   

        else if (plkey.equals("TLint") ) {
        //  TLinterval =  plvalue.toInt();//TLint is een id van de htlm   ,  de socket stuurde TLint:60  => save to variables n esp32
         // Serial.println("tlint met value gekozen:"  + plvalue);//timelapse
        }

        else {
          Serial.println("Unknown command");
        }

        payloadstring = "";
        json = "";
       updateStatusdotJsonVariable();
        webSocket.sendTXT(num,  json );//send updated  json to connected client

        // send data to all connected clients
         char vithchar[100];
       json.toCharArray(vithchar, json.length() + 1);
        Serial.println(json.length() + 1); //126
        Serial.println("vithlengte letop 100bytes lengte voorzien");
        webSocket.broadcastTXT(vithchar , json.length());
        break;
      }      //jump to case label [-fpermissive]

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;

    default:
      Serial.printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}
////////////////////einde websockettemplate

void handleRoot()
{
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

static void writeLED(bool LEDon)
{
  LEDStatus = LEDon;
  if (LEDon) {
    digitalWrite(LEDPIN, 0);
  }
  else {
    digitalWrite(LEDPIN, 1);
  }
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".png")) return "image/x-icon";
  else if (filename.endsWith(".jpg")) return "image/x-icon";
    else if (filename.endsWith(".json")) return "application/json"; // static char json_response[1024];
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  // Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}


String bestandsysteem()
{
  /////check filesystem
  String filesysteem = "<ul>";
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    filesysteem += "<li>";
    filesysteem += dir.fileName();
    filesysteem += " ";
    filesysteem += dir.fileSize();
    filesysteem += "kb</li>";
  }
  filesysteem += "<ul>";
  /////interne webpaginas
  filesysteem += "<ul>";

  // filesysteem += "<li>/ als lan device met ipadres en versie als melding</li>";
  filesysteem += "<li>/ root</li>";
  filesysteem += "<ul>";
  Serial.println(filesysteem);
  return filesysteem;
}

//////

void setup()
{
 Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  writeLED(false);
 SPIFFS.begin();                           // Start the SPI Flash Files System
loadstatus();// haal de json file uit , als die bestaat !!!!
   updateStatusdotJsonVariable();
  bestandsysteem();
   Serial.print(json);

  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("espWebSock", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.print("Connect to http://espWebSock.local or http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

 
  server.on("/ownthingspeak.html", []() {
    ////action_input?stm=0&module=2&kanaalnr=8&gedrag=2
    //http://192.168.1.48/ownthingspeak.html?var=stroomverbruik&val=15
    //    http://192.168.1.48/ownthingspeak.html
    //functie die site kan inlezen en de argumenten in de url , we belanden hier door als de gebruiker input.html de form verzend
    //opvangen form argumenten vann 
    String  XLMinputcommand = ""; 
    XLMinputcommand +=  (server.arg("var") + " ");
    XLMinputcommand +=  (server.arg("val") + " ");
    Serial.println(XLMinputcommand);

    writeThingspeakDotJSON (XLMinputcommand);
      // handleFileRead("/");//geef de originele input.html terug
server.send_P(200, "text/html", THINGSPEAK_HTML);
       //THINGSPEAK_HTML
  //  server.send(200, "text/plain", "savedownthingspeak\n\n");// if the page not exist or is not in spiffs , send file not found
  });


   
  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "File Not Found\n\n");// if the page not exist or is not in spiffs , send file not found
  });  

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  server.handleClient();
}
