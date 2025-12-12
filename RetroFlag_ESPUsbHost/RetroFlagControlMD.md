# **RetroFlag Controller Bridge (ESP32-S3)**

This project allows you to use a wired RetroFlag (or other USB HID) controller as a Bluetooth gamepad using an ESP32-S3. It acts as a USB Host to read the controller input and then broadcasts it as a BLE Gamepad.

## **Features**

* **USB Host:** Reads standard HID reports from USB controllers.  
* **BLE Gamepad:** Emulates a standard Bluetooth gamepad (compatible with Android/iOS/Windows/Linux/macOS).  
* **Hybrid Mode:** Simultaneously outputs debug information to Serial and gamepad reports to Bluetooth.  
* **Mode Switching:** Use a reed switch (or boot button) to toggle Bluetooth on/off at runtime.  
* **Hot-Swappable:** Automatically detects controller connection/disconnection.  
* **Battery Friendly:** Completely shuts down the BLE stack when not in use.

## **Hardware Requirements**

* **ESP32-S3 Board:** (e.g., YD-ESP32-S3, DevKitC-1). Must support USB-OTG.  
* **USB OTG Adapter:** USB-C to USB-A adapter to connect the controller to the ESP32's native USB port.  
* **RetroFlag Controller:** (or any standard USB HID gamepad).  
* **(Optional) Reed Switch:** Connected to GPIO 17 and GND for mode toggling.

## **Pin Configuration**

| Component | Pin | Description |
| :---- | :---- | :---- |
| **Reed Switch** | GPIO 17 | Pull to GND to toggle Bluetooth Mode. |
| **Boot Button** | GPIO 0 | Built-in button, also toggles Bluetooth Mode. |
| **USB D-** | GPIO 19 | Native USB Data- (White wire). |
| **USB D+** | GPIO 20 | Native USB Data+ (Green wire). |

**Note:** If using a dev board with two USB-C ports, plug the controller into the port labeled "USB" or "OTG". Use the "UART" or "COM" port for uploading code.

## **Software Dependencies**

This project relies on the following libraries (install via Arduino Library Manager):

1. **NimBLE-Arduino** by h2zero (v1.4.0 or newer) \- *High performance Bluetooth Low Energy stack.*  
2. **ESP32-BLE-Gamepad** by lemmingDev (v0.5.2 or newer) \- *Simplifies gamepad emulation.*

## **Setup Instructions**

1. **Install Libraries:** Open Arduino IDE \-\> Sketch \-\> Include Library \-\> Manage Libraries. Search for and install the two dependencies listed above.  
2. **Board Settings:**  
   * **Board:** ESP32S3 Dev Module  
   * **USB CDC On Boot:** Disabled (Crucial\! Forces Serial to UART0).  
   * **USB Mode:** USB-OTG (TinyUSB)  
3. **Upload:** Connect ESP32 to PC via UART port and upload the sketch.  
4. **Wiring:** Connect your USB controller to the ESP32's USB-OTG port.

## **Usage**

1. **Power On:** The ESP32 starts in "USB Debug Mode" (BLE is OFF).  
2. **Verify Input:** Open the Serial Monitor (115200 baud). Press buttons on your controller. You should see raw hex input and button names (e.g., Input: 00 14 \-\> UP A).  
3. **Enable Bluetooth:** Swipe a magnet near the reed switch or press the BOOT button.  
   * The console will print \>\>\> BLE MODE ON \<\<\<.  
4. **Connect:** On your phone/PC, scan for Bluetooth devices. Connect to **"Retro-Joystick"**.  
5. **Play:** Your controller inputs are now sent wirelessly to your device\!

## **Troubleshooting**

* **Crash on Connect:** Ensure you are using the NimBLE-Arduino library, not the standard BLEDevice.  
* **No Input:** Check wiring (Green=D+, White=D-). Ensure controller is in D-Input mode (Hold 'B' while plugging in for RetroFlag controllers).  
* **"Device not found":** If you previously connected, "Forget" the device on your phone and re-scan.

## **File Structure**

* RetroFlagControl.ino: Main application logic, state machine, and mapping.  
* EspUsbHost.cpp: Low-level USB Host driver implementation.  
* EspUsbHost.h: USB Host driver definitions.