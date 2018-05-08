#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT11.h>
#include <Thread.h>
#include <ThreadController.h>

#define dht D5

const char* ssid = "Guille";
const char* password = "0000002345";
const char* host = "ezequielest.com";

String ip = "";

DHT11 dht11(dht);

ThreadController controll = ThreadController();
Thread* myThread = new Thread();

ESP8266WebServer server(80);

#define luz D1
#define led D2

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
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
  digitalWrite(led, 0);
}

void setup(void){
  
  pinMode(led, OUTPUT);
  pinMode(luz, OUTPUT);
  digitalWrite(led, 0);
  digitalWrite(luz, 0);
  Serial.begin(115200);

  myThread->onRun(enviarInfoDHT);
  myThread->setInterval(10000);

  controll.add(myThread);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  String ip = WiFi.localIP().toString();
  Serial.println("wifi string " + ip);

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/apagarLed", [](){
    int numLed = server.arg("led").toInt();
    digitalWrite(numLed, HIGH);
    server.send(200, "text/plain", "led apagado desde esp");
  });

  server.on("/encenderLed", [](){
    int numLed = server.arg("led").toInt();
    digitalWrite(numLed, LOW);
    server.send(200, "text/plain", "led encendido desde esp");
  });

  server.on("/encenderLuz", [](){
    int numLuz = server.arg("luz").toInt();
    digitalWrite(numLuz, HIGH);
    server.send(200, "text/plain", "luz encendida desde esp");
  });

  server.on("/apagarLuz", [](){
    int numLuz = server.arg("luz").toInt();
    digitalWrite(numLuz, LOW);
    server.send(200, "text/plain", "luz apagada desde esp");
  });

  server.on("/obtenerTemHum", [](){
    
    int err;
    float temp, hum;
    if ( (err = dht11.read(hum,temp)) == 0 )  
    {
        Serial.print("Temperatura : ");
        Serial.println(temp);
        Serial.print("Humedad: ");
        Serial.println(hum);
      
      // Creo los datos a enviar
      String data = "{'temp':" + (String)temp + ",'hum':" + (String)hum +"}";
      //application/x-www-form-urlencoded
    server.send(200, "application/json", data);

    }
  });


  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
  controll.run();
} 


void enviarInfoDHT(){
  Serial.println("Enviando data a webservice");
  
  // Creamos el cliente
  WiFiClient client;
  const int httpPort = 80; // Puerto HTTP
  if (!client.connect(host, httpPort)) {
      // ¿hay algún error al conectar?
      Serial.println("Ha fallado la conexión");
      return;
  }

  int err;
  float temp, hum;
  if ( (err = dht11.read(hum,temp)) == 0 )  
  {
      Serial.print("Temperatura : ");
      Serial.println(temp);
      Serial.print("Humedad: ");
      Serial.println(hum);

      Serial.println("IP COMO STRING " + ip);
      
      // Creo los datos a enviar
      String data = "temp=" + (String)temp +"&hum=" + (String)hum + "&ip=" +  WiFi.localIP().toString();
      
      String url = "/arduino/webservice/domo.php";
    
      Serial.println("Enviando " + data);
 
       client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               //"Connection: close\r\n" +
               "Accept: *" + "/" + "*\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + data.length() + "\r\n" +
               "\r\n" + // This is the extra CR+LF pair to signify the start of a body
               data + "\n");

      //Muestro las respuestas desde el server
      Serial.println("Respuesta del servidor:");
      while ( client.available() ){
          String resp = client.readStringUntil('\r');
          Serial.println(resp);
       }
      
  }else{
      Serial.println("Error: " + err);
  }
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
      if (millis() - timeout > 2000) {
          Serial.println(">>> Superado el tiempo de espera !");
          client.stop();
          return;
      }
  }

  // Consutar la memoria libre
  // Quedan un poco más de 40 kB
  Serial.printf("\nMemoria libre en el ESP8266: %d Bytes\n\n",ESP.getFreeHeap());

  // Leemos la respuesta y la enviamos al monitor serie
  while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
  }

  Serial.println();
  Serial.println("Cerrando la conexión");
}  
