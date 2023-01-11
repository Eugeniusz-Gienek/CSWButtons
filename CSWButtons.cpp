/**
  ******************************************************************************
  * @file    CSWbuttons.cpp
  * @author  Eugene at sky.community
  * @version V1.0.0
  * @date    12-December-2022
  * @brief   The functionality to work with the buttons of the smartwatch based on esp32.
  *
  ******************************************************************************
  */

#include "CSWButtons.h"
using namespace swbtns;
#include <Arduino.h>
#include <vector>
#include <deque>
#include <string>
#include <sstream>

#define DEBUG 0

typedef void (*VoidFunctionWithNoParameters) (void);
typedef void (*VoidFunctionWithOneParameter) (int);

std::vector<uint8_t> btnPins;

int CSWButtons::button_click_flow_limit=5;
int CSWButtons::button_recheck_interval_longpress_ms=500;
int CSWButtons::button_recheck_interval_ms=1000;

template<typename ValueType>
std::string stringulate(ValueType v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}
typedef struct Record
{
      int pin;
      int indx_num=-1;
      std::string name;
}_btn_struct;

typedef struct Record2
{
      int pin;
      int click_count=1;
      VoidFunctionWithOneParameter event_function;
      std::string name;
}_click_event;

class SWbtns
{
  private:
  std::vector<_click_event> clickEvents;
  std::vector<_btn_struct> stack;
  bool _eventsBlocked=false;
  //int button_click_flow_limit=5;

  t_buttonsStack buttonsClickStack;
  // alt click stack is used for the situations when during the processing of the stack
  // there are click events happening and they are processed by the CPU and the code is
  // executed. For the same reasons the blockers below are introduced.
  // BTW we block the selected stack for ALL buttons at once at this moment - thus
  // the blocker vars are just bool's and not having IDs inside them.
  t_buttonsStack buttonsClickStackAlt;
  bool buttonsClickStackLocked=false;
  bool buttonsClickStackAltLocked=false;
  int getClickStackIndex(int pin, bool is_alt=false);

  public:
  bool checkEventsBlocked() {
    return _eventsBlocked;
  }
  void setEventsBlocked(bool v) {
    _eventsBlocked=v;
  }
  int CLICK_index(std::string ev_name)
  {
    for(int i=0;i<clickEvents.size();++i)
    {
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("CSWBUTTONS: Verifying ONCLICK event '");
      Serial.print(ev_name.c_str());
      Serial.print("'. Check for: ");
      Serial.println(clickEvents[i].name.c_str());
      #endif
      if(clickEvents[i].name==ev_name)
        return i;
    }
    return -1;
  }
  VoidFunctionWithOneParameter getOnclickFunction(int pin, int click_count=1) {
    std::string ev_name;
    std::string ev_spin;
    std::string ev_clk;
    std::string _ev_spin;
    std::string _ev_spin2;
    ev_spin = stringulate(pin);
    ev_clk = stringulate(click_count);
    _ev_spin = "ev_";
    _ev_spin2 = "_";
    ev_name = _ev_spin + ev_clk +_ev_spin2 + ev_spin;
    int indx = this->CLICK_index(ev_name);
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("CSWBUTTONS: NAME of CLICK function: ");
    Serial.print(ev_name.c_str());
    Serial.print("; INDEX of CLICK function: ");
    Serial.println(indx);
    #endif
    if(indx != -1) return clickEvents[indx].event_function;
    else return __null;
  }

  void execOnclickFunction(int pin, int click_count=1) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Executing onclick function....");
    #endif
    VoidFunctionWithOneParameter f = this->getOnclickFunction(pin, click_count);
    if(f) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Function found. Execution!");
      #endif
      f(pin);
    } else {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: No onclick function attached!");
      #endif
    }
  }
  
  VoidFunctionWithOneParameter getOnlongpressFunction(int pin) {
    int click_count=1;
    std::string ev_name;
    std::string ev_spin;
    std::string ev_clk;
    std::string _ev_spin;
    std::string _ev_spin2;
    ev_spin = stringulate(pin);
    ev_clk = stringulate(click_count);
    _ev_spin = "ev_";
    _ev_spin2 = "_long_";
    ev_name = _ev_spin + ev_clk +_ev_spin2 + ev_spin;
    int indx = this->CLICK_index(ev_name);
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("CSWBUTTONS: NAME of longpress function: ");
    Serial.print(ev_name.c_str());
    Serial.print("; INDEX of longpress function: ");
    Serial.println(indx);
    #endif
    if(indx != -1) return clickEvents[indx].event_function;
    else return __null;
  }

  void execOnlongpressFunction(int pin, int click_length=-1) {
    VoidFunctionWithOneParameter f = this->getOnlongpressFunction(pin);
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Execution of the longpress function...");
    #endif
    if(f) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Function found. Executing!");
      #endif
      #if defined(DEBUG) && DEBUG>=1
      Serial.print("CSWBUTTONS: Longpress click duration(ms): ");
      Serial.println(click_length);
      #endif
      f(pin);
    } else {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: No function for longpress!");
      #endif
    }
  }
  
  int getPinByNum(int pinNum) {
    return stack[pinNum].pin;
  }

  void onclick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count=1)
  {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Adding onclick function!");
    #endif
    std::string ev_name;
    std::string ev_spin;
    std::string ev_clk;
    std::string _ev_spin;
    std::string _ev_spin2;
    ev_spin = stringulate(pin);
    ev_clk = stringulate(click_count);
    _ev_spin = "ev_";
    _ev_spin2 = "_";
    ev_name = _ev_spin + ev_clk +_ev_spin2 + ev_spin;
    int ev_index=CLICK_index(ev_name);
    if(ev_index>=0)
    {
      //Replace!
      clickEvents[ev_index].event_function=onclick_function;
    }
    else
    {
       _click_event _ev;
      _ev.pin=pin;
      _ev.name=ev_name;
      _ev.click_count=click_count;
      _ev.event_function=onclick_function;
      clickEvents.push_back(_ev);
    }
  }

  void onlongpress(int pin, VoidFunctionWithOneParameter onclick_function)
  {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Adding onlongpress function!");
    #endif
    int click_count=1;
    std::string ev_name;
    std::string ev_spin;
    std::string ev_clk;
    std::string _ev_spin;
    std::string _ev_spin2;
    ev_spin = stringulate(pin);
    ev_clk = stringulate(click_count);
    _ev_spin = "ev_";
    _ev_spin2 = "_long_";
    ev_name = _ev_spin + ev_clk +_ev_spin2 + ev_spin;
    int ev_index=CLICK_index(ev_name);
    if(ev_index>=0)
    {
      //Replace!
      clickEvents[ev_index].event_function=onclick_function;
    }
    else
    {
       _click_event _ev;
      _ev.pin=pin;
      _ev.name=ev_name;
      _ev.click_count=click_count;
      _ev.event_function=onclick_function;
      clickEvents.push_back(_ev);
    }
  }
  int BTN_index(std::string btn_name)
  {
    for(int i=0;i<stack.size();++i)
    {
      if(stack[i].name==btn_name)
        return i;
    }
    return -1;
  }
  void Add_btn(int pin,int indx_num=-1)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0)
    {
      //Whoopsie. Ignore?
    }
    else
    {
       _btn_struct _btn;
      _btn.pin=pin;
      _btn.name=btn_name;
      _btn.indx_num=indx_num;
      stack.push_back(_btn);
    }
  }
  
  const static int CLICK=0;
  const static int UNCLICK=1;
  void addEventToClickStack(int pin,int click_type=CLICK, int ts=-1);//click_type: 0-click, 1-unclick; ts = timestamp, optional
  bool checkClickStackDone(int pin, bool is_alt=false);
  void clearClickStack(int pin, bool is_alt=false);
  void clearAllClickStacks();
  void swapClickStacks(int pin);
  void setButtonStackFimit(int l);
  int getButtonStackFimit(void);
  void processStack(void);
  
  _btn_struct operator [] (int pin)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    for(int i=0; i<stack.size(); i++)
    {
      if(stack[i].name==btn_name)
        return stack[i];
    }
    long index = stack.size();
    _btn_struct _btn;
    _btn.pin=pin;
    _btn.name=btn_name;
    stack.push_back(_btn);
    return stack[index];
  }
}; //EOF class SWbtns

SWbtns btns;

/// @brief The button which has to be called by a "tick" function when processing click stack. So it works with the finished stack and executes onclick/onlongpress events.
/// @param pin 
/// @param _click_count 
/// @param longPress 
/// @return 
bool _callbackMulticlick(int pin, int _click_count,bool longPress=false,int click_length=-1) {
  int click_count=_click_count;
  if(longPress) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.print("CSWBUTTONS: MULTICLICK. LONGPRESS. PIN: ");
    Serial.println(pin);
    #endif
    btns.execOnlongpressFunction(pin, click_length);
    return true;
  } else {
    #if defined(DEBUG) && DEBUG>=1
    Serial.print("CSWBUTTONS: MULTICLICK. Click amount: ");
    Serial.print(click_count);
    Serial.print("; PIN: ");
    Serial.println(pin);
    #endif
    btns.execOnclickFunction(pin, click_count);
    return false;
  }
  return false;
}

/// @brief System-called function which is called when a click event was generated
/// @param pinNum 
void handleInterrupt(int pinNum) {
  int pin=btns.getPinByNum(pinNum);
  if(btns.checkEventsBlocked()) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.println("CSWBUTTONS: BUTTONS: Blocking onclick!");
    #endif
    return;
  }
  uint8_t eventType = 0; //0 - pressed, 1 - unpressed
  uint8_t buttonState = digitalRead(pin);
  eventType = (buttonState == LOW) ? 0 : 1;
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: Calling the addEventToClickStack.");
  Serial.print("CSWBUTTONS: event type:");
  Serial.println(eventType);
  delay(500);
  #endif
  btns.addEventToClickStack(pin,eventType);
}
unsigned long lastPressedTime = millis();
#define PIN_HANDLER(pin) \
void pin_handler_##pin (void) \
{ \
  unsigned long pressedTime = millis();\
  if(pressedTime-lastPressedTime >= 50) {lastPressedTime=pressedTime;handleInterrupt(pin);} \
}

//////////////////////////////////////////////////////////////////////////////
/* The config of buttons lies actually here */

#define PIN_HANDLER(pin) \
extern void pin_handler_##pin (void) \
{ \
  unsigned long pressedTime = millis();\
  if(pressedTime-lastPressedTime >= 100) {lastPressedTime=pressedTime;handleInterrupt(pin);} \
}

/* YES I KNOW!!!!!!!!!!!!!
 * this is UGLY!!!
 * However if you know better how to make this preprocessor bullsh*t - plz let me know. For now it will live here.
 * I assume the amount of buttons on a SmartWatch should not extend 10 - so let it be 10 in here.
 * Well, someone may put a full-sized QWERTY keyboard on a smartwatch. Please add the necessary amount of buttons here hehe
*/
const int pinKey1 = 0;
const int pinKey2 = 1;
const int pinKey3 = 2;
const int pinKey4 = 3;
const int pinKey5 = 4;
const int pinKey6 = 5;
const int pinKey7 = 6;
const int pinKey8 = 7;
const int pinKey9 = 8;
const int pinKey10 = 9;
PIN_HANDLER (pinKey1)
PIN_HANDLER (pinKey2)
PIN_HANDLER (pinKey3)
PIN_HANDLER (pinKey4)
PIN_HANDLER (pinKey5)
PIN_HANDLER (pinKey6)
PIN_HANDLER (pinKey7)
PIN_HANDLER (pinKey8)
PIN_HANDLER (pinKey9)
PIN_HANDLER (pinKey10)
/*
 * End of bullsh*t...
*/

VoidFunctionWithNoParameters intrp_functions[] = {pin_handler_pinKey1,pin_handler_pinKey2,pin_handler_pinKey3,pin_handler_pinKey4,pin_handler_pinKey5,pin_handler_pinKey6,pin_handler_pinKey7,pin_handler_pinKey8,pin_handler_pinKey9,pin_handler_pinKey10};

/*
 * Oh, here we go - real end of bullsh*t
*/

//////////////////////////////////////////////////////////////////////////////

//the CSWButtons class functions bodies lie here.

CSWButtons::CSWButtons() {
}

void CSWButtons::addButton(int pin) {
  btnPins.push_back(pin);
}

void CSWButtons::onClick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count) {
  btns.onclick(pin, onclick_function, click_count);
}

void CSWButtons::onLongpress(int pin, VoidFunctionWithOneParameter onclick_function) {
  btns.onlongpress(pin, onclick_function);
}

bool CSWButtons::checkEventsBlocked() {
  return _firstRun || _eventsBlocked;
}
void CSWButtons::setEventsBlocked(bool v) {
  _eventsBlocked = v;
  btns.setEventsBlocked(v);
}

void CSWButtons::setButtonClickFlowFimit(int l) {
  CSWButtons::button_click_flow_limit=l;
}

void CSWButtons::setButtonLongpressIntervalms(int i) {
  CSWButtons::button_recheck_interval_longpress_ms=i;
}

void CSWButtons::setButtonRecheckIntervalms(int i) {
  CSWButtons::button_recheck_interval_ms=i;
}


/// @brief This has to be called after all of the buttons are added to the object. It attaches the necessary system interrupts so the click events will work. It should NOT be called more than once!
void CSWButtons::attachInterrupts() {
  _firstRun=true;
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: First run set.");
  #endif
  btns.setEventsBlocked(true);
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: Events blocked set. Going to pinmodes...");
  Serial.print("CSWBUTTONS: Buttons numer is: ");
  Serial.println(btnPins.size());
  #endif
  for(int i=0;i<btnPins.size();i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("CSWBUTTONS: Pinmode for the pin ");
    Serial.print(btnPins[i]);
    Serial.print(" with nr ");
    Serial.println(i);
    Serial.println("CSWBUTTONS:  is set as PULLUP. Attaching interrupt...");
    #endif
    attachInterrupt (btnPins[i], intrp_functions[i], CHANGE);
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Interrupt attached. Adding button...");
    #endif
    btns.Add_btn(btnPins[i],i);
  }
  btns.setEventsBlocked(false);
}
void CSWButtons::tickTimer() {
  btns.processStack();
}

/// @brief Add click/unclick event to the stack which will be processed during next tick
/// @param pin 
/// @param click_type 
/// @param ts 
void SWbtns::addEventToClickStack(int pin,int click_type, int ts) {
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("CSWBUTTONS: Adding event to click stack for PIN: ");
  Serial.println(pin);
  #endif
  int indx_alt = this->getClickStackIndex(pin, true);
  if(this->checkEventsBlocked()) return;
  bool is_alt=buttonsClickStackLocked;
  //if events are currently being processed for the current stack type (main or alt)
  bool ckst = this->checkClickStackDone(pin, is_alt); //optimization - less calls
  if(!is_alt && buttonsClickStackLocked) return;
  if(is_alt && buttonsClickStackAltLocked) return;
  if((!is_alt) && ckst) {
    // We expect that the main buffer will be processed in next "tick" iteration.
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: the main buffer will be processed in next 'tick' iteration");
    #endif
    return;
  }
  //this behaviour is just for alt buffer
  if(is_alt && ckst) {
    if(indx_alt != -1) {
      //well, smth HAS to be in alt. Otherwise just add.
      //alt buffer overflow. We have to use stack approach or clear it due to timing;
      if(buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size()>= this->getButtonStackFimit()) {
        //stack - so just remove the first element;
        buttonsClickStackAlt[indx_alt].buttonClickStackEvents.pop_front();
      } else {
        //it'll be empty
        this->clearClickStack(pin, true);
      }
    }
  }
  // At this point the stack is NOT done (two previous "if's" are responsible for that).
  // So we can add to the current stack the event
  int currTime = millis();
  t_buttonsStack * buttons_stack = &buttonsClickStack;
  if(is_alt) buttons_stack = &buttonsClickStackAlt;
  int btn_s_indx = this->getClickStackIndex(pin, is_alt);
  if(btn_s_indx == -1) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("CSWBUTTONS: Creating the new events stack for pin");
    Serial.print(pin);
    Serial.println(is_alt ? " - alternative one." : ".");
    #endif
    //we have to create it!
    buttonEventsStack newButtonEventsStack = {};
    newButtonEventsStack.PIN = pin;
    newButtonEventsStack.buttonClickStackEvents = {};
    if(is_alt)
      buttonsClickStackAlt.push_back(newButtonEventsStack);
    else
      buttonsClickStack.push_back(newButtonEventsStack);
    btn_s_indx = 0;
  }
  t_buttonClickStackEvents stack = (*buttons_stack)[btn_s_indx].buttonClickStackEvents;
  if((stack.size() == 0) || (stack[stack.size()-1].is_complete)) {
    if(click_type != CLICK) return;
    #if defined(DEBUG) && DEBUG>=1
    Serial.print("CSWBUTTONS: Adding first ");
    Serial.print((click_type == CLICK) ? "click" : "unclick");
    Serial.print(" event to ");
    Serial.print((stack.size() == 0) ? "empty" : "complete");
    Serial.println(" stack.");
    #endif
    //add click/unclick element to stack ("bcse")
    buttonClickStackEvent bcse;
    if(click_type == CLICK) {
      bcse.time_pressed = currTime;
    } 
    (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
  } else {
    // Add the click/unclick event to queue. Stack size is more than zero.
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Adding next event to stack.");
    #endif
    if(click_type == CLICK) {
      #if defined(DEBUG) && DEBUG>=1
      Serial.println("CSWBUTTONS: Click event detected.");
      #endif
      if(stack[stack.size()-1].time_pressed == -1) {
        #if defined(DEBUG) && DEBUG>=1
        Serial.println("CSWBUTTONS: We are updating the previously added event without click time.");
        #endif
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed = currTime;
      } else {
        // ...and add a new event record
        buttonClickStackEvent bcse;
        bcse.time_pressed = currTime;
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
        return;
      }
    } else {
      //UNCLICK
      #if defined(DEBUG) && DEBUG>=1
      Serial.println("CSWBUTTONS: Unclick event detected.");
      #endif
      if((stack.size() > 0) && ((*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed != -1)) {
        //it's actually not an unpress but very quick press. YES, that's how the ESP32 works.
        buttonClickStackEvent bcse;
        bcse.time_pressed = currTime;
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
      }
      if(stack.size() == 1) {
        #if defined(DEBUG) && DEBUG>=1
        Serial.println("CSWBUTTONS: Potential longpress.");
        #endif
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed = currTime;
        //(*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].is_complete = true;
      }
    }
  }
}

/// @brief this function verifies if the buffer is ready to be processed. Usually called by the "tick" event
/// @param pin 
/// @param is_alt 
/// @return 
bool SWbtns::checkClickStackDone(int pin, bool is_alt) {
  int btn_s_indx = this->getClickStackIndex(pin, is_alt);
  t_buttonsStack * buttons_stack = &buttonsClickStack;
  if(is_alt) buttons_stack = &buttonsClickStackAlt;
  if(btn_s_indx == -1) return false;
  t_buttonClickStackEvents stack = (*buttons_stack)[btn_s_indx].buttonClickStackEvents;
  int ssz = stack.size();
  //check for reaching the stack limit
  if(ssz >= this->getButtonStackFimit()) return true;
  if(ssz == 0) return false;
  int currTime = millis();
  
  //mark the LAST complete item, as we don't care about all of them actually
  if(ssz>0) {
    int ii = ssz - 1;
    if(
      (currTime - stack[ii].time_pressed > CSWButtons::button_recheck_interval_ms)
    )
    {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Marking correct events as complete.");
      #endif
      (*buttons_stack)[btn_s_indx].buttonClickStackEvents[ii].is_complete=true;
    }
  }
  //check for reaching the time limit
  if(
    stack[ssz-1].is_complete
  ) return true;
  return false;
}

/// @brief Clears the stack - main or alternative, for the defined pin
/// @param pin 
/// @param is_alt 
void SWbtns::clearClickStack(int pin, bool is_alt) {
  if(is_alt) {
    int indx_alt = this->getClickStackIndex(pin, true);
    if(indx_alt != -1) buttonsClickStackAlt[indx_alt].buttonClickStackEvents.clear();
  } else {
    int indx = this->getClickStackIndex(pin, false);
    if(indx != -1) buttonsClickStack[indx].buttonClickStackEvents.clear();
  }
}

/// @brief Clears ALL of the stacks for ALL of the pins
void SWbtns::clearAllClickStacks() {
  buttonsClickStack.clear();
}

/// @brief Clears the main stack, copies the alternative to it and clears the alterative one.
/// @param pin 
void SWbtns::swapClickStacks(int pin) {
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("CSWBUTTONS: Performing SWAP for the pin: ");
  Serial.println(pin);
  #endif
  int indx = this->getClickStackIndex(pin, false);
  int indx_alt = this->getClickStackIndex(pin, true);
  if((indx_alt != -1) && (buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size() > 0)) {
    if(indx == -1) {
      //we have to create it!
      buttonEventsStack newButtonEventsStack = {};
      newButtonEventsStack.PIN = pin;
      newButtonEventsStack.buttonClickStackEvents = {};
      buttonsClickStack.push_back(newButtonEventsStack);
      indx = 0;
      buttonsClickStack[indx].buttonClickStackEvents = buttonsClickStackAlt[indx_alt].buttonClickStackEvents;
    } else {
      //if the alt buffer exists and the main is incomplete
      //we have to merge them!.... or have we?
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: MERGING (?) buffers.");
      #endif
      buttonsClickStack[indx].buttonClickStackEvents = buttonsClickStackAlt[indx_alt].buttonClickStackEvents;
    }
  } else
    this->clearClickStack(pin, false);
  this->clearClickStack(pin, true);
}

/// @brief Well, returns the index in the vector of where the stack events with the defined pin are located (and depends on the alternative vs main param)
/// @param pin 
/// @param is_alt 
/// @return 
int SWbtns::getClickStackIndex(int pin, bool is_alt) {
  t_buttonsStack * stack = &buttonsClickStack;
  if(is_alt) stack = &buttonsClickStackAlt;
  for(int i=0; i<(*stack).size();i++) {
    if((*stack)[i].PIN == pin) return i;
  }
  return -1;
}

/// @brief Sets the limit for the maximum amount of buffered clicks are allowed. Saying, for double-click the best value should be probably not less than 5
/// @param l 
void SWbtns::setButtonStackFimit(int l) {
  //button_click_flow_limit = l;
  CSWButtons::button_click_flow_limit = l;
}
int SWbtns::getButtonStackFimit(void) {
  return CSWButtons::button_click_flow_limit;
}
void SWbtns::processStack(void) {
  if(buttonsClickStackLocked || buttonsClickStackAltLocked) {
    //another instance of processStack is running
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: another instance of processStack is running.");
    #endif
    return;
  }
  for(buttonEventsStack buttons_stack_el : buttonsClickStack) {
    int pin = buttons_stack_el.PIN;
    int indx = this->getClickStackIndex(pin, false);
    int indx_alt = this->getClickStackIndex(pin, true);
    if((indx_alt != -1) && (buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size() > 0)) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Found alt buffer. Processing it.");
      #endif
      // No checks if the alt buffer is complete or not - we don't care at this moment.
      if(buttons_stack_el.buttonClickStackEvents.size() == 0) {
        buttonsClickStackLocked = true;
        buttonsClickStackAltLocked = true;
        this->clearClickStack(pin, false);
        this->swapClickStacks(pin);
        buttonsClickStackLocked = false;
        buttonsClickStackAltLocked = false;
      } else {
        if(!this->checkClickStackDone(pin, false) ) {
          // The main queue is NOT complete and the alt queue isn't empty.
          // Bad situation, should not happen. I'll clear both.
          #if defined(DEBUG) && DEBUG>=10
          Serial.println("CSWBUTTONS: The buffers issue happened. They will be cleared.");
          #endif
          buttonsClickStackLocked = true;
          buttonsClickStackAltLocked = true;
          this->clearClickStack(pin, true);
          this->clearClickStack(pin, false);
          buttonsClickStackLocked = false;
          buttonsClickStackAltLocked = false;
        } // Otherwise it'll be processed below and NEXT iteration will go to just swap
      }
    }
    if(this->checkClickStackDone(pin, false) ) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Found the click buffer to process. Processing it.");
      #endif
      buttonsClickStackLocked = true;
      int clicks_amount = buttons_stack_el.buttonClickStackEvents.size();
      //let's process the stack
      buttonClickStackEvent ell = buttons_stack_el.buttonClickStackEvents[clicks_amount-1];
      if (( clicks_amount == 1 )
      &&
      ((ell.time_unpressed - ell.time_pressed) >= CSWButtons::button_recheck_interval_longpress_ms)) {
          #if defined(DEBUG) && DEBUG>=10
          Serial.print("CSWBUTTONS: Calling multiclick LONGPRESS callback! PIN: ");
          Serial.println(pin);
          #endif
          _callbackMulticlick(pin,1,true,(ell.time_unpressed - ell.time_pressed));
      } else {
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("CSWBUTTONS: Calling multiclick callback! The clicks numer there is: ");
        Serial.print(clicks_amount);
        Serial.print("; time between click and unclick diff is: ");
        Serial.println(ell.time_unpressed - ell.time_pressed);
        Serial.print("CSWBUTTONS: PIN: ");
        Serial.println(pin);
        #endif
        _callbackMulticlick(pin,max(clicks_amount,1),false);
      }
      this->clearClickStack(pin, false);
      buttonsClickStackLocked = false;
    }
  }
  buttonsClickStackLocked = false;
  buttonsClickStackAltLocked = false;
}