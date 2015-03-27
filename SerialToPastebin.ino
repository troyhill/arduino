/* This code does the following: 
(1) retrieves a pastebin session key, and  
(2) posts arbitrary text entered into the serial terminal.

This code requires the following conditions:
- an Arduino wireless card
- an internet connection (fill in network SSID and password below)
- a pastebin account (fill in your username, password, and API dev key from http://pastebin.com/api/)

WiFi connection code is modified from Arduino example code "ConnectWithWPA." Credit: 
 "created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe"
*/

// issues: - I appear to be limited to ~63 characters regardless of INPUTLENGTH.
//            ^This may be due to some buffer size limitation. How to modify or workaround?
//         - printing pasteID from previous paste


#include <SPI.h>
#include <WiFi.h>

char ssid[] = "ye_ole_internetwork";      // network SSID
char pass[] = "ye_ole_WPA_password";      // network password

// variables for session key function
char session_key[33];
char api_user_name[] = "username";     // Pastebin username
char api_user_password[] = "password"; // Pastebin password
char api_dev_key[] = "your_dev_key";   // Pastebin dev key (http://pastebin.com/api/)
char key_tag[] = "&api_dev_key=";
char req1[] = "POST ";
char req2[] = "&api_user_name=";
char req3[] = "&api_user_password=";
int b1 = strlen(api_dev_key) + strlen(api_user_name) + strlen(api_user_password) + strlen(key_tag) + strlen(req2) + strlen(req3);
const int buffer = 32; // length of session key
char seshKey[buffer + 1]; // allow for the terminating null [buffer + 1]


// variables for paste function
const int idBuffer = 20;
char pasteID[idBuffer + 1]; // holds paste ID before merging to pasteList (more than enough characters)
#define INPUTLENGTH 150 // max length of input array
static int pos = 0; // Index into array; where to store the character
static char inData[INPUTLENGTH + 1]; // Allocate some space for the string


// variables for wireless settings
int status = WL_IDLE_STATUS;
// could use the numeric IP instead of the name for the server:
// IPAddress server(190,93,243,15);  // numeric IP (no DNS)
char server[] = "pastebin.com";    // name address (using DNS)

// Initialize the Ethernet client library
WiFiClient client;






void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  // check for wifi shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while(true);
  }
 
  // attempt to connect to wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network   
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
}



void loop() {
// if pastebin session key doesn't exist, get one. 
// This could use a counter so it doesn't get stuck in an endless loop
// if, e.g., website is unavailable
 if(strlen(seshKey) < 32) { 
  keyReq(seshKey);
  delay(3000);
  if(strlen(seshKey) == 32) { 
  Serial.print("Session key: ");
  Serial.println(seshKey); // print the key to the serial window
  }  
 }
 
 // get text entered into serial
if(Serial.available()) {
   // wait for full message. slight chance this may need upward adjustment to allow for longer messages
   delay(100);
   
   while (Serial.available() > 0){
      
    if (pos < INPUTLENGTH){
        inData[pos] = Serial.read();
        pos++;
        inData[pos] = 0;
    } else {
        pos = 0;
        inData[pos] = 0;
   }
  }

  if(Serial.available() == 0){
  pos = 0; // reset writing position
  
   // and make a paste from serial input
   // The paste ID may not be registering the first time around.
   // Printing customized text will help troubleshoot this by
   // allowing me link pastes to specific inputs and figure out what's going on.
 if(strlen(seshKey) == 32) {
  //char pasteInput[] = "Hello, world."; // uncomment if you want to print a pre-defined message
  pasteIt(inData); // formerly pasteInput
  Serial.print("input text: ");
  Serial.println(inData);
  }
  }
   }
}


// function retrieves a session key from pastebin API
char keyReq(char *keyTemp){
  if (client.connect(server, 80)) {
//    Serial.println("Connected to server. Retrieving session key..."); // uncomment if you're not sure you're connecting
    // Make a HTTP request:
    client.print(req1);
    client.print("/api/api_login.php HTTP/1.1\r\n");
    client.print("Host:pastebin.com\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type:application/x-www-form-urlencoded\r\n");
    client.print("Content-Length:"); 
    client.print(b1);
    client.println("\r\n");
    client.print(key_tag); 
    client.print(api_dev_key);
    client.print(req2); 
    client.print(api_user_name);
    client.print(req3); 
    client.print(api_user_password);
}
    while (client.available()) {
    if(client.find("EWR")){ // find distinctive text anchor
            if(client.find("20")){ // "20" seems to always precede the key
              if(client.find("\r\n")){ // as does a newline character
    client.readBytesUntil('\r\n', keyTemp, buffer); // save text through next newline to keyTemp
    }}
    }
  }
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
}





// function writes a paste and returns a string with the paste ID
char pasteIt(char *text){ 
Serial.println("Making a paste..."); // remove once function works

char pasteIDtemp[idBuffer + 1];
char api_user_key_tag[] = "&api_user_key="; // points to session_key
char api_option_tag[] = "&api_option=paste";
char api_paste_code_tag[] = "&api_paste_code=";
char api_paste_expire_date_tag[] = "&api_paste_expire_date=";
char api_paste_expire_date[] = "1M";
char api_paste_private_tag[] = "&api_paste_private=";
char api_paste_private[] = "0";
char api_paste_name_tag[] = "&api_paste_name=";
char api_paste_name[] = "Untitled";

int contentLength = strlen(key_tag) + strlen(api_dev_key) + strlen(api_user_key_tag) + strlen(seshKey) +// seshKey is session key
  strlen(api_option_tag) + strlen(api_paste_code_tag) +
  strlen(text) + strlen(api_paste_expire_date_tag) + strlen(api_paste_expire_date) + 
  strlen(api_paste_private_tag) + strlen(api_paste_private) + strlen(api_paste_name_tag) + 
  strlen(api_paste_name);
  
if (client.connect(server, 80)) {
    client.print(req1);
    client.print("/api/api_post.php HTTP/1.1\r\n");
    client.print("Host:pastebin.com\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type:application/x-www-form-urlencoded\r\n");
    client.print("Content-Length:"); 
    client.print(contentLength);
    client.println("\r\n");
    client.print(key_tag); 
    client.print(api_dev_key);
    client.print(api_user_key_tag);
    client.print(seshKey); // session_key
    client.print(api_option_tag);
    client.print(api_paste_code_tag);
    client.print(text);
    client.print(api_paste_expire_date_tag);
    client.print(api_paste_expire_date);
    client.print(api_paste_private_tag);
    client.print(api_paste_private);
    client.print(api_paste_name_tag);
    client.print(api_paste_name);
}
 while (client.available()) {
    if(client.find("http://pastebin.com/")){ // find distinctive text anchor
    client.readBytesUntil('\r\n', pasteIDtemp, idBuffer); // seek text until next newline, save to pasteID
    // still printing old pasteID, whether this object is "pasteID" or a variable defined in local scope
    }
    Serial.print("Paste ID: ");
    Serial.println(pasteIDtemp); // still printing old pasteID, whether this object is "pasteID" or a variable defined in local scope
  
 }    
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
}
