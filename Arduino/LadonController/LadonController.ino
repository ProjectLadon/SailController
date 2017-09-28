#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <SPI.h>
#include <WiFi101.h>

#define FUNCTION_PIN  (2)
#define NEOPIXEL_PIN  (1)
#define AFT_INPUT     (A0)
#define FORE_INPUT    (A1)

#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (30)

// IP Addresses
IPAddress foresailIP(192,168,0,90);
IPAddress mizzenIP(192,168,0,91);
IPAddress controllerIP(192,168,0,92);

// Status
int status = WL_IDLE_STATUS;

// WiFi parameters
char ssid[] = "Underworld";
char password[] = "divedivedive";

// Neopixel
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  
  // set up pins
  pinMode(FUNCTION_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set up the pixel
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(255,0,0));     // set it to red when there's no connection
  pixels.show();
  
  // Connect to WiFi
  WiFi.config(controllerIP);
  while (status != WL_CONNECTED) {
    if (Serial) Serial.print("Attempting to connect to SSID: ");
    if (Serial) Serial.println(ssid);
    status = WiFi.begin(ssid, password);

    // Wait 1 seconds for connection:
    delay(1000);
  }
  if (Serial) Serial.println("WiFi connected");
  pixels.setPixelColor(0, pixels.Color(255,194,0));   // set it to amber when the wifi is connected
  pixels.show();

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);
}

void loop() {

  WiFiClient clientForesail;
  WiFiClient clientMizzen;

  // check for connection, reconnect if missing
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    pixels.setPixelColor(0, pixels.Color(255,194,0));   // set it to amber when the wifi is connected
  } else {
    pixels.setPixelColor(0, pixels.Color(255,0,0));     // set it to red when there's no connection
    pixels.show();
    digitalWrite(LED_BUILTIN, LOW);
    WiFi.begin(ssid, password);
    delay(100);
    return;
  }

  // Read inputs
  int foreinput = map(analogRead(FORE_INPUT), 0, 1023, -SERVO_RANGE, SERVO_RANGE);
  int aftinput = map(analogRead(AFT_INPUT), 0, 1023, -SERVO_RANGE, SERVO_RANGE);
  int function = digitalRead(FUNCTION_PIN);

  // if we're in relative mode, modify the inputs appropriately
  if (function == LOW) {    // relative/differential mode
    if (Serial) Serial.print("Differential Mode\t");
    int base = foreinput;
    int offset = aftinput;
    foreinput = base + offset;
    aftinput = base - offset;
  }
  if (foreinput > SERVO_RANGE) {
    foreinput = SERVO_RANGE;
  }
  if (foreinput < -SERVO_RANGE) {
    foreinput = -SERVO_RANGE;
  }
  if (aftinput > SERVO_RANGE) {
    aftinput = SERVO_RANGE;
  }
  if (aftinput < -SERVO_RANGE) {
    aftinput = -SERVO_RANGE;
  }

  // shift the data to center at 90 degrees
  foreinput += SERVO_OFFSET;
  aftinput += SERVO_OFFSET;

  if (Serial) {
    Serial.print(foreinput);
    Serial.print('\t'); 
    Serial.println(aftinput);
  }
  
  if (clientForesail.connect(foresailIP, 80) && clientMizzen.connect(mizzenIP, 80)) { // make sure we connect to both
    pixels.setPixelColor(0, pixels.Color(0,255,0));     // set it to green when there's a connection
    String forebuf = "GET /sail?param=0" + String(foreinput);
    forebuf += " HTTP/1.1\r\n\r\n";
    if (Serial) Serial.println(forebuf);
    String mizbuf = "GET /sail?param=0" + String(aftinput);
    mizbuf += " HTTP/1.1\r\n\r\n";
    if (Serial) Serial.print(mizbuf);
    //clientForesail.print("GET /sail?param=0"); // for reasons completely opaque to me, aREST drops the first character, so we add a leading zero
    //clientForesail.print(foreinput);
    //clientForesail.println(" HTTP/1.1");
    //clientForesail.println("");
    //clientForesail.println("");
    clientForesail.println(forebuf);
    //clientMizzen.print("GET /sail?param=0");
    //clientMizzen.print(aftinput);
    //clientMizzen.println(" HTTP/1.1");
    //clientMizzen.println("");
    //clientMizzen.println("");
    clientMizzen.print(mizbuf);
  } else {
    if (Serial) Serial.println("Connection failed");
    pixels.setPixelColor(0, pixels.Color(255,194,0));   // set it to amber when the wifi is connected
  }
  clientMizzen.stop();
  clientForesail.stop();
  pixels.show();
  delay(100);
}
