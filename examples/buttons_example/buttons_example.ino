using namespace std;
#define DEBUG 1
#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <CSWButtons.h>


////////////////////////////////////////////////////////////////////////
//Constants
////////////////////////////////////////////////////////////////////////

#define LOAD_GFXFF 1
#define PIN_MOTOR 4
#define PIN_KEY 35
#define PWR_EN 5
// #define Backlight 33

#define SPI_SCK 14
#define SPI_DIN 13
#define EPD_CS 15
#define EPD_DC 2
#define SRAM_CS -1
#define EPD_RESET 17
#define EPD_BUSY 16

#define ROTATION_DIR 0

#define BUTTON_RECHECK_INTERVAL_MS 800
#define BUTTON_RECHECK_INTERVAL_LONGPRESS_MS 100

#define POWER_OFF_CLICKS 3

////////////////////////////////////////////////////////////////////////
//EOF Constants
////////////////////////////////////////////////////////////////////////

int EPD_dispIndex; // The index of the e-Paper's type
uint8_t EPD_Image[5000] = {0};
uint16_t EPD_Image_count = 0;

GxIO_Class io(SPI, /*CS=*/ 15, /*DC=*/2, /*RST=*/17);
GxEPD_Class display(io, /*RST=*/17, /*BUSY=*/16);

//init the button(s) of the smartwatch
swbtns::CSWButtons Buttons;

void powerDownSys() {
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);
  digitalWrite(PWR_EN,LOW);
  // attempt to make the vibration motor NOT to vibrate when hibernated and on a USB power.
  pinMode(PIN_MOTOR, INPUT);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);//!< RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);//!< RTC slow memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);//!< RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);//!< XTAL oscillator
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_start();
}

void shutdownSystem() {
  #if defined(DEBUG) && DEBUG>=1
  Serial.println("Shutting down system...");
  #endif
  display.setRotation(ROTATION_DIR);
  display.setTextColor(GxEPD_BLACK);
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);  
  display.fillScreen(GxEPD_BLACK);
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);
  #if defined(DEBUG) && DEBUG>=1
  Serial.println("Power off.");
  #endif
  powerDownSys();
}

void onPwrOff(int pin) {
  shutdownSystem();
}

void showFullscreenCleanupCallback(uint32_t v) {
  uint16_t box_x = 0;
  uint16_t box_y = 0;
  uint16_t box_w = GxEPD_WIDTH;
  uint16_t box_h = GxEPD_HEIGHT;
  display.fillRect(box_x, box_y, box_w, box_h, v);
  display.updateWindow(box_x, box_y, box_w, box_h, true);
}

void setup() {
  // put your setup code here, to run once:
  #if defined(DEBUG) && DEBUG>=1
  Serial.begin(115200);
  delay(10);
  Serial.println("Smartwatch is in DEBUG mode. Starting system...");
  Serial.print("Debug level: ");
  Serial.println(DEBUG);
  #endif
  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  pinMode(PWR_EN,OUTPUT);
  display.init();
  display.update();
  display.setRotation(ROTATION_DIR);
  display.setTextColor(GxEPD_BLACK);
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
  display.fillScreen(GxEPD_BLACK);
  display.setTextSize(1);
  display.setCursor(30, GxEPD_HEIGHT/2+8);
  display.print("Loading");
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, true);
  display.setTextSize(1);
  //init the one and only programmable button.
  pinMode(PIN_KEY, INPUT);
  Buttons.addButton(PIN_KEY);
  Buttons.attachInterrupts();
  Buttons.onClick(PIN_KEY,onPwrOff,POWER_OFF_CLICKS);
  #if defined(DEBUG) && DEBUG>=1
  Serial.println("Setup done.");
  #endif
  display.drawPagedToWindow(showFullscreenCleanupCallback, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_WHITE);
}
void loop() {
  Buttons.tickTimer();
}
