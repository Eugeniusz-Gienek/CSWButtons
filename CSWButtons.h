/**
  ******************************************************************************
  * @file    CSWbuttons.h
  * @author  Eugene at sky.community
  * @version V1.0.0
  * @date    12-December-2022
  * @brief   The functionality to work with the buttons of the smartwatch based on esp32.
  *
  ******************************************************************************
  */


#ifndef CSWButtons_h
#define CSWButtons_h
#include <stdint.h>
#include <vector>
#include <deque>

namespace swbtns {
  
struct qButton {
  uint8_t PIN;
  uint32_t numberKeyPresses;
  uint32_t numberLongKeyPresses;
  bool pressed;
  bool longpressed;
};

typedef void (*VoidFunctionWithNoParameters) (void);
typedef void (*VoidFunctionWithOneParameter) (int);

struct buttonClickStackEvent {
  int time_pressed=-1;
  int time_unpressed=-1;
  bool is_complete=false;
};
typedef std::deque<buttonClickStackEvent> t_buttonClickStackEvents;
struct buttonEventsStack {
  int PIN;
  t_buttonClickStackEvents buttonClickStackEvents;
};
typedef std::vector<buttonEventsStack> t_buttonsStack;

class CSWButtons{
  public:
    CSWButtons();
    void addButton(int pin);
    void attachInterrupts(void);
    void tickTimer(void);
    bool checkEventsBlocked(void);
    void setEventsBlocked(bool v);
    void onLongpress(int pin, VoidFunctionWithOneParameter onclick_function);
    void onClick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count=-1);
    void setButtonClickFlowFimit(int l);
    void setButtonLongpressIntervalms(int i);
    void setButtonRecheckIntervalms(int i);
    static int button_recheck_interval_ms;
    static int button_click_flow_limit;
    static int button_recheck_interval_longpress_ms;
  private:
    int _button_pin=-1;
    bool _firstRun=true;
    bool _eventsBlocked=false;
    qButton _button1;
    
};
}

#endif
