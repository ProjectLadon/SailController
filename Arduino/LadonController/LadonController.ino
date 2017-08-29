//#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include <SPI.h>
#include <WiFi101.h>

#define FUNCTION_PIN  (2)
#define NEOPIXEL_PIN  (1)
#define AFT_INPUT     (A0)
#define FORE_INPUT    (A1)

#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (15)

// IP Addresses
IPAddress foresailIP(192,168,0,90);
IPAddress mizzenIP(192,168,0,91);
IPAddress controllerIP(192,168,0,92);

// Status
int status = WL_IDLE_STATUS;

// WiFi parameters
char ssid[] = "Underworld";
char password[] = "divedivedive";

// Initialize the client library
WiFiClient clientForesail;
WiFiClient clientMizzen;

// Neopixel
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  
  // set up pins
  pinMode(FUNCTION_PIN, INPUT);

  // Connect to WiFi
  WiFi.config(controllerIP);
  while (status != WL_CONNECTED) {
    if (Serial) Serial.print("Attempting to connect to SSID: ");
    if (Serial) Serial.println(ssid);
    status = WiFi.begin(ssid, password);

    // Wait 10 seconds for connection:
    delay(10000);
  }
  if (Serial) Serial.println("WiFi connected");

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);
}

void loop() {

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
    if (foreinput > SERVO_RANGE) foreinput = SERVO_RANGE;
    if (foreinput < -SERVO_RANGE) foreinput = -SERVO_RANGE;
    if (aftinput > SERVO_RANGE) foreinput = SERVO_RANGE;
    if (aftinput < -SERVO_RANGE) foreinput = -SERVO_RANGE;
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
    clientForesail.print("GET /sail?params=0"); // for reasons completely opaque to me, aREST drops the first character, so we add a leading zero
    clientForesail.print(foreinput);
    clientForesail.println(" HTTP/1.1");
    clientForesail.println();
    clientMizzen.print("GET /sail?params=");
    clientMizzen.print(aftinput);
    clientMizzen.println(" HTTP/1.1");
    clientMizzen.println();
  } else {
    if (Serial) Serial.println("Connection failed");
  }
  delay(50);
}