#include <Wire.h>
#include <bluefruit.h>
#include <Adafruit_TinyUSB.h>

// ---------- CardKB ----------
#define CARDKB_ADDR 0x5F

// ---------- Reed switch (NO): D2 -> reed -> GND ; LOW = docked ----------
const int REED_PIN = 2;
const unsigned long DEBOUNCE_MS = 50;

// ---------- BLE services ----------
BLEDis         bledis;
BLEHidAdafruit blehid;

// ---------- Connection handle for proper disconnect ----------
static uint16_t g_conn = BLE_CONN_HANDLE_INVALID;

// ---------- HID usage we use ----------
#define HID_KEY_ENTER        0x28
#define HID_KEY_ESCAPE       0x29
#define HID_KEY_BACKSPACE    0x2A
#define HID_KEY_TAB          0x2B
#define HID_KEY_ARROW_RIGHT  0x4F
#define HID_KEY_ARROW_LEFT   0x50
#define HID_KEY_ARROW_DOWN   0x51
#define HID_KEY_ARROW_UP     0x52

// Standard HID keycodes for letters, digits, etc.
#define HID_KEY_A            0x04
#define HID_KEY_Z            0x1D
#define HID_KEY_1            0x1E
#define HID_KEY_9            0x26
#define HID_KEY_0            0x27
#define HID_KEY_SPACE        0x2C
#define HID_KEY_MINUS        0x2D
#define HID_KEY_EQUAL        0x2E
#define HID_KEY_LEFT_BRACE   0x2F
#define HID_KEY_RIGHT_BRACE  0x30
#define HID_KEY_BACKSLASH    0x31
#define HID_KEY_SEMICOLON    0x33
#define HID_KEY_APOSTROPHE   0x34
#define HID_KEY_GRAVE        0x35
#define HID_KEY_COMMA        0x36
#define HID_KEY_PERIOD       0x37
#define HID_KEY_SLASH        0x38

// Modifier bits (USB HID)
#define KEYBOARD_MOD_LEFT_CTRL   0x01
#define KEYBOARD_MOD_LEFT_SHIFT  0x02
#define KEYBOARD_MOD_LEFT_ALT    0x04
#define KEYBOARD_MOD_LEFT_GUI    0x08
#define KEYBOARD_MOD_RIGHT_CTRL  0x10
#define KEYBOARD_MOD_RIGHT_SHIFT 0x20
#define KEYBOARD_MOD_RIGHT_ALT   0x40
#define KEYBOARD_MOD_RIGHT_GUI   0x80

// ---------- Reed debounce state ----------
int lastStable = HIGH; // assume undocked at boot
int lastRaw    = HIGH;
unsigned long edgeMs  = 0;

inline bool isDocked() { return lastStable == LOW; }

// ---------- TinyUSB USB HID (wired keyboard) ----------
Adafruit_USBD_HID usb_hid;

// Simple keyboard report descriptor
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_KEYBOARD()
};

// ---------- Helpers to map ASCII → HID (modifier + keycode) ----------
bool asciiToHid(uint8_t ascii, uint8_t &mod, uint8_t &key)
{
  mod = 0;
  key = 0;

  // Letters
  if (ascii >= 'a' && ascii <= 'z') {
    key = HID_KEY_A + (ascii - 'a');
    return true;
  }
  if (ascii >= 'A' && ascii <= 'Z') {
    key = HID_KEY_A + (ascii - 'A');
    mod = KEYBOARD_MOD_LEFT_SHIFT;
    return true;
  }

  // Digits and their shifted symbols
  switch (ascii) {
    case '1': key = HID_KEY_1; return true;
    case '!': key = HID_KEY_1; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '2': key = HID_KEY_1 + 1; return true;
    case '@': key = HID_KEY_1 + 1; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '3': key = HID_KEY_1 + 2; return true;
    case '#': key = HID_KEY_1 + 2; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '4': key = HID_KEY_1 + 3; return true;
    case '$': key = HID_KEY_1 + 3; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '5': key = HID_KEY_1 + 4; return true;
    case '%': key = HID_KEY_1 + 4; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '6': key = HID_KEY_1 + 5; return true;
    case '^': key = HID_KEY_1 + 5; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '7': key = HID_KEY_1 + 6; return true;
    case '&': key = HID_KEY_1 + 6; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '8': key = HID_KEY_1 + 7; return true;
    case '*': key = HID_KEY_1 + 7; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '9': key = HID_KEY_1 + 8; return true;
    case '(': key = HID_KEY_1 + 8; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '0': key = HID_KEY_0; return true;
    case ')': key = HID_KEY_0; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
  }

  // Space
  if (ascii == ' ') { key = HID_KEY_SPACE; return true; }

  // Punctuation
  switch (ascii) {
    case '-': key = HID_KEY_MINUS; return true;
    case '_': key = HID_KEY_MINUS; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '=': key = HID_KEY_EQUAL; return true;
    case '+': key = HID_KEY_EQUAL; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '[': key = HID_KEY_LEFT_BRACE; return true;
    case '{': key = HID_KEY_LEFT_BRACE; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case ']': key = HID_KEY_RIGHT_BRACE; return true;
    case '}': key = HID_KEY_RIGHT_BRACE; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '\\': key = HID_KEY_BACKSLASH; return true;
    case '|':  key = HID_KEY_BACKSLASH; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case ';': key = HID_KEY_SEMICOLON; return true;
    case ':': key = HID_KEY_SEMICOLON; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '\'': key = HID_KEY_APOSTROPHE; return true;
    case '\"': key = HID_KEY_APOSTROPHE; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '`': key = HID_KEY_GRAVE; return true;
    case '~': key = HID_KEY_GRAVE; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case ',': key = HID_KEY_COMMA; return true;
    case '<': key = HID_KEY_COMMA; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '.': key = HID_KEY_PERIOD; return true;
    case '>': key = HID_KEY_PERIOD; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
    case '/': key = HID_KEY_SLASH; return true;
    case '?': key = HID_KEY_SLASH; mod = KEYBOARD_MOD_LEFT_SHIFT; return true;
  }

  // Not mapped
  return false;
}

// ---------- Send a HID usage key to BLE + USB ----------
static void tapUsageKey(uint8_t hidcode, uint8_t modifier = 0)
{
  uint8_t kc[6] = { hidcode, 0,0,0,0,0 };

  // Prefer USB when it is ready, otherwise use BLE
  bool use_usb = usb_hid.ready();
  bool use_ble = Bluefruit.connected() && !use_usb;

  if (!use_ble && !use_usb) {
    // Nowhere to send, just bail
    return;
  }

  // Send press
  if (use_ble) {
    blehid.keyboardReport(modifier, kc);
  }
  if (use_usb) {
    usb_hid.keyboardReport(0, modifier, kc);
  }

  delay(8);

  // Release
  if (use_ble) {
    blehid.keyRelease();
  }
  if (use_usb) {
    usb_hid.keyboardRelease(0);
  }
}


// ---------- Send ASCII (mapped to HID) to BLE + USB ----------
static void tapAscii(uint8_t ascii)
{
  uint8_t mod = 0;
  uint8_t key = 0;
  if (!asciiToHid(ascii, mod, key)) {
    // Not mappable, ignore
    return;
  }
  tapUsageKey(key, mod);
}

// ---------- BLE advertising helpers ----------
static void advStart() {
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  Bluefruit.Advertising.addService(blehid);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(false);

  Bluefruit.Advertising.setInterval(32, 244); // ~20–152 ms
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

static void advStop() {
  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.restartOnDisconnect(false);
}

// ---------- Bluefruit callbacks ----------
void connect_callback(uint16_t conn_hdl) {
  g_conn = conn_hdl;
}

void disconnect_callback(uint16_t /*conn_hdl*/, uint8_t /*reason*/) {
  g_conn = BLE_CONN_HANDLE_INVALID;
}

// ---------- Reed polling / debounce ----------
void pollReed() {
  int raw = digitalRead(REED_PIN);
  unsigned long now = millis();
  if (raw != lastRaw) { lastRaw = raw; edgeMs = now; }

  if ((now - edgeMs) >= DEBOUNCE_MS && lastStable != raw) {
    lastStable = raw;

    if (lastStable == LOW) {
      // Docked → allow connect
      advStart();
    } else {
      // Undocked → drop link and go silent
      if (Bluefruit.connected() && g_conn != BLE_CONN_HANDLE_INVALID) {
        Bluefruit.disconnect(g_conn);
      }
      advStop();
    }
  }
}

// =================== Arduino lifecycle ===================
void setup() {
  // Reed
  pinMode(REED_PIN, INPUT_PULLUP);

  // I2C (XIAO: D4=SDA, D5=SCL)
  Wire.begin();
  Wire.setClock(100000);

  // ----- USB HID bring-up -----
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  // ----- BLE bring-up -----
  Bluefruit.begin();
  Bluefruit.setName("CardKB Dock KB");
  Bluefruit.setTxPower(4);

  bledis.setManufacturer("DIY");
  bledis.setModel("CardKB-Bridge");
  bledis.begin();

  blehid.begin();

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Start according to dock state
  lastStable = digitalRead(REED_PIN);
  lastRaw    = lastStable;
  edgeMs     = millis();
  if (isDocked()) advStart(); else advStop();
}

void loop() {
  pollReed();

  // USB HID runs automatically in background via TinyUSB; no special loop needed

  // Only read/send keys when at least one channel is usable:
  // (BLE connected OR USB ready)
  bool any_output = Bluefruit.connected() || usb_hid.ready();
  if (!any_output) {
    delay(2);
    return;
  }

  Wire.requestFrom(CARDKB_ADDR, 1);
  if (!Wire.available()) { delay(2); return; }

  uint8_t c = Wire.read();
  if (c == 0) { delay(2); return; }

  // Control keys from CardKB
  switch (c) {
    case 0x08: // Backspace
      tapUsageKey(HID_KEY_BACKSPACE);
      return;
    case 0x09: // Tab
      tapUsageKey(HID_KEY_TAB);
      return;
    case 0x0A: // LF
    case 0x0D: // CR
      tapUsageKey(HID_KEY_ENTER);
      return;
    case 0x1B: // ESC
      tapUsageKey(HID_KEY_ESCAPE);
      return;
  }

  // CardKB arrow codes
  if (c == 0xB4) { tapUsageKey(HID_KEY_ARROW_LEFT);  return; }
  if (c == 0xB7) { tapUsageKey(HID_KEY_ARROW_RIGHT); return; }
  if (c == 0xB5) { tapUsageKey(HID_KEY_ARROW_UP);    return; }
  if (c == 0xB6) { tapUsageKey(HID_KEY_ARROW_DOWN);  return; }

  // Printable ASCII
  if (c >= 32 && c <= 126) {
    tapAscii(c);
    return;
  }

  // Ignore others
  delay(2);
}