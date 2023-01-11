#include <CSWButtons.h>

////////////////////////////////////////////////////////////////////////
//Constants
////////////////////////////////////////////////////////////////////////

#define PIN_KEY 35
#define PWR_EN 5

#define SINGLE_CLICK 1
#define DOUBLE_CLICK 2

////////////////////////////////////////////////////////////////////////
//EOF Constants
///////////////////////////////////////////////////////////////////////

//init the button(s) of the smartwatch
swbtns::CSWButtons Buttons;

void onClk(int pin) {
  Serial.println("Button was clicked once!");
}

void onDblClk(int pin) {
  Serial.println("Button was clicked twice!");
}

void onLngPrs(int pin) {
  Serial.println("Button was longpressed!");
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Buttons test");
  pinMode(PWR_EN,OUTPUT);
  digitalWrite(PWR_EN,HIGH);
  Buttons.addButton(PIN_KEY);
  Buttons.attachInterrupts();
  Buttons.onClick(PIN_KEY,onDblClk,DOUBLE_CLICK);
  Buttons.onClick(PIN_KEY,onClk,SINGLE_CLICK);
  Buttons.onLongpress(PIN_KEY, onLngPrs);
  Serial.println("Setup done.");
}
void loop() {
  Buttons.tickTimer();
}
