#include <M5Stack.h>

/*
 * this code is based on https://github.com/nkolban/esp32-snippets/blob/9112aebed4ef86cfccccfdbf3aedf8fe44ec08e4/cpp_utils/tests/BLETests/SampleHIDKeyboard.cpp
 */

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDKeyboardTypes.h"
#include <esp_log.h>
#include <string>
#include <vector>
#include "Task.h"
#include "sdkconfig.h"

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#endif

static char LOG_TAG[] = "SampleHIDDevice";

static BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;
bool isConnected = false;


extern void DisplayConnectionText();
extern void DisplayStatusText(const char *text);
extern void DisplayGuide();

/*
 * This callback is connect with output report. In keyboard output report report special keys changes, like CAPSLOCK, NUMLOCK
 * We can add digital pins with LED to show status
 * bit 1 - NUM LOCK
 * bit 2 - CAPS LOCK
 * bit 3 - SCROLL LOCK
 */
class MyOutputCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* me){
    uint8_t* value = (uint8_t*)(me->getValue().c_str());
    ESP_LOGI(LOG_TAG, "special keys: %d", *value);
  }
};

class MyTask : public Task {
  private:
  KEYMAP payload[256];
  int length = 0;
  
  public:
  MyTask(const char *text) {
    this->setString(text);
  }
  
  MyTask(const KEYMAP *payload, int length) {
    this->setKeys(payload, length);
  }

  MyTask() {
    this->length = 0;
  }

  void setString(const char *text) {
    this->length = 0;
    const char *pointer = text;
    while(*pointer){
        KEYMAP map = keymap[(uint8_t)*pointer];
        this->payload[this->length++] = map;
        pointer++;
    }
  }
  
  void setKeys(const KEYMAP *payload, int length) {
    int realLen = min(256, length);
    for (int i=0 ; i<realLen ; i++) {
      this->payload[i] = payload[i];
    }
    this->length = realLen;
  }

  void deleteMe() {
    vTaskDelete(NULL);
  }
  
  private:
  void run(void*){
      DisplayStatusText("sending keys.");
      for(int i=0 ; i<this->length ; i++) {
        KEYMAP map = this->payload[i];
        /*
         * simulate keydown, we can send up to 6 keys
         */
        uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
        input->setValue(a,sizeof(a));
        input->notify();

        /*
         * simulate keyup
         */
        uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
        input->setValue(v, sizeof(v));
        input->notify();

        vTaskDelay(100/portTICK_PERIOD_MS);
      }
      DisplayStatusText("sent keys.");
  }
};

std::vector<MyTask *> tasks;

void DisplayConnectionText() {
  int yStart = 0;
  int height = 50;
  M5.Lcd.setCursor(0, yStart);
  const char *text = isConnected ? "Connected" : "Not connected.";
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.printf(text);
}

void DisplayStatusText(const char *text) {
  int yStart = 100;
  int height = 50;
  M5.Lcd.setCursor(0, yStart);
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.printf(text);
}

void DisplayGuide() {
  int yStart = 200;
  M5.Lcd.setCursor(0, yStart);
  M5.Lcd.printf("[HELLO]");
  M5.Lcd.setCursor(90, yStart);
  M5.Lcd.printf("[CTRL+ALT+DEL]");
}

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    isConnected = true;
    DisplayConnectionText();
  }

  void onDisconnect(BLEServer* pServer){
    isConnected = false;
    DisplayConnectionText();

    for(int i=0 ; i<tasks.size(); i++) {
      MyTask *task = tasks[i];
      task->stop();
      task->deleteMe();
      //delete task;
    }
    tasks.clear();
  }
};

class MainBLEServer: public Task {
   BLEServer *pServer;

public:
   BLEServer *getServer() {
    return pServer;
   }
  
  void run(void *data) {
    ESP_LOGD(LOG_TAG, "Starting BLE work!");

    BLEDevice::init("M5Stack-HID");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyCallbacks());

    /*
     * Instantiate hid device
     */
    hid = new BLEHIDDevice(pServer);


    input = hid->inputReport(1); // <-- input REPORTID from report map
    output = hid->outputReport(1); // <-- output REPORTID from report map

    output->setCallbacks(new MyOutputCallbacks());

    /*
     * Set manufacturer name (OPTIONAL)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
     */
    std::string name = "esp-community";
    hid->manufacturer()->setValue(name);

    /*
     * Set pnp parameters (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
     */

    hid->pnp(0x02, 0xe502, 0xa111, 0x0210);

    /*
     * Set hid informations (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
     */
    hid->hidInfo(0x00,0x01);


    /*
     * Keyboard
     */
    const uint8_t reportMap[] = {
      USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
      USAGE(1),           0x06,       // Keyboard
      COLLECTION(1),      0x01,       // Application
      REPORT_ID(1),   0x01,   // REPORTID
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
      REPORT_COUNT(1),    0x08,
      HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
      REPORT_SIZE(1),     0x08,
      HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
      REPORT_SIZE(1),     0x01,
      USAGE_PAGE(1),      0x08,       //   LEDs
      USAGE_MINIMUM(1),   0x01,       //   Num Lock
      USAGE_MAXIMUM(1),   0x05,       //   Kana
      HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
      REPORT_SIZE(1),     0x03,
      HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
      REPORT_SIZE(1),     0x08,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      END_COLLECTION(0)
    };
    /*
     * Set report map (here is initialized device driver on client side) (MANDATORY)
     * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
     */
    hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

    /*
     * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
     * Also before we start, if we want to provide battery info, we need to prepare battery service.
     * We can setup characteristics authorization
     */
    hid->startServices();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

    ESP_LOGD(LOG_TAG, "Wait a bit ...");
    delay(5000);

    /*
     * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
     */
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    ESP_LOGD(LOG_TAG, "Advertising started!");
    delay(1000000);
  }
};

MainBLEServer* pMainBleServer = NULL;

void StartBLEServer(void)
{
  esp_log_level_set("*", ESP_LOG_DEBUG);
  pMainBleServer = new MainBLEServer();
  pMainBleServer->setStackSize(20000);
  pMainBleServer->start();

}

void setup() {
  M5.begin();                   // M5STACK INITIALIZE
  
  esp_log_level_set("*", ESP_LOG_DEBUG);        // set all components to ERROR level
  //esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
  log_e("HELLO_ERR");

  Serial.begin(115200);         // SERIAL
  
  M5.Lcd.setBrightness(200);    // BRIGHTNESS = MAX 255
  M5.Lcd.fillScreen(BLACK);     // CLEAR SCREEN
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setRotation(0);        // SCREEN ROTATION = 0

  DisplayStatusText("Initializing...");
  DisplayConnectionText();
  DisplayGuide();
  
  // put your setup code here, to run once:
  StartBLEServer();
  DisplayStatusText("Initialized.");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(isConnected && M5.BtnA.wasPressed()) {
    MyTask *task = new MyTask("Hello From M5Stack!\n");
    task->start();
    tasks.push_back(task);
  }

  if(isConnected && M5.BtnB.wasPressed()) {
    KEYMAP payload[1];
    payload[0] = {0x4c, KEY_CTRL | KEY_ALT}; // CTRL+ALT+DEL !!!!
    MyTask *task = new MyTask(payload, 1);
    task->start();
    tasks.push_back(task);
  }
  
  if (M5.BtnC.wasPressed()) {
  }
  M5.update();
}


