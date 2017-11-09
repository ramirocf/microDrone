/* 
 * Con este programa hacemos que el ESP8266 actue como un access point al cual nos conectamos y por medio del 
 * protocolo telnet enviamos y recibimos caracteres, los cuales al llegar al ESP8266 son impresos en el puerto
 * serial. De igual manera los caracteres que son recibidos por el puerto serial son enviados por medio de telnet
 * a la computadora que este conectada en ese momento, implementando una especie de chat Serial <----> Telnet
 * 
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

//Defined Variables-------------------------------------------------------------------------------------------------------

#define BUILTIN_LED2 13

//Nombre de la red y contraseña
const char *ssid = "microDrone2";
const char *password = "pass1234";

//Variable para saber si se mando el mensaje
bool sent = true;
int i = 0;

//Variables usadas para la recepción de mensajes
char inByte;
String message = "\0";
//------------------------------------------------------------------------------------------------------------------------

//Inicializamos el cliente al puerto 23 donde comunmente se coloca el acceso telnet
WiFiServer telnetServer(23);
WiFiClient serverClient;
enum INIT_STATES {
  INFORM_SERVER_UP,
  WAIT_FOR_CLIENT,
  INFORM_CLIENT_UP,
  NORMAL_OP
};
INIT_STATES initState = INFORM_SERVER_UP;
bool initDone = false;
void setup()
{
//Inicializamos el puerto serial con Baudrate de 115200 para recibir datos
delay(1000);
Serial.begin(115200);
Serial.println();
//Serial.println("Puerto Serial Configurado...");

//Inicializamos el modulo Wifi del ESP8266  
//Serial.println("Configurando el modulo WiFi...");
WiFi.softAP(ssid, password);
IPAddress myIP = WiFi.softAPIP();

//Imprimimos la IP donde se va a encontrar el servidor telnet
//Serial.print("AP IP...");
//Serial.println(myIP);

//Iniciamos el servidor telnet
telnetServer.begin();
telnetServer.setNoDelay(true);
//Serial.println(ESP.getFreeHeap());
}

void initFSM(){
  switch (initState) {
    case INFORM_SERVER_UP:
      Serial.println("serverUp");
      if (Serial.available()){
        if (Serial.read() == 'a'){
          initState = WAIT_FOR_CLIENT;
        }
      }
      delay(500);
      break;
    case WAIT_FOR_CLIENT:
        if (telnetServer.hasClient()) {
          serverClient = telnetServer.available();
          serverClient.flush();  // clear input buffer, else you get strange characters 
          initState = INFORM_CLIENT_UP;
        }
        break;
    case INFORM_CLIENT_UP:
      Serial.println("clientUp");
      if (Serial.available()) {
        if (Serial.read() == 'a') {
          initState = NORMAL_OP;
        }
      }
      delay(500);
      break;
    case NORMAL_OP:
      initDone = true;
      break; 
    default:
      break;
  }
}

void loop()
{
  if (!initDone){
    initFSM();
    return;
  }
  if (!serverClient.connected()) {
    serverClient.stop();
    Serial.println("Telnet Client Stop");
    initDone = false;
    initState = WAIT_FOR_CLIENT;
  }
  // Revisamos el puerto serial para ver si tenemos un nuevo mensaje, 
  //considerar que el puerto debe borrarse si llega un mensaje nuevo.
  if( Serial.available())
  {
    while(Serial.available())
    {
      inByte = Serial.read();
      message += inByte;
    }
    Serial.print("Mensaje enviado: ");
    Serial.println(message);
    sent = false;
  }
  //Revisamos si el cliente nos envía mensajes por Wifi, si es asi los enviamos por
  //el puerto serial sin modificarlos. (Usuario a uController)
  while(serverClient.available()) 
  {
    Serial.write(serverClient.read());
  }

  //Si recibimos mensaje desde el puerto serial lo enviamos a traves del modulo Wifi
  if (sent == false)
  {
    if (serverClient && serverClient.connected()) 
    {
        serverClient.println(message);        
        ESP.getFreeHeap();
        message = "\0";
        sent = true;
    }
  } 
  delay(10);  // Eliminamos el rudio...
}
