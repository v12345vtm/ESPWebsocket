/*
   ESP8266 Web server with Web Socket to control an LED.

   The web server keeps all clients' LED status up to date and any client may
   turn the LED on or off.

   For example, clientA connects and turns the LED on. This changes the word
   "LED" on the web page to the color red. When clientB connects, the word
   "LED" will be red since the server knows the LED is on.  When clientB turns
   the LED off, the word LED changes color to black on clientA and clientB web
   pages.

   References:

   https://github.com/v12345vtm/ESPWebsocket/blob/master/ESPWebsocket.ino

*/

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

String versie = "peha-feedback-input10.ino fb  shared webworkerinput.html werkt basis";
String json = "" ;
String    payloadstring;

const int LEDPIN = 14;
// Current LED status
bool LEDStatus;



const char* ssid = mySSID;
const char* password = myPASSWORD;

 


MDNSResponder mdns;

static void writeLED(bool);

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);





void updatejson()

{
  json ="";
   json += '{';
  json +=   "\"Led\":\"" ; //
  json +=  String(LEDStatus); //

  json +=   "\",\"ESPtimer\":\"" ; //
  json += "esptimerwaarder";
  json +=   "\",\"Streamport\":\"" ; //
  json += "streamportwaarde"; //runtime van de esp in seconden
 json += "\"}";
  
     Serial.print(json);
  }


static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
 <title>ESP8266 v12345vtm WebSocket 2 Javascript JSON</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
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
//document.getElementById("vt_html").innerHTML = vt_tjson.SDused + "/" + vt_tjson.SDmax + "Mb";
document.getElementById("jsonhtml").innerHTML = vt_tJSONtostring;

//document.getElementById("sdmem").value =  vt_tjson.SDused;
//document.getElementById("sdmem").max =  vt_tjson.SDmax;
//document.getElementById("TLint").value =  vt_tjson.Timelapseinterval;
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
  
     var bdknop = document.getElementById('Bd');
    if (vt_tjson.TLrunning === "1") {bdknop.style.color = 'red';bdknop.innerHTML = "stop TL";}
    else 
  if (vt_tjson.TLrunning === "0") {bdknop.style.color = 'green';bdknop.innerHTML = "start TL";}
   
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
       // webSocket.sendTXT(num, "hello from arduino sketch");
       updatejson(); 
        webSocket.sendTXT(num,  json );//send 1e json to connected client
      }
      break;
    case WStype_TEXT:
      { //jump to case label [-fpermissive]
        Serial.printf("[%u] get Text: %s\r\n", num, payload);
        for (int i = 0; i < length; i++) {
          // string achter pointer to string doen
        
       payloadstring = payloadstring + (char)payload[i];
        }
        int pos = payloadstring.indexOf(':');
    String   plkey  =  payloadstring.substring(0, pos);
    String    plvalue = payloadstring.substring(pos + 1);

     Serial.print("browser send:");//start stream
      Serial.println(plkey);//start stream

  Serial.print("and:");//start stream
      Serial.println(plvalue);//start stream

if (plvalue.equals("off") ) {
        //  TLinterval =  plvalue.toInt();//TLint is een id van de htlm   ,  de socket stuurde TLint:60  => save to variables n esp32
         // Serial.println("tlint met value gekozen:"  + plvalue);//timelapse
            Serial.println("ledonpl dedrukt");//start stream
          writeLED(false);
        }
        
   else if (plvalue.equals("on") ) {
        //  TLinterval =  plvalue.toInt();//TLint is een id van de htlm   ,  de socket stuurde TLint:60  => save to variables n esp32
         // Serial.println("tlint met value gekozen:"  + plvalue);//timelapse
            Serial.println("ledonpl dedrukt");//start stream
          writeLED(true);
        }
      
      else  if (strcmp("Ba", (const char *)payload) == 0) {
          Serial.println("Ba knop dedrukt");//get still
        }
        else if (strcmp("Bb", (const char *)payload) == 0) {
          Serial.println("Bb knop dedrukt");//start stream
        }
        else if (strcmp("Bc", (const char *)payload) == 0) {
          Serial.println("Bc knop dedrukt");//save2imagesd
         
        }

        else if (strcmp("Bd", (const char *)payload) == 0) {
          Serial.println("Bd knop dedrukt create timer");//timelapse starten , creat timer
          // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/esp_timer.html
          // voor ons gemak resetten we de esp timer ??
         // toggleTimelapse(); //toggle de timer die de timelapse doet
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
       updatejson();
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
  // Note inverted logic for Adafruit HUZZAH board
  if (LEDon) {
    digitalWrite(LEDPIN, 0);
  }
  else {
    digitalWrite(LEDPIN, 1);
  }
}//oud







void setup()
{


 


  pinMode(LEDPIN, OUTPUT);
  writeLED(false);


  updatejson();

  Serial.begin(115200);

  //Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();
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
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  server.handleClient();
}
