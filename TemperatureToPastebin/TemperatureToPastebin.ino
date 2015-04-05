/* This code retrieves a pastebin session key and 
posts temperature data recorded by probe
*/

// data posted to pastebin is limited to 90 characters. 

#include <SPI.h>
#include <WiFi.h>


// Modify these lines to match your setup:
char ssid[] = "ye_ole_internetwork";      // network SSID
char pass[] = "ye_ole_WPA_password";      // network password
char api_user_name[] = "username";     // Pastebin username
char api_user_password[] = "password"; // Pastebin password
char api_dev_key[] = "your_dev_key";   // Pastebin dev key (http://pastebin.com/api/)


// parameters for session key function
char key_tag[] = "&api_dev_key=";
char req1[] = "POST ";
char req2[] = "&api_user_name=";
char req3[] = "&api_user_password=";
int b1 = strlen(api_dev_key) + strlen(api_user_name) + strlen(api_user_password) + strlen(key_tag) + strlen(req2) + strlen(req3);
const int buffer = 32; // length of session key
char seshKey[buffer + 1]; // allow for the terminating null [buffer + 1]

// parameters for paste function
const int idBuffer = 20;
char pasteID[idBuffer + 1];


// parameters for temperature readings
// Board setup: 5V to resistor between thermister wire and line to A0; 2nd therm wire going to ground
// which analog pin connects to thermister
#define THERMISTORPIN A0
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000
// length of data file stored in RAM
#define dataLength 90 // 90 works, 100 appears not to
// sampling interval in milliseconds (very simplistic - Arduino doesn't go into a low-power mode)
#define pauseInterval 3000 // 5 minutes = 5 * 60 * 1000 = 30000 milliseconds
    
int samples[NUMSAMPLES];
char GlobTmp[12]; // temperature array in global scope
char data[dataLength] = "Deg_F:,"; // this is where temperature values will accumulate
char sep[] = ","; // set separator



// variables for wireless settings
int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
// IPAddress server(190,93,243,15);  // numeric IP (no DNS)
char server[] = "pastebin.com";    // name address (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;






void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while(true);
  }
 
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    // Connect to WPA/WPA2 network   
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  
  if (status == WL_CONNECTED){
    Serial.print("Connected to SSID: ");
    Serial.println(ssid);
  }
  
  keyReq(seshKey); // try to get a key at the outset
  if(strlen(seshKey) == 32) { 
  Serial.println("Session key generated!");
//  Serial.println(seshKey); // print the key to the serial window. this never prints - pointer issue?
  }
}





void loop() {
// if pastebin session key doesn't exist, get one. 
// This could use a counter so it doesn't get stuck in an endless loop
// if, e.g., website is unavailable
static char pasteIDtemp[idBuffer + 1]; 
 
 if(strlen(seshKey) < 32) { 
  keyReq(seshKey);
  if(strlen(seshKey) == 32) { 
  Serial.print("Session key: ");
  Serial.println(seshKey); // print the key to the serial window
  }  
 }
 
 
 // save temperature readings to data file
 if(strlen(data) < (dataLength - 5)){ // if dataset ("data") has room, append a temperature reading
      temp(GlobTmp); // pass temp() function the array you want to modify (i.e., GlobTmp)
      strcat(data, GlobTmp); // append "GlobTmp" to "data"
      strcat(data, sep); // add a separator
      Serial.print("Temp: ");
      Serial.print(GlobTmp); // print temperature to serial window
      Serial.print(" *F, dataset length: "); // add units
      Serial.println(strlen(data)); // print the total length of our dataset
      Serial.println(data);  // print entire dataset
      delay(pauseInterval); // wait x milliseconds, then do it again
  }

if(strlen(data) >= (dataLength - 5)){ // if dataset ("data") has room, append a temperature reading
   pasteIt();
   Serial.println();
   Serial.print("Paste ID: ");
   //Serial.println(pasteIDtemp); 
   //strcpy(pasteID, pasteIDtemp);
   Serial.println(pasteID); 
   
 // if the connection to Pastebin is lost (or rejected, in the event you've
 // exceeded your paste-quota, this will stick the logger in an endless loop (or at least,
 // until it can paste again. Change this to suit your needs. 
 if(strlen(pasteID) > 5){ // if paste was successful, delete data file and Paste ID and start over
   sprintf(pasteID, "");
   sprintf(data, "Deg_F:,");
   //delay(pauseInterval); // wait 3 seconds, then do it again
 }
}
}






// function retrieves a session key from pastebin API
void keyReq(char *keyTemp){
  if (client.connect(server, 80)) {
//    Serial.println("Connected to server. Retrieving session key..."); // uncomment if more serial printing is desired
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
    delay(3000);
}
    while (client.available()) {
    if(client.find("EWR")){ // find distinctive text anchor
            if(client.find("20")){ // "20" seems to always precede the key
              if(client.find("\r\n")){ // as does a newline character
    client.readBytesUntil('\r\n', keyTemp, buffer); // save text through next newline to keyTemp
    }}
    }
    client.stop();
  }
}







// function writes a paste and returns a string with the paste ID
void pasteIt(){ 
 static char pasteIDtemp[idBuffer + 1]; 
  Serial.println("Making a paste..."); // remove once function works

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
  strlen(data) + strlen(api_paste_expire_date_tag) + strlen(api_paste_expire_date) + 
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
    client.print(data);
    client.print(api_paste_expire_date_tag);
    client.print(api_paste_expire_date);
    client.print(api_paste_private_tag);
    client.print(api_paste_private);
    client.print(api_paste_name_tag);
    client.print(api_paste_name);
    delay(3000);
}
 while (client.available()) {
    if(client.find("http://pastebin.com/")){ // find distinctive text anchor
    client.readBytesUntil('\r\n', pasteID, idBuffer); // seek text until next newline, save to pasteIDtemp
    //return pasteIDtemp; // if a returned pasteID is desired
    //strcpy(pasteID, pasteIDtemp);
    }
      client.stop();
    }    
 }






// function to read temperature
// input: a character array
// output: the same character array, overwritten with new temperature
char temp(char *tmp){
    uint8_t i;
    float average;
    
    // take N samples in a row, with a slight delay
    for (i=0; i< NUMSAMPLES; i++) {
    samples[i] = analogRead(THERMISTORPIN); // record voltage readings to object
    delay(10);
    }
    // average replicate readings
    average = 0;
    for (i=0; i< NUMSAMPLES; i++) {
    average += samples[i]; // sum measurements
    }
    average /= NUMSAMPLES; // divide by number of measurements

    // convert the value to resistance
    average = 1023 / average - 1;
    average = SERIESRESISTOR / average;
    float steinhart;
    steinhart = average / THERMISTORNOMINAL; // (R/Ro)
    steinhart = log(steinhart); // ln(R/Ro)
    steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart; // Invert
    steinhart -= 273.15; // convert to C
    float deg_f = steinhart * (9/5) + 32;
    ltoa(deg_f, tmp, 10); // convert to an array (10 is base)
    // check out dtostrf to preserve numbers right of decimal
    }
