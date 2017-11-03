#include <SPI.h>
#include <UIPEthernet.h> // Identical with Ethernet.h, but works with the module
#include "dht.h"  // Temp and humidity sensor lib
#include <LinkedList.h>

// Defining temp and humidity sensor
#define sensor A0
dht DHT;

// Buffer with caracters to send to client
char *_buffer;

// Current index in buffer (used when receiving)
int index;

LinkedList<String, void(*)(void)> list;

//Setting up unique MAC address for the arduino
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

//Setting static IP, DNS, GW and Subnet Mask
IPAddress ip(192, 168, 1, 2);
IPAddress myDns(192,168,1, 0);
IPAddress gateway(192, 168, 1, 0);
IPAddress subnet(255, 255, 255, 0);

// Using "random" port above 1000 to not interfare with reserved ports
EthernetServer server(27015);

EthernetClient client;

// Whether or not the client was connected previously
boolean alreadyConnected = false;

/* 
 * Compare two arrays where size of arr1 is less than size of arr2
 * so that we can compare a preset array to whats in the buffer.
 * This is used to decide which function to use for returning to client
 */
bool inBuffer(char *arr1, unsigned int size1){
  for (unsigned int i = 0; i < size1; i++){
    if (arr1[i] != _buffer[i]){
      return false;
    }
  }
  return true;
}

// Clears the input buffer and resets index variable
void clearBuffer() {
  for (int i = 0; i < 1024; i++) {
    _buffer[i] = 0;
  }
  index = 0;
}

// Calculates integer length
int ilen(int i) {
  int len = 0;
  while (i != 0) {
    i /= 10;
    len++;
  }
  return len;
}

// Integer to string function
String itos(int num, int len){
  String s;
  char cArr[len + 1];
  for (unsigned int i = 0; i < len; i++)
  {
    cArr[len - i - 1] = (num % 10) + 48;
    num /= 10;
  }
  cArr[len] = '\0';
  s = cArr;
  return s;
}

void sendTemp() {
  // Sends temperature to client
  DHT.read11(sensor);
  int temp = (int)DHT.temperature;
  server.println(temp);
}

void sendHum(){
  // Sends humidity to client        
  DHT.read11(sensor);
  int hum = (int)DHT.humidity;
  server.println(hum);
}

void _update(){
  DHT.read11(sensor);
  int temp = (int)DHT.temperature;
  int hum = (int)DHT.humidity;
  String s = itos(temp, ilen(temp)) + ", " + itos(hum, ilen(hum));
  Serial.println(s);
  server.print(s); 
}

void discon(){
  // If client sends dsiconnect, stop the communication
  alreadyConnected = false;
  client.stop();
  Serial.println("Client disconnected!");
}

void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  
  // start listening for clients
  server.begin();
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Prints the ip address to Serial monitor
  Serial.print("Chat server address:");
  Serial.println(Ethernet.localIP());

  // Instantiate buffer
  _buffer = new char[1024];  
  index = 0; 

  list.append("check_temp", sendTemp);
  list.append("check_hum", sendHum);
  list.append("update", _update);
  list.append("disconnect", discon);
}

void loop() {
  // Wait for a new client
  client = server.available();

  // When the client sends the first byte, say hello
  if (client) {
    if (!alreadyConnected) {      
      client.flush(); // Clear the input buffer
      Serial.println("We have a new client");
      client.println("Hello, client!");
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client to the buffer
      _buffer[index++] = client.read();
      Serial.print(_buffer[index - 1]); 
    }
        
    if (index > 0 && _buffer[index - 1] == '\0') {
      Serial.println();
      String s = _buffer;
      list.get(s)();     

      // Clear input buffers
      client.flush();
      clearBuffer();
    }
  }
}
