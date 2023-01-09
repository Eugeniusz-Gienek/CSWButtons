/**
  ******************************************************************************
  * @file    buttons.cpp
  * @author  Eugene at sky.community
  * @version V1.0.0
  * @date    12-December-2022
  * @brief   The functionality to work with the buttons of the smartwatch based on esp32.
  *
  ******************************************************************************
  */

#include "CSWButtons.h"
using namespace swbtns;
#include <SimpleTimer.h>
#include <vector>
#include <deque>
#include <string>
#include <sstream>

#define VERIFICATION_BTN_TIMEOUT 500
#define SLF_RESET_TIMEOUT 2000
#define VRF_TIMEOUT 100
#define DEBUG 0

typedef void (*VoidFunctionWithNoParameters) (void);
typedef void (*VoidFunctionWithOneParameter) (int);

SimpleTimer stimer;

std::vector<uint8_t> btnPins;

int CSWButtons::button_click_flow_limit=3;

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
      int click_count=-1;
      int unclick_count=-1;
      int prevunclick_count=-1;
      int _button_press_time=0;
      int _button_unpress_time=0;
      int _timerClick=0;
      int _timerUnpress=0;
      int _timerId=-1;
      int indx_num=-1;
      int recursion_level=0;
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
  int button_click_flow_limit=5;


  t_buttonsStack buttonsClickStack;
  // alt click stack is used for the situations when during the processing of the stack
  // there are click events happening and they are processed by the CPU and the code is
  // executed. Seems to be that ESP doesn't support it YET but when it will be the code 
  // will just work. Hopefully. For the same reasons the blockers below are introduced.
  // BTW we block the selected stack for ALL buttons at once at this moment - thus
  // the blocker vars are just bool's and not having IDs inside them.
  t_buttonsStack buttonsClickStackAlt;
  bool buttonsClickStackLocked=false;
  bool buttonsClickStackAltLocked=false;
  int getClickStackIndex(int pin, bool is_alt=false);


  public:
  //VoidFunctionWithNoParameters _tmoFunctions[];
  std::vector<void (*)()> _tmoFunctions;
  bool CLICK_present(std::string ev_name)
  {
    for(int i=0;i<clickEvents.size();++i)
    {
      if(clickEvents[i].name==ev_name)
        return true;
    }
    return false;
  }
  bool checkEventsBlocked() {
    return _eventsBlocked;
  }
  void setEventsBlocked(bool v) {
    _eventsBlocked=v;
  }
  void cleanupBtns(void) {
    for(_btn_struct btn : stack) {
      int pressedTime=millis();
      int unPressedTime=millis();
      if (
        (!
          (
            (btn._button_unpress_time <= 0)
            &&
            (btn._button_unpress_time <= 0)
          )
        )
        &&
        (
          !(stimer.getNumTimers() > 0)
        )
        &&
        (
          (
            (
            (unPressedTime-btn._button_unpress_time>CSWButtons::button_recheck_interval_ms)
            ||
            (unPressedTime < btn._button_unpress_time)
            )
          )
          &&
          (
            (
            (pressedTime-btn._button_press_time>CSWButtons::button_recheck_interval_ms)
            ||
            (pressedTime < btn._button_press_time)
            )
          )
        )
      )
      {
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Cleaning the potentially stuck buttons.");
        #endif
        resetBtn(btn.pin);
      }
    }
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

  void execOnlongpressFunction(int pin) {
    VoidFunctionWithOneParameter f = this->getOnlongpressFunction(pin);
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Execution of the longpress function...");
    #endif
    if(f) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Function found. Executing!");
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
  
  bool BTN_present(std::string btn_name)
  {
    for(int i=0;i<stack.size();++i)
    {
      if(stack[i].name==btn_name)
        return true;
    }
    return false;
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
    int click_count=-1;
    int unclick_count=-1;
    int _button_press_time;
    int _button_unpress_time;
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
      _btn._button_press_time=0;
      _btn._button_unpress_time=0;
      _btn.click_count=-1;
      _btn.unclick_count=-1;
      _btn.indx_num=indx_num;
      stack.push_back(_btn);
    }
  }

  void Update_timer_id(int pin,int _timerId)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("CSWBUTTONS: Updating timer id for button with index: ");
      Serial.println(btn_index);
      #endif
      stack[btn_index]._timerId=_timerId;
    }
    else {
      //add new
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Addin new button and timer id.");
      #endif
      this->Add_btn(pin);
      this->Update_timer_id(pin,_timerId);
    }
  }

  void Reset_timer_id(int pin)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      try {
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("CSWBUTTONS: REMOVING  timer for the btn_index:");
        Serial.println(btn_index);
        #endif
        if(stack[btn_index]._timerId>=0) stimer.deleteTimer(stack[btn_index]._timerId);
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("CSWBUTTONS: Timers amount LEFT: ");
        Serial.println(stimer.getNumTimers());
        Serial.println("CSWBUTTONS: **************************************");
        #endif
      }
      catch(...) {
        //whatever
        #if defined(DEBUG) && DEBUG>=1
        Serial.println("CSWBUTTONS: Could not remove timer. Whatever.");
        #endif
      }
      //stack[btn_index]._timerId=-1;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Reset_timer_id(pin);
    }
  }
  
  void Update_btn_press_time(int pin,int _button_press_time)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index]._button_press_time=_button_press_time;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_btn_press_time(pin,_button_press_time);
    }
  }
  
  void Update_btn_unpress_time(int pin,int _button_unpress_time)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index]._button_unpress_time=_button_unpress_time;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_btn_unpress_time(pin,_button_unpress_time);
    }
  }

  void Update_btn_recursion(int pin)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: INCREASING RECURSION!");
      #endif
      stack[btn_index].recursion_level++;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_btn_recursion(pin);
    }
  }
  
  void Reset_btn_recursion(int pin)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
    #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: RESETTING RECURSION!");
    #endif
      stack[btn_index].recursion_level=0;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Reset_btn_recursion(pin);
    }
  }

  void Inc_click_count(int pin) {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index].click_count++;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Inc_click_count(pin);
    }
  }


  void Dec_click_count(int pin) {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index].click_count--;
      if(stack[btn_index].click_count < -1) stack[btn_index].click_count=-1;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Dec_click_count(pin);
    }
  }
  
  void Inc_unclick_count(int pin) {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("CSWBUTTONS: Increasing UNCLICKS count for the btn_index:");
      Serial.println(btn_index);
      Serial.print("CSWBUTTONS: It was:");
      Serial.println(stack[btn_index].unclick_count);
      #endif
      stack[btn_index].unclick_count++;
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("CSWBUTTONS: It becomes:");
      Serial.println(stack[btn_index].unclick_count);
      #endif
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Inc_unclick_count(pin);
    }
  }

  
  void Update_click_count(int pin,int click_count)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index].click_count=click_count;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_click_count(pin,click_count);
    }
  }

  void Update_unclick_count(int pin,int unclick_count)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index].unclick_count=unclick_count;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_unclick_count(pin,unclick_count);
    }
  }


  void Update_prevunclick_count(int pin,int prevunclick_count)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index].prevunclick_count=prevunclick_count;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->Update_prevunclick_count(pin,prevunclick_count);
    }
  }

  void setClickTimer(int pin,int clickTimer)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      stack[btn_index]._timerClick=clickTimer;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->setClickTimer(pin,clickTimer);
    }
  }
  
  void setUnpressTimer(int pin,int unpressTimer)
  {
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("CSWBUTTONS: Setting UNPRESS timer for the btn_index:");
      Serial.println(btn_index);
      #endif
      stack[btn_index]._timerUnpress=unpressTimer;
    }
    else {
      //add new
      this->Add_btn(pin);
      this->setUnpressTimer(pin,unpressTimer);
    }
  }

  void _displayInfo(int pin) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.println("CSWBUTTONS: **************************************");
    Serial.print("Info for the PIN ");
    Serial.println(pin);
    std::string btn_name;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0) {
      Serial.print("click_count");
      Serial.println(stack[btn_index].click_count);
      Serial.print("unclick_count");
      Serial.println(stack[btn_index].unclick_count);
      Serial.print("_button_press_time");
      Serial.println(stack[btn_index]._button_press_time);
      Serial.print("_button_unpress_time");
      Serial.println(stack[btn_index]._button_unpress_time);
      Serial.print("_timerClick");
      Serial.println(stack[btn_index]._timerClick);
      Serial.print("_timerUnpress");
      Serial.println(stack[btn_index]._timerUnpress);
      Serial.print("_timerId");
      Serial.println(stack[btn_index]._timerId);
      Serial.print("indx_num");
      Serial.println(stack[btn_index].indx_num);
      Serial.print("Timers amount: ");
      Serial.println(stimer.getNumTimers());
      Serial.println("CSWBUTTONS: **************************************");
    }
    else {
      Serial.println("CSWBUTTONS: info NOT FOUND!");
    }
    #endif
  }
  
  void Update_btn(int pin,_btn_struct btn)
  {
    std::string btn_name;
    int click_count=-1;
    int unclick_count=-1;
    int _button_press_time;
    int _button_unpress_time;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0)
    {
      stack[btn_index]._button_press_time=btn._button_press_time;
      stack[btn_index]._button_unpress_time=btn._button_unpress_time;
      stack[btn_index].click_count=btn.click_count;
      stack[btn_index].unclick_count=btn.unclick_count;
      stack[btn_index].prevunclick_count=btn.prevunclick_count;
    }
    else
    {
      //add new
      this->Add_btn(pin);
      this->Update_btn(pin,btn);
    }
  }

  void resetBtn(int pin, bool partly=false, int num_to_clean=0) {
    std::string btn_name;
    int click_count=-1;
    int unclick_count=-1;
    int prevunclick_count=-1;
    int _button_press_time;
    int _button_unpress_time;
    std::string btn_spin;
    std::string _btn_spin;
    btn_spin = stringulate(pin);
    _btn_spin = "btn_";
    btn_name = _btn_spin + btn_spin;
    int btn_index=BTN_index(btn_name);
    if(btn_index>=0)
    {
      if(!partly) {
        stack[btn_index]._button_press_time=0;
        stack[btn_index]._button_unpress_time=0;
        stack[btn_index].click_count=-1;
        stack[btn_index].unclick_count=-1;
      }
      else {
        stack[btn_index].click_count -= num_to_clean;
        stack[btn_index].click_count = max(-1,stack[btn_index].click_count);
        stack[btn_index].unclick_count -= num_to_clean;
        stack[btn_index].unclick_count = max(-1,stack[btn_index].unclick_count);
      }
    }
    else
    {
      //add new
      this->Add_btn(pin);
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
    int click_count=-1;
    int unclick_count=-1;
    int _button_press_time;
    int _button_unpress_time;
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
    _btn._button_press_time=0;
    _btn._button_unpress_time=0;
    _btn.click_count=-1;
    _btn.unclick_count=-1;
    _btn.prevunclick_count=-1;
    stack.push_back(_btn);
    return stack[index];
  }
};

SWbtns btns;

bool _callbackMulticlick(int pin, int _click_count,bool longPress=false,int _button_press_time=-100,int _button_unpress_time=-100) {
  int click_count=_click_count;
  if( longPress) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.print("CSWBUTTONS: MULTICLICK. LONGPRESS. PIN: ");
    Serial.println(pin);
    #endif
    btns.execOnlongpressFunction(pin);
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

int _sttmout(long d, timer_callback f) {
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("CSWBUTTONS: Timers: ");
  Serial.println(stimer.getNumTimers());
  #endif
  if(stimer.getNumTimers() > CSWButtons::button_click_flow_limit) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Timer overflow! NOT adding a new timer!");
    #endif
    return -1;
  }
  else {
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("CSWBUTTONS: Timer set! d: ");
    Serial.println(d);
    #endif
    return stimer.setTimeout(d, f);
  }
}

void handleTimer(int pinNum) {
  //TODO: remove. Obsolete.
}
void _callbackUnpress(int pin) {
  //on button RELEASE - UNPRESS - is handled here!
  //_btn_struct btn;
  //Serial.println("CSWBUTTONS: ON-UNPRESS TIMER FOR PIN: ");
  //Serial.println(pin);
  //btn = btns[pin];
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: IN UNPRESS CALLBACK!");
  btns._displayInfo(pin);
  #endif
}

void handleInterrupt(int pinNum) {
  int pin=btns.getPinByNum(pinNum);//to be refactored
  //btns._displayInfo(pin);
  if(btns.checkEventsBlocked()) {
    //#if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: BUTTONS: Blocking onclick!");
    //#endif
    return;
  }
  uint8_t eventType = 0; //0 - pressed, 1 - unpressed
  uint8_t buttonState = digitalRead(pin);
  if(buttonState == LOW) {
    eventType = 0;
  } else {
    eventType = 1;
  }
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
  if(pressedTime-lastPressedTime >= 5) {lastPressedTime=pressedTime;handleInterrupt(pin);} \
}
#define TMO_HANDLER(pin) \
void tmo_handler_##pin (void) \
{ \
  handleTimer(pin); \
}
//////////////////////////////////////////////////////////////////////////////
/* The config of buttons lies actually here */

#define PIN_HANDLER(pin) \
extern void pin_handler_##pin (void) \
{ \
  unsigned long pressedTime = millis();\
  if(pressedTime-lastPressedTime >= 100) {lastPressedTime=pressedTime;handleInterrupt(pin);} \
}
#define TMO_HANDLER(pin) \
extern void tmo_handler_##pin (void) \
{ \
  handleTimer(pin); \
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
TMO_HANDLER (pinKey1)
PIN_HANDLER (pinKey2)
TMO_HANDLER (pinKey2)
PIN_HANDLER (pinKey3)
TMO_HANDLER (pinKey3)
PIN_HANDLER (pinKey4)
TMO_HANDLER (pinKey4)
PIN_HANDLER (pinKey5)
TMO_HANDLER (pinKey5)
PIN_HANDLER (pinKey6)
TMO_HANDLER (pinKey6)
PIN_HANDLER (pinKey7)
TMO_HANDLER (pinKey7)
PIN_HANDLER (pinKey8)
TMO_HANDLER (pinKey8)
PIN_HANDLER (pinKey9)
TMO_HANDLER (pinKey9)
PIN_HANDLER (pinKey10)
TMO_HANDLER (pinKey10)
/*
 * End of bullsh*t...
*/

VoidFunctionWithNoParameters intrp_functions[] = {pin_handler_pinKey1,pin_handler_pinKey2,pin_handler_pinKey3,pin_handler_pinKey4,pin_handler_pinKey5,pin_handler_pinKey6,pin_handler_pinKey7,pin_handler_pinKey8,pin_handler_pinKey9,pin_handler_pinKey10};
VoidFunctionWithNoParameters tmo_functions[]   = {tmo_handler_pinKey1,tmo_handler_pinKey2,tmo_handler_pinKey3,tmo_handler_pinKey4,tmo_handler_pinKey5,tmo_handler_pinKey6,tmo_handler_pinKey7,tmo_handler_pinKey8,tmo_handler_pinKey9,tmo_handler_pinKey10};

/*
 * Oh, here we go - real end of bullsh*t
*/

//////////////////////////////////////////////////////////////////////////////

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
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: Interrupts attached! Now calling the tmo functions...");
  delay(500);
  #endif
  for(int i=0;i<btnPins.size();i++)
    btns._tmoFunctions.push_back(tmo_functions[i]);
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("CSWBUTTONS: TMO functions called. Exiting.");
  delay(500);
  #endif
  btns.setEventsBlocked(false);
}
void CSWButtons::tickTimer() {
  btns.processStack();
  //delay(VRF_TIMEOUT);
  //Serial.println(ESP.getFreeHeap());
  //Serial.println(uxTaskGetStackHighWaterMark(NULL));
  //if((millis() % SLF_RESET_TIMEOUT) <=VRF_TIMEOUT) this->resetTimer();
}
void CSWButtons::resetTimer() {
  SimpleTimer stimer;
}

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
    // Interesting situation.
    // We expect that the main buffer will be processed in next "tick" iteration.
    // thus let's call it maybe or leave for now? I decided to leave.
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
        //stack - so just remove the first element; NOT optimal code though
        //buttonsClickStackAlt[indx_alt].buttonClickStackEvents.erase(buttonsClickStackAlt[indx_alt].buttonClickStackEvents.begin());
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
    //we have to creat it!
    buttonEventsStack newButtonEventsStack = {};
    newButtonEventsStack.PIN = pin;
    newButtonEventsStack.buttonClickStackEvents = {};
    if(is_alt) {
      buttonsClickStackAlt.push_back(newButtonEventsStack);
    } else {
      buttonsClickStack.push_back(newButtonEventsStack);
    }
    btn_s_indx = 0;
  }
  t_buttonClickStackEvents stack = (*buttons_stack)[btn_s_indx].buttonClickStackEvents;
  if((stack.size() == 0) || (stack[stack.size()-1].is_complete)) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Adding first event to stack.");
    #endif
    //add new
    buttonClickStackEvent bcse;
    if(click_type == CLICK) {
      bcse.time_pressed = currTime;
    }else {
      //First element with UNCLICK only? I think smth is off
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: First element with UNCLICK only? Smth is off.");
      #endif
      //if(is_alt) {
      //  bcse.time_pressed = max(currTime-10,0);
      //  bcse.time_unpressed = max(currTime,0);
      //} else {
      //  return;
      //}
      bcse.time_unpressed = max(currTime,0);
    }
    (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
  } else {
    //update last
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Adding next event to stack.");
    #endif
    if(click_type == CLICK) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Click event detected.");
      #endif
      if(stack[stack.size()-1].time_pressed == -1) {
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: We are updating the previously added event without click time.");
        #endif
        //this means we are updating the previously added event without click time
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed = currTime;
      } else {
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: We've lost the previous unclick because we have time in time_pressed.");
        #endif
        //we've lost the previous unclick because we have time in time_pressed.
        if(stack[stack.size()-1].time_pressed > currTime) {
          //if the system time is changed flush all buffers.
          #if defined(DEBUG) && DEBUG>=1
          Serial.println("CSWBUTTONS: System time seem to either overflow or was changed to PAST.");
          Serial.println("CSWBUTTONS: Buttons buffers will be reset.");
          #endif
          this->clearAllClickStacks();
          return;
        }
        //Not inventing smth
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Broken click event.");
        #endif
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed = currTime;
        return;
        //Unless You're interested in the thing below. Then comment the above 2 lines.
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Adding a fixing unpress event.");
        #endif
        //add an unpress event (fixing one) - 1 MS before current press event or 1 ms after time of the prev click
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed = 
          max(currTime-1, stack[stack.size()-1].time_pressed+1);
        //'cause time_pressed is set - that's why we are in here
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].is_complete=true;
        //and add a new event
        buttonClickStackEvent bcse;
        bcse.time_pressed = currTime;
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
      }
    } else {
      //UNCLICK
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Unclick event detected.");
      #endif
      if(stack[stack.size()-1].time_unpressed == -1) {
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Updating the last event with unclick.");
        #endif
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed = currTime;
      } else {
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: We lost the previous CLICK because we have time in time_unpressed..");
        #endif
        //we lost the previous CLICK because we have time in time_unpressed.
        if(stack[stack.size()-1].time_unpressed > currTime) {
          //if the system time is changed flush all buffers.
          #if defined(DEBUG) && DEBUG>=1
          Serial.println("CSWBUTTONS: System time seem to either overflow or was changed to PAST.");
          Serial.println("CSWBUTTONS: Buttons buffers will be reset.");
          #endif
          this->clearAllClickStacks();
          return;
        }
        // Let's not invent anything and just go away
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: IGNORING the unconnected unclick event.");
        #endif
        return;
        //Or maybe You're interested in the thing below - then just comment out the "return" above;
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Adding a press event (fixing one) - 1 MS before that unpress event");
        #endif
        //add a press event (fixing one) - 1 MS before that unpress event
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed = 
          max(stack[stack.size()-1].time_unpressed-1,0);
        //'cause time_pressed is set - that's why we are in here
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].is_complete=true;
        //and add a new event. BTW it doesn't have CLICK time. Smth seems to be off.
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("CSWBUTTONS: Second unpress event in a row without a press event. Something is off.");
        #endif
        buttonClickStackEvent bcse;
        bcse.time_unpressed = currTime;
        (*buttons_stack)[btn_s_indx].buttonClickStackEvents.push_back(bcse);
      }
    }
  }

}
bool SWbtns::checkClickStackDone(int pin, bool is_alt) {
  int btn_s_indx = this->getClickStackIndex(pin, is_alt);
  t_buttonsStack * buttons_stack = &buttonsClickStack;
  if(is_alt) buttons_stack = &buttonsClickStackAlt;
  if(btn_s_indx == -1) return false;
  t_buttonClickStackEvents stack = (*buttons_stack)[btn_s_indx].buttonClickStackEvents;
  //check for reaching the stack limit
  if(stack.size() >= this->getButtonStackFimit()) return true;
  if(stack.size() == 0) return false;
  int currTime = millis();

  //mark the LAST complete item, as we don't care about all of them actually
  if(stack.size()>0) {
    int ii = stack.size() - 1;
  //for(int ii=0;ii<stack.size();ii++){
    if(
      (stack[ii].time_pressed != -1)
      &&
      (stack[ii].time_unpressed != -1)
    ) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("CSWBUTTONS: Marking correct events as complete.");
      #endif
      (*buttons_stack)[btn_s_indx].buttonClickStackEvents[ii].is_complete=true;
    }
  }

  if((
    (stack[stack.size()-1].time_unpressed == -1)
    &&
    (stack[stack.size()-1].time_pressed != -1)
    &&
    (currTime - stack[stack.size()-1].time_pressed > CSWButtons::button_recheck_interval_ms)
  ) || (
    (stack[stack.size()-1].time_unpressed != -1)
    &&
    (stack[stack.size()-1].time_pressed == -1)
    &&
    (currTime - stack[stack.size()-1].time_unpressed > CSWButtons::button_recheck_interval_ms)
  ))
   {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("CSWBUTTONS: Marking last incorrect events as complete.");
    Serial.print("CSWBUTTONS: Time unpressed was:");
    Serial.println((*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed);
    Serial.print("CSWBUTTONS: Time pressed was:");
    Serial.println((*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed);
    #endif
    (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed = currTime;
    if(stack[stack.size()-1].time_pressed == -1) (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_pressed = (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].time_unpressed-1;
    (*buttons_stack)[btn_s_indx].buttonClickStackEvents[stack.size()-1].is_complete=true;
  }

  //check for reaching the time limit
  if(stack[stack.size()-1].is_complete && (
      (currTime-stack[stack.size()-1].time_unpressed > CSWButtons::button_recheck_interval_longpress_ms)
      ||
      (currTime-stack[stack.size()-1].time_unpressed < 0)
    )
  ) return true;
  return false;
}

void SWbtns::clearClickStack(int pin, bool is_alt) {
  if(is_alt) {
    int indx_alt = this->getClickStackIndex(pin, true);
    if(indx_alt != -1) buttonsClickStackAlt[indx_alt].buttonClickStackEvents.clear();
  } else {
    int indx = this->getClickStackIndex(pin, false);
    if(indx != -1) buttonsClickStack[indx].buttonClickStackEvents.clear();
  }
}

void SWbtns::clearAllClickStacks() {
  buttonsClickStack.clear();
}

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
      /*
      int tempI = 0;
      if(buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_unpressed == -1) {
        if(buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size() > 0) {
          buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_pressed = buttonsClickStackAlt[indx_alt].buttonClickStackEvents[0].time_pressed;
          buttonsClickStackAlt[indx_alt].buttonClickStackEvents[buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size()-1].is_complete = true;
          tempI = 1;
        }
      }
      for(int tempII=tempI;tempII<buttonsClickStackAlt[indx_alt].buttonClickStackEvents.size();tempII++) {
        buttonsClickStack[indx].buttonClickStackEvents.push_back(buttonsClickStackAlt[indx_alt].buttonClickStackEvents[tempII]);
      }*/
      buttonsClickStack[indx].buttonClickStackEvents = buttonsClickStackAlt[indx_alt].buttonClickStackEvents;
      // Actually we care only about the last item. This is called when we've just processed
      // the main buffer so we don't have to actually merge it.
      if(
        //(buttonsClickStack[indx].buttonClickStackEvents.size()>0)
        //&&
        (
          (buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_pressed == -1)
          && 
          (buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_unpressed != -1)
        )
      )
       {
        buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_pressed = max(buttonsClickStack[indx].buttonClickStackEvents[buttonsClickStack[indx].buttonClickStackEvents.size()-1].time_unpressed-10,0);
      }
    }
    //buttonsClickStack[indx].buttonClickStackEvents = buttonsClickStackAlt[indx_alt].buttonClickStackEvents;
  } else
    this->clearClickStack(pin, false);
  this->clearClickStack(pin, true);
}
int SWbtns::getClickStackIndex(int pin, bool is_alt) {
  t_buttonsStack * stack = &buttonsClickStack;
  if(is_alt) stack = &buttonsClickStackAlt;
  for(int i=0; i<(*stack).size();i++) {
    if((*stack)[i].PIN == pin) return i;
  }
  return -1;
}
void SWbtns::setButtonStackFimit(int l) {
  button_click_flow_limit = l;
}
int SWbtns::getButtonStackFimit(void) {
  return button_click_flow_limit;
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
          _callbackMulticlick(pin,1,true);
      } else {
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("CSWBUTTONS: Calling multiclick callback! The clicks numer there is: ");
        Serial.print(clicks_amount);
        Serial.print("; time between click and unclick diff is: ");
        Serial.println(ell.time_unpressed - ell.time_pressed);
        Serial.print("CSWBUTTONS: PIN: ");
        Serial.println(pin);
        #endif
        _callbackMulticlick(pin,clicks_amount,false);
      }
      this->clearClickStack(pin, false);
      buttonsClickStackLocked = false;
    }
  }
  buttonsClickStackLocked = false;
  buttonsClickStackAltLocked = false;
}