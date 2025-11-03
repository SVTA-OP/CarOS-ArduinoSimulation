#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9

#define ACCEL_BUTTON_PIN 2
#define LED_PIN 4
#define NEXT_TRACK_PIN 5
#define PREV_TRACK_PIN 6

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

const byte BUFFER_SIZE = 100;
char inputBuffer[BUFFER_SIZE];
byte bufferIndex = 0;
bool stringComplete = false;
int lastButtonState = LOW;

// Variables for non-blocking LED flash
bool ledIsOn = false;
unsigned long ledTurnOffTime = 0;
const long ledFlashDuration = 100;

// Draws the static UI frames and titles on the screen.
void drawLayout() {
  tft.fillScreen(ILI9341_BLACK);
  // Speed Tile
  tft.drawRect(10, 20, 140, 140, ILI9341_CYAN);
  tft.setCursor(45, 30); tft.setTextColor(ILI9341_CYAN); tft.setTextSize(2); tft.print("SPEED");
  // Distance Tile
  tft.drawRect(170, 20, 140, 140, ILI9341_GREEN);
  tft.setCursor(180, 30); tft.setTextColor(ILI9341_GREEN); tft.setTextSize(2); tft.print("Dist");
  // Music Tile
  tft.drawRect(5, 180, 310, 55, ILI9341_MAGENTA);
}

// Runs once at startup to initialize hardware and serial communication.
void setup() {
  Serial.begin(9600);
  tft.begin();
  tft.setRotation(1);
  pinMode(ACCEL_BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(NEXT_TRACK_PIN, INPUT);
  pinMode(PREV_TRACK_PIN, INPUT);
  drawLayout();
  Serial.println("Arduino Dashboard Ready!");
}

// Checks all buttons and sends updates.
void handleInputs() {
  int accelState = digitalRead(ACCEL_BUTTON_PIN);
  if (accelState != lastButtonState) {
    if (accelState == HIGH) Serial.println("IN|ACCEL");
    else Serial.println("IN|IDLE");
    lastButtonState = accelState;
  }
  if (digitalRead(NEXT_TRACK_PIN) == HIGH) {
    Serial.println("IN|NEXT");
    delay(200);
  }
  if (digitalRead(PREV_TRACK_PIN) == HIGH) {
    Serial.println("IN|PREV");
    delay(200);
  }
}

// Reads incoming characters from the serial port into a buffer.
void readSerial() {
  while (Serial.available() > 0 && !stringComplete) {
    char inChar = Serial.read();
    if (inChar == '\n') {
      inputBuffer[bufferIndex] = '\0';
      stringComplete = true;
    } else if (bufferIndex < BUFFER_SIZE - 1) {
      inputBuffer[bufferIndex++] = inChar;
    }
  }
}

// Manages the non-blocking LED flash timer.
void updateLedState() {
  if (ledIsOn && millis() >= ledTurnOffTime) {
    digitalWrite(LED_PIN, LOW);
    ledIsOn = false;
  }
}

// The main Arduino loop that runs continuously.
void loop() {
  handleInputs();
  readSerial();
  updateLedState();
  if (stringComplete) {
    processMessage(inputBuffer);
    bufferIndex = 0;
    stringComplete = false;
  }
}

// Triggers the LED to turn on and sets its turn-off timer.
void flash_led() {
  digitalWrite(LED_PIN, HIGH);
  ledIsOn = true;
  ledTurnOffTime = millis() + ledFlashDuration;
}

// **CODE CHANGED HERE: Added a case for the "GEAR" command.**
// Parses a message from the C program and calls the correct display function.
void processMessage(char* message) {
  char* separator = strchr(message, '|');
  if (separator != NULL) {
    *separator = '\0';
    const char* cmd = message;
    const char* text = separator + 1;
    
    if (strcmp(cmd, "TIME") == 0) displayTime(text);
    else if (strcmp(cmd, "MUSIC") == 0) displayMusic(text);
    else if (strcmp(cmd, "SPEED") == 0) displaySpeed(text);
    else if (strcmp(cmd, "DIST") == 0) displayDistance(text);
    else if (strcmp(cmd, "GEAR") == 0) displayGear(text); // New case for gear
    else if (strcmp(cmd, "BEEP") == 0) flash_led();

    *separator = '|';
  }
}

// Clears the speed area and draws the new speed value.
void displaySpeed(const char* speedText) {
  tft.fillRect(15, 55, 130, 80, ILI9341_BLACK);
  tft.setCursor(45, 75); tft.setTextColor(ILI9341_WHITE); tft.setTextSize(6);
  tft.print(speedText);
}

// **CODE CHANGED HERE: New function to display the gear.**
// Clears the gear area and draws the new gear number.
void displayGear(const char* gearText) {
    tft.fillRect(15, 130, 130, 25, ILI9341_BLACK); // Clear area under speed
    tft.setCursor(60, 135);
    tft.setTextColor(ILI9341_CYAN);
    tft.setTextSize(2);
    tft.print("Gear: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(gearText);
}

// Clears the distance area and draws the new distance value.
void displayDistance(const char* distText) {
  tft.fillRect(175, 55, 130, 90, ILI9341_BLACK);
  tft.setCursor(175, 85); tft.setTextColor(ILI9341_WHITE); tft.setTextSize(2);
  for (int i = 0; distText[i] != '\0'; i++) {
    if (distText[i] == '.') break;
    tft.print(distText[i]);
  }
}

// Clears the time area and draws the current time.
void displayTime(const char* timeText) {
  tft.fillRect(0, 0, 320, 18, ILI9341_BLACK);
  tft.setCursor(10, 5); tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.print("TIME: "); tft.setTextColor(ILI9341_WHITE);
  tft.print(timeText);
}

// Clears the music area and draws the current track name.
void displayMusic(const char* musicText) {
  tft.fillRect(10, 185, 300, 45, ILI9341_BLACK);
  tft.setCursor(10, 185); tft.setTextColor(ILI9341_MAGENTA); tft.setTextSize(2);
  tft.print("Music: "); tft.setCursor(10, 210); tft.setTextColor(ILI9341_WHITE); tft.setTextSize(2);
  tft.print(musicText);
}