/*
PS/2 Keyboard Terminal with TFT Display
An ESP32-based interactive terminal that interfaces with a PS/2 keyboard and displays output on the CYD. This project creates a simple command-line interface with real-time display updates and basic command processing capabilities.

Description
This project implements a retro-style terminal interface using modern hardware. It combines PS/2 keyboard input processing with a TFT display output, creating an interactive command-line environment. The system features a scrolling display buffer, cursor tracking, and a simple command processor that can control an LED and respond to basic system commands.
Key features include:

Real-time PS/2 keyboard input processing with interrupt handling
Scrolling text display with support for up to 10 lines of history
Visual cursor feedback showing current input position
Command processing system (to be extended):
"led on/off": Controls an onboard LED
"cls": Clears the screen
"state": Reports the current LED state

The implementation uses efficient buffer management for both keyboard input and display output, with careful attention to memory usage and real-time processing requirements. The code includes a complete PS/2 scancode mapping for standard keyboard layout and handles special keys like Backspace and Enter.
Technical details:

Hardware: ESP32 microcontroller
Display: TFT screen using TFT_eSPI library
Input: PS/2 keyboard (Clock Pin: 27, Data Pin: 14)
Built-in LED control on Pin 2
30-character input buffer with overflow protection
10-line display buffer with automatic scrolling

ToDo:
Cursor behavior is not working properly after pressing backspace multiple times.
*/

#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

/*
To be on the save side, please use 5V-3V level converters for the input pins to not fry your CYD inputs with 5V.
I tested my keyboard with 3.3V but, as expected, it didn't work reliable.
*/
#define CLOCK_PIN 27
#define DATA_PIN 22
#define LED_PIN 2
#define BUFFER_SIZE 30
#define ENTER_KEY 0x5A
#define BACKSPACE_KEY 0x66  // Adding PS/2 backspace scancode
#define MAX_LINES 18
#define LINE_HEIGHT 16
#define CURSOR_CHAR '_'

// Keep your existing keymap array here
const char keymap[] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, '`', 0,
  0, 0, 0, 0, 0, 'q', '1', 0,
  0, 0, 'z', 's', 'a', 'w', '2', 0,
  0, 'c', 'x', 'd', 'e', '4', '3', 0,
  0, ' ', 'v', 'f', 't', 'r', '5', 0,
  0, 'n', 'b', 'h', 'g', 'y', '6', 0,
  0, 0, 'm', 'j', 'u', '7', '8', 0,
  0, ',', 'k', 'i', 'o', '0', '9', 0,
  0, '.', '/', 'l', ';', 'p', '-', 0,
  0, 0, '\'', 0, '[', '=', 0, 0,
  0, 0, 13, ']', 0, '\\', 0, 0,
  0, 0, 0, 0, 0, 0, 127, 0,
  0, '1', 0, '4', '7', 0, 0, 0,
  '0', '.', '2', '5', '6', '8', 0, 0,
  0, '+', '3', '-', '*', '9', 0, 0,
  0, 0, 0, 0
};

struct LineBuffer {
  String lines[MAX_LINES];
  int currentLine;
  int totalLines;
  int cursorX;  // Added to track cursor position

  void init() {
    currentLine = 0;
    totalLines = 0;
    cursorX = 0;  // Initialize cursor position
    for (int i = 0; i < MAX_LINES; i++) {
      lines[i] = "";
    }
  }

  void addLine(const char* text) {
    lines[currentLine] = String(text);
    currentLine = (currentLine + 1) % MAX_LINES;
    if (totalLines < MAX_LINES) totalLines++;
    cursorX = 0;  // Reset cursor position for new line
    redrawScreen();
  }

  void redrawScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 2);

    int startPos = (currentLine - totalLines + MAX_LINES) % MAX_LINES;

    for (int i = 0; i < totalLines; i++) {
      int pos = (startPos + i) % MAX_LINES;
      tft.setCursor(0, i * LINE_HEIGHT);
      tft.println(lines[pos]);
    }

    // Position cursor at end of current line
    tft.setCursor(cursorX * 6, totalLines * LINE_HEIGHT);  // Assuming 6 pixels per character
    tft.print(CURSOR_CHAR);
  }
} lineBuffer;

// Command processing function
void processCommand(const char* cmd) {
  char strState[2];
  String command = String(cmd);
  command.trim();  // Remove leading/trailing whitespace

  if (command == "led on") {
    digitalWrite(LED_PIN, HIGH);
    lineBuffer.addLine("led turned on");
  } else if (command == "led off") {
    digitalWrite(LED_PIN, LOW);
    lineBuffer.addLine("led turned OFF");
  } else if (command == "cls") {
    lineBuffer.init();
    lineBuffer.addLine("");
    tft.fillScreen(TFT_BLACK);
  } else if (command == "state") {
    lineBuffer.addLine(itoa(digitalRead(2), strState, 10));
  }
}

struct InputBuffer {
  char data[BUFFER_SIZE];
  uint8_t position;

  void init() {
    position = 0;
    clear();
  }

  void clear() {
    memset(data, 0, BUFFER_SIZE);
  }

  bool add(char c) {
    if (position >= BUFFER_SIZE - 1) return false;
    data[position++] = c;
    data[position] = '\0';
    lineBuffer.cursorX = position;  // Update cursor position
    return true;
  }

  bool backspace() {
    if (position > 0) {
      position--;
      data[position] = '\0';
      lineBuffer.cursorX = position;  // Update cursor position
      return true;
    }
    return false;
  }

  void print() {
    Serial.println(data);
    lineBuffer.addLine(data);
    // Process command after displaying it
    processCommand(data);
    clear();
    position = 0;
    lineBuffer.cursorX = 0;  // Reset cursor position
  }
} buffer;

// Keep your existing PS/2 buffer and interrupt handler code here
volatile struct {
  uint16_t data;
  uint8_t bits;
  bool ready;
} ps2_buffer = { 0, 0, false };

volatile uint8_t scanval = 0;
volatile bool newKey = false;

void IRAM_ATTR clockInterrupt() {
  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL_ISR(&mux);

  if (ps2_buffer.bits < 11) {
    ps2_buffer.data |= (digitalRead(DATA_PIN) << ps2_buffer.bits);
    ps2_buffer.bits++;
    if (ps2_buffer.bits == 11) {
      ps2_buffer.ready = true;
    }
  }

  portEXIT_CRITICAL_ISR(&mux);
}

void processPS2Data() {
  static bool releaseCode = false;
  static uint8_t lastKey = 0;

  if (ps2_buffer.ready) {
    uint8_t val = (ps2_buffer.data >> 1) & 0xFF;

    if (val == 0xF0) {
      releaseCode = true;
    } else if (releaseCode) {
      lastKey = 0;
      releaseCode = false;
    } else if (val != lastKey && val < sizeof(keymap)) {
      scanval = val;
      newKey = true;
      lastKey = val;
    }

    ps2_buffer.data = 0;
    ps2_buffer.bits = 0;
    ps2_buffer.ready = false;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Start with LED off

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);

  pinMode(CLOCK_PIN, INPUT_PULLUP);
  pinMode(DATA_PIN, INPUT_PULLUP);

  buffer.init();
  lineBuffer.init();

  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), clockInterrupt, FALLING);

  lineBuffer.addLine("r
  eady:");
}

void loop() {
  processPS2Data();

  if (newKey) {
    if (scanval == BACKSPACE_KEY) {
      if (buffer.backspace()) {
        // Redraw the current line with the character removed
        tft.fillRect(0, lineBuffer.totalLines * LINE_HEIGHT, tft.width(), LINE_HEIGHT, TFT_BLACK);
        tft.setCursor(0, lineBuffer.totalLines * LINE_HEIGHT);
        tft.print(buffer.data);
        tft.print(CURSOR_CHAR);
      }
    } else {
      char key = keymap[scanval];
      if (key) {
        if (scanval == ENTER_KEY) {
          tft.println();
          buffer.print();  // This will now also process any commands
        } else {
          if (buffer.add(key)) {
            // Redraw the current line with the new character
            tft.fillRect(0, lineBuffer.totalLines * LINE_HEIGHT, tft.width(), LINE_HEIGHT, TFT_BLACK);
            tft.setCursor(0, lineBuffer.totalLines * LINE_HEIGHT);
            tft.print(buffer.data);
            tft.print(CURSOR_CHAR);
          } else {
            Serial.println("Buffer full!");
          }
        }
      }
    }
    newKey = false;
  }

  delay(1);
}
