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
#include <string>
#include <sstream>

#define VERIFICATION_BTN_TIMEOUT 500
#define SLF_RESET_TIMEOUT 2000
#define VRF_TIMEOUT 100
#define DEBUG 1

typedef void (*VoidFunctionWithNoParameters) (void);
typedef void (*VoidFunctionWithOneParameter) (int);

SimpleTimer stimer;

std::vector<uint8_t> btnPins;

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
  std::vector<_click_event> clickEvents;
  std::vector<_btn_struct> stack;
  bool _eventsBlocked=false;
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
  int CLICK_index(std::string ev_name)
  {
    for(int i=0;i<clickEvents.size();++i)
    {
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
    Serial.print("NAME of CLICK function: ");
    Serial.print(ev_name.c_str());
    Serial.print("; INDEX of CLICK function: ");
    Serial.println(indx);
    #endif
    if(indx != -1) return clickEvents[indx].event_function;
    else return __null;
  }

  void execOnclickFunction(int pin, int click_count=1) {
    VoidFunctionWithOneParameter f = this->getOnclickFunction(pin, click_count);
    if(f) f(pin);
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
    Serial.print("NAME of longpress function: ");
    Serial.print(ev_name.c_str());
    Serial.print("; INDEX of longpress function: ");
    Serial.println(indx);
    #endif
    if(indx != -1) return clickEvents[indx].event_function;
    else return __null;
  }

  void execOnlongpressFunction(int pin) {
    VoidFunctionWithOneParameter f = this->getOnlongpressFunction(pin);
    if(f) f(pin);
  }
  
  int getPinByNum(int pinNum) {
    return stack[pinNum].pin;
  }

  void onclick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count=1)
  {
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
      stack[btn_index]._timerId=_timerId;
    }
    else {
      //add new
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
        if(stack[btn_index]._timerId>=0) stimer.deleteTimer(stack[btn_index]._timerId);
      }
      catch(...) {
        //whatever
        #if defined(DEBUG) && DEBUG>=1
        Serial.println("Could not remove timer. Whatever.");
        #endif
      }
      stack[btn_index]._timerId=-1;
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
      Serial.println("INCREASING RECURSION!");
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
      Serial.println("RESETTING RECURSION!");
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
      stack[btn_index].unclick_count++;
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
    Serial.println("**************************************");
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
      Serial.println("**************************************");
    }
    else {
      Serial.println("info NOT FOUND!");
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

  void resetBtn(int pin) {
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
      stack[btn_index]._button_press_time=0;
      stack[btn_index]._button_unpress_time=0;
      stack[btn_index].click_count=-1;
      stack[btn_index].unclick_count=-1;
    }
    else
    {
      //add new
      this->Add_btn(pin);
    }
  }
  
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

void _callbackMulticlick(int pin, int _click_count,bool longPress=false) {
  int click_count=_click_count+1;
  if(click_count > CSmartWatchButtons::button_click_flow_limit) click_count=CSmartWatchButtons::button_click_flow_limit;
  _btn_struct btn;
  btn = btns[pin];
  if( longPress || (
    (click_count == 1) && ((btn._button_unpress_time - btn._button_press_time) > CSmartWatchButtons::button_recheck_interval_longpress_ms))
    ) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.println("MULTICLICK. LONGPRESS.");
    #endif
    return btns.execOnlongpressFunction(pin);
  } else {
    #if defined(DEBUG) && DEBUG>=1
    Serial.print("MULTICLICK. Click amount: ");
    Serial.println(click_count);
    #endif
    return btns.execOnclickFunction(pin, click_count);
  }
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("Timers amount: ");
  Serial.println(stimer.getNumTimers());
  #endif
}

int _sttmout(long d, timer_callback f) {
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("Timers: ");
  Serial.println(stimer.getNumTimers());
  #endif
  if(stimer.getNumTimers() > CSmartWatchButtons::button_click_flow_limit) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("Timer overflow! NOT adding a new timer!");
    #endif
    return -1;
  }
  else
    return stimer.setTimeout(d, f);
}

void handleTimer(int pinNum) {
  //ONPRESS event is handled here
  int pin=btns.getPinByNum(pinNum);//to be refactored
  _btn_struct btn;
  int clickCount=-1;
  int unclickCount=-1;
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("ONPRESS TIMER FOR PIN: ");
  Serial.println(pin);
  #endif
  btn = btns[pin];
  int clc = btn.click_count;
  if(clc > CSmartWatchButtons::button_click_flow_limit) {
    #if defined(DEBUG) && DEBUG>=1
    Serial.println("TOO MUCH CLICKS. Ignoring this one.");
    #endif
    return;
  }
  btns.Inc_click_count(pin);
  btn = btns[pin];
  //btns.Update_click_count(pin,clc+1);
  int pressedTime=btn._button_press_time;
  int unPressedTime=btn._button_unpress_time;
  int btn_recursion=btn.recursion_level;
  #if defined(DEBUG) && DEBUG>=10
  Serial.print("Recursion level: ");Serial.println(btn_recursion);
  #endif
  if(btn_recursion > CSmartWatchButtons::button_click_flow_limit) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("Recursion overflow. Resetting.");
    #endif
    btns.resetBtn(pin);
    btns.Reset_timer_id(pin);
    btns.Reset_btn_recursion(pin);
    return;
  }
  clickCount=btn.click_count;
  unclickCount=btn.unclick_count;
  if((btn.click_count > 0) || (btn.unclick_count > 0)) {
    
    //if the unclicks didn't change - then no sense to run it XXX more times, right?
    if((btn.unclick_count == btn.prevunclick_count) && (btn.unclick_count > 0 )) {
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("Updating click count - outta recursion.");
      #endif
      btns.Update_click_count(pin,btn.unclick_count);
      btn = btns[pin];
    }
    
    //multiclick.
    if(btn.click_count == btn.unclick_count) {
      if(btn.click_count > CSmartWatchButtons::button_click_flow_limit) {
        btn.click_count = CSmartWatchButtons::button_click_flow_limit;
        btn.unclick_count = CSmartWatchButtons::button_click_flow_limit;
      }
      //multickick finished
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("Multickick finished");
      #endif
      _callbackMulticlick(pin,btn.click_count);
      //now clear all up
      btns.resetBtn(pin);
      btns.Reset_btn_recursion(pin);
      btns.Reset_timer_id(pin);
      return;
    }
    else if(
        (btn.click_count <= CSmartWatchButtons::button_click_flow_limit)
        &&
        (btn.unclick_count <= CSmartWatchButtons::button_click_flow_limit)
      ) {
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("Clicks: ");Serial.println(btn.click_count);
        Serial.print("Unclicks: ");Serial.println(btn.unclick_count);
        #endif
        if(btn_recursion <= CSmartWatchButtons::button_click_flow_limit) {
          btns.Update_btn_recursion(pin);
          //set new timeout - for the flow
          #if defined(DEBUG) && DEBUG>=10
          Serial.println("set new timeout - for the flow");
          #endif
          btns.Update_prevunclick_count(pin,btn.unclick_count);
          btns.Update_timer_id(pin,_sttmout(VERIFICATION_BTN_TIMEOUT, btns._tmoFunctions[btn.indx_num]));
          return;
        } else {
          btns.Reset_btn_recursion(pin);
          //smth went wrong. We have to clear up everything.
          #if defined(DEBUG) && DEBUG>=1
          Serial.println("Smth went wrong - recursion limit reached... let's clear everything up. Issue with unpress perhaps or the order?");
          #endif
          btns.resetBtn(pin);
          btns.Reset_timer_id(pin);
          _callbackMulticlick(pin,btn.click_count);
          clickCount=btn.click_count;
          unclickCount=btn.unclick_count;
          return;
        }
    } else {
      //smth went wrong. We have to clear up everything.
      #if defined(DEBUG) && DEBUG>=1
      Serial.println("Smth went wrong... let's clear everything up. It's a single click perhaps?");
      #endif
      btns.resetBtn(pin);
      btns.Reset_btn_recursion(pin);
      clickCount=0;
      unclickCount=0;
      btns.Reset_timer_id(pin);
    }
  }
  if(
    (clickCount==0) 
    &&
    (unclickCount==0)
    ) {
      #if defined(DEBUG) && DEBUG>=1
      Serial.println("Ordinary click detected.");
      #endif
      //ordinary click
      _callbackMulticlick(pin,0);
      btns.resetBtn(pin);
      btns.Reset_btn_recursion(pin);
      btns.Reset_timer_id(pin);
    } else if ((clickCount==0)/* && (btn_recursion < 1)*/) {
      btns.Update_btn_recursion(pin);
      btns.Dec_click_count(pin);
      //set new timeout - for the flow
      #if defined(DEBUG) && DEBUG>=10
      Serial.println("We've maybe got LONGPRESS?");
      Serial.print("Clicks: ");
      Serial.println(clickCount);
      Serial.print("Unclicks: ");
      Serial.println(unclickCount);
      #endif
      btns.Update_timer_id(pin,_sttmout(VERIFICATION_BTN_TIMEOUT, btns._tmoFunctions[btn.indx_num]));
      return;
    } else {
      #if defined(DEBUG) && DEBUG>=10
      Serial.print("WHO'S THERE???");
      #endif
    }
}
void _callbackUnpress(int pin) {
  //on button RELEASE - UNPRESS - is handled here!
  //_btn_struct btn;
  //Serial.println("ON-UNPRESS TIMER FOR PIN: ");
  //Serial.println(pin);
  //btn = btns[pin];
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("IN UNPRESS CALLBACK!");
  btns._displayInfo(pin);
  #endif
}

void handleInterrupt(int pinNum) {
  int pin=btns.getPinByNum(pinNum);//to be refactored
  //btns._displayInfo(pin);
  if(btns.checkEventsBlocked()) {
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("Blocking onclick!");
    #endif
    return;
  }
  _btn_struct btn;
  uint8_t eventType = 0; //0 - pressed, 1 - unpressed
  unsigned long pressedTime = millis();
  unsigned long unPressedTime = millis();
  uint8_t buttonState = digitalRead(pin);
  if(buttonState == LOW) {
    pressedTime = millis();
    eventType = 0;
  } else {
    unPressedTime = millis();
    eventType = 1;
  }
  btn = btns[pin];
  if(eventType == 1) {
    //on UNPRESS do this:
    if(
      (unPressedTime-btn._button_unpress_time>CSmartWatchButtons::button_min_recheck_interval_ms)
      ||
      (unPressedTime < btn._button_unpress_time)
      ) {
       #if defined(DEBUG) && DEBUG>=10
       Serial.println("UNPRESS DETECTED!!!");
       #endif
       #if defined(DEBUG) && DEBUG>=10
       Serial.println("Increasing unclick count...");
       #endif
       btns.Inc_unclick_count(pin);
       #if defined(DEBUG) && DEBUG>=10
       Serial.println("Updating btn unpress time...");
       #endif
       btns.Update_btn_unpress_time(pin,unPressedTime);
       #if defined(DEBUG) && DEBUG>=10
       Serial.println("Calling unpress callback...");
       #endif
        _callbackUnpress(pin);
        #if defined(DEBUG) && DEBUG>=10
        Serial.println("Outta here.");
        #endif
      }
  }
  else {
    //on PRESS do this:
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("PRESS DETECTED!!!");
    #endif
    if(
      (pressedTime-btn._button_press_time>CSmartWatchButtons::button_min_recheck_interval_ms)
      ||
      (pressedTime < btn._button_press_time)
      ) {
        #if defined(DEBUG) && DEBUG>=10
        Serial.print("PRESS PASSED FURTHER!!! ");
        #endif
        #if defined(DEBUG) && DEBUG>=10
        Serial.println(pressedTime-btn._button_press_time);
        #endif
        btns.Update_btn_press_time(pin,pressedTime);
        //new functionality
        if(btn.indx_num >=0) {
          #if defined(DEBUG) && DEBUG>=10
          Serial.println("Setting up timeout.");
          #endif
          if(btn.click_count+1 <= CSmartWatchButtons::button_click_flow_limit) {
            btns.Update_timer_id(pin,_sttmout(VERIFICATION_BTN_TIMEOUT, btns._tmoFunctions[btn.indx_num]));
          }
          #if defined(DEBUG) && DEBUG>=10
          else 
            Serial.println("Multiclick limite reached. NOT setting up timeout.");
          #endif
        }
        #if defined(DEBUG) && DEBUG>=10
        else
          Serial.println("btn.indx_num is not set...");
        #endif
    }
  }
}
unsigned long lastPressedTime = millis();
#define PIN_HANDLER(pin) \
void pin_handler_##pin (void) \
{ \
  unsigned long pressedTime = millis();\
  if(pressedTime-lastPressedTime >= 100) {lastPressedTime=pressedTime;handleInterrupt(pin);} \
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

CSmartWatchButtons::CSmartWatchButtons() {
}

void CSmartWatchButtons::addButton(int pin) {
    btnPins.push_back(pin);
  }

void CSmartWatchButtons::onClick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count=1) {
  btns.onclick(pin, onclick_function, click_count);
}

void CSmartWatchButtons::onLongpress(int pin, VoidFunctionWithOneParameter onclick_function) {
  btns.onlongpress(pin, onclick_function);
}

bool CSmartWatchButtons::checkEventsBlocked() {
  return _firstRun || _eventsBlocked;
}
void CSmartWatchButtons::checkEventsBlocked(bool v) {
  _eventsBlocked = v;
}

void CSmartWatchButtons::attachInterrupts() {
  _firstRun=true;
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("First run set.");
  #endif
  btns.setEventsBlocked(true);
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("Events blocked set. Going to pinmodes...");
  Serial.print("Buttons numer is: ");
  Serial.println(btnPins.size());
  #endif
  for(int i=0;i<btnPins.size();i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
    #if defined(DEBUG) && DEBUG>=10
    Serial.print("Pinmode for the pin ");
    Serial.print(btnPins[i]);
    Serial.print(" with nr ");
    #endif
    Serial.print(i);
    #if defined(DEBUG) && DEBUG>=10
    Serial.println(" is set as PULLUP. Attaching interrupt...");
    #endif
    attachInterrupt (btnPins[i], intrp_functions[i], CHANGE);
    #if defined(DEBUG) && DEBUG>=10
    Serial.println("Interrupt attached. Adding button...");
    #endif
    btns.Add_btn(btnPins[i],i);
  }
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("Interrupts attached! Now calling the tmo functions...");
  delay(500);
  #endif
  for(int i=0;i<btnPins.size();i++)
    btns._tmoFunctions.push_back(tmo_functions[i]);
  #if defined(DEBUG) && DEBUG>=10
  Serial.println("TMO functions called. Exiting.");
  delay(500);
  #endif
}
void CSmartWatchButtons::tickTimer() {
  if(_firstRun) {
    btns.setEventsBlocked(false);
    _firstRun=false;
  }
  stimer.run();
  delay(VRF_TIMEOUT);
  //Serial.println(ESP.getFreeHeap());
  //Serial.println(uxTaskGetStackHighWaterMark(NULL));
  //if((millis() % SLF_RESET_TIMEOUT) <=VRF_TIMEOUT) this->resetTimer();
}
void CSmartWatchButtons::resetTimer() {
  SimpleTimer stimer;
}