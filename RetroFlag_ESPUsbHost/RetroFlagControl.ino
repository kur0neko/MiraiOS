#include <Arduino.h>
#include <BleGamepad.h>
#include <NimBLEDevice.h>

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "usb/usb_host.h" 
}
#include "EspUsbHost.h" 

// --- PINS ---
const int REED_SWITCH_PIN = 17;
const int BOOT_BUTTON_PIN = 0;

// --- STATE ---
bool bleMode = false;
bool bleInitialized = false;

// --- BLE GAMEPAD ---
BleGamepad bleGamepad("Retro-Joystick", "RetroFlag", 100);

// HAT SWITCH CONSTANTS
#define HAT_UP          1
#define HAT_UP_RIGHT    2
#define HAT_RIGHT       3
#define HAT_DOWN_RIGHT  4
#define HAT_DOWN        5
#define HAT_DOWN_LEFT   6
#define HAT_LEFT        7
#define HAT_UP_LEFT     8
#define HAT_CENTER      0

class RetroflagBridge : public UsbHidDevice {
  //BUFFER FOR LAST DATA PACKET
  uint8_t lastData[8];
  
  //LENGTH OF LAST DATA PACKET
  int lastLen = 0;
  
  //TIME OF LAST PRINT
  unsigned long lastPrintTime = 0;

public:
  void onData(const usb_transfer_t *transfer) override {
    
    //GET DATA LENGTH
    int len = transfer->actual_num_bytes;
    
    //GET DATA POINTER
    uint8_t *data = transfer->data_buffer;

    //ANTI SPAM CHECK
    if (len == lastLen && memcmp(lastData, data, len) == 0) {
      return; 
    }

    //COPY DATA TO BUFFER
    if (len > 0 && len <= 8) {
      memcpy(lastData, data, len);
      lastLen = len;
    }

    //PROCESS IF ENOUGH DATA
    if (len >= 4) {
      uint8_t b2 = data[2];
      uint8_t b3 = data[3];
      
      //THROTTLED DEBUG OUTPUT
      if (millis() - lastPrintTime > 200 && (b2 != 0 || b3 != 0)) {
        Serial0.printf("Input: %02X %02X -> ", b2, b3);
        if (b2 & 0x01) Serial0.print("UP ");
        if (b2 & 0x02) Serial0.print("DOWN ");
        if (b2 & 0x04) Serial0.print("LEFT ");
        if (b2 & 0x08) Serial0.print("RIGHT ");
        if (b2 & 0x10) Serial0.print("START ");
        if (b2 & 0x20) Serial0.print("SELECT ");
        if (b3 & 0x01) Serial0.print("LB ");
        if (b3 & 0x02) Serial0.print("RB ");
        if (b3 & 0x10) Serial0.print("A ");
        if (b3 & 0x20) Serial0.print("B ");
        if (b3 & 0x40) Serial0.print("X ");
        if (b3 & 0x80) Serial0.print("Y ");
        Serial0.println();
        
        //UPDATE LAST PRINT TIME
        lastPrintTime = millis();
      }

      //SEND TO BLE IF CONNECTED
      if (bleMode && bleInitialized && bleGamepad.isConnected()) {
        
        //MAP BUTTONS PRESS/RELEASE
        (b3 & 0x20) ? bleGamepad.press(BUTTON_1) : bleGamepad.release(BUTTON_1);   // A
        (b3 & 0x10) ? bleGamepad.press(BUTTON_2) : bleGamepad.release(BUTTON_2);   // B
        (b3 & 0x80) ? bleGamepad.press(BUTTON_3) : bleGamepad.release(BUTTON_3);   // X
        (b3 & 0x40) ? bleGamepad.press(BUTTON_4) : bleGamepad.release(BUTTON_4);   // Y
        (b3 & 0x01) ? bleGamepad.press(BUTTON_5) : bleGamepad.release(BUTTON_5);   // LB
        (b3 & 0x02) ? bleGamepad.press(BUTTON_6) : bleGamepad.release(BUTTON_6);   // RB
        (b2 & 0x20) ? bleGamepad.pressSelect() : bleGamepad.releaseSelect();        // SELECT
        (b2 & 0x10) ? bleGamepad.pressStart() : bleGamepad.releaseStart();          // START
        
        //MAP DPAD TO HAT SWITCH
        uint8_t hat = HAT_CENTER;
        
        bool up = (b2 & 0x01);
        bool down = (b2 & 0x02);
        bool left = (b2 & 0x04);
        bool right = (b2 & 0x08);
        
        //CALCULATE HAT POSITION
        if (up && !down && !left && !right) {
          hat = HAT_UP;
        } else if (up && right) {
          hat = HAT_UP_RIGHT;
        } else if (right && !up && !down) {
          hat = HAT_RIGHT;
        } else if (down && right) {
          hat = HAT_DOWN_RIGHT;
        } else if (down && !left && !right) {
          hat = HAT_DOWN;
        } else if (down && left) {
          hat = HAT_DOWN_LEFT;
        } else if (left && !up && !down) {
          hat = HAT_LEFT;
        } else if (up && left) {
          hat = HAT_UP_LEFT;
        }
        
        //SET HAT SWITCH
        bleGamepad.setHat(hat);
        
        //SEND BLE REPORT
        bleGamepad.sendReport();
      }
    }
  }
};

EspUsbHost usbHost;
RetroflagBridge bridge;

//INITIALIZE BLE GAMEPAD
void initializeBLE() {
  if (!bleInitialized) {
    Serial0.println("\nInitializing BLE Gamepad...");
    
    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setAutoReport(false);
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD);
    bleGamepadConfig.setButtonCount(6);
    bleGamepadConfig.setHatSwitchCount(1);  // ENABLE 1 HAT SWITCH FOR DPAD
    bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);  // DISABLE AXES
    bleGamepadConfig.setIncludeStart(true);
    bleGamepadConfig.setIncludeSelect(true);
    
    //START BLE GAMEPAD
    bleGamepad.begin(&bleGamepadConfig);
    bleInitialized = true;
    
    Serial0.println(">> BLE Gamepad initialized and advertising");
    Serial0.println(">> Pair as 'Retro-Joystick'");
  }
}

//TERMINATE BLE CONNECTION
void terminateBLE() {
  if (bleInitialized) {
    Serial0.println("Terminating BLE connection...");
    
    //GET BLE SERVER INSTANCE
    NimBLEServer* pServer = NimBLEDevice::getServer();
    
    if (pServer) {
      
      //DISCONNECT ALL CLIENTS
      for (uint16_t connHandle : pServer->getPeerDevices()) {
        pServer->disconnect(connHandle);
        Serial0.printf("Disconnected device handle: %d\n", connHandle);
      }
      
      //STOP ADVERTISING
      NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
      if (pAdvertising) {
        pAdvertising->stop();
        Serial0.println("Advertising stopped");
      }
    }
    
    //DELAY FOR DISCONNECTION
    delay(200);
    
    //MARK AS INACTIVE
    bleInitialized = false;
    Serial0.println(">> BLE disconnected and hidden");
    Serial0.println(">> (BLE stack remains initialized for quick restart)");
  }
}

void setup() {
  Serial0.begin(115200);
  delay(1000);
  
  Serial0.println("\n========================================");
  Serial0.println("RetroFlag BLE Gamepad (NimBLE)");
  Serial0.println("========================================\n");
  
  //SETUP INPUT PINS
  pinMode(REED_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  
  Serial0.println(">> BLE NOT initialized (waiting for trigger)");
  Serial0.println(">> Press reed switch or BOOT button to enable BLE");
  
  delay(500);
  
  //SETUP USB HOST
  Serial0.println("Setting up USB Host...");
  usbHost.begin();
  usbHost.init(&bridge);
  
  Serial0.println("\n=== READY ===");
  Serial0.println("BLE Mode: OFF (press reed/boot to toggle)");
}

void loop() {
  
  //RUN USB HOST TASK
  usbHost.task();
  
  //MODE SWITCH LOGIC
  static bool lastState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static unsigned long lastToggle = 0;
  const unsigned long debounceDelay = 50;
  const unsigned long toggleCooldown = 1000;
  
  //READ INPUT PINS
  bool reedActive = (digitalRead(REED_SWITCH_PIN) == LOW);
  bool bootActive = (digitalRead(BOOT_BUTTON_PIN) == LOW);
  bool currentReading = (reedActive || bootActive) ? LOW : HIGH;
  
  //DEBOUNCE LOGIC
  if (currentReading != lastState) {
    lastDebounceTime = millis();
  }
  
  //CHECK DEBOUNCE TIMER
  if ((millis() - lastDebounceTime) > debounceDelay) {
    
    //CHECK TRIGGER AND COOLDOWN
    if (currentReading == LOW && millis() - lastToggle > toggleCooldown) {
      bleMode = !bleMode;
      lastToggle = millis();
      
      //TOGGLE BLE MODE
      if (bleMode) {
        Serial0.println("\n>>> BLE MODE ON <<<");
        initializeBLE();  // INITIALIZE BLE
        Serial0.println("Device should appear as HID Gamepad with D-pad");
      } else {
        Serial0.println("\n>>> BLE MODE OFF <<<");
        terminateBLE();  // TERMINATE BLE
      }
    }
  }
  
  lastState = currentReading;
  
  //CHECK BLE CONNECTION STATUS
  if (bleInitialized) {
    static bool lastConnected = false;
    if (bleGamepad.isConnected() != lastConnected) {
      lastConnected = bleGamepad.isConnected();
      if (lastConnected) {
        Serial0.println(">> BLE CONNECTED");
      } else {
        Serial0.println(">> BLE DISCONNECTED");
      }
    }
  }
  
  delay(20);
}