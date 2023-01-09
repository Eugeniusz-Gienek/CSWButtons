/**
  ******************************************************************************
  * @file    buttons.h
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
    void resetTimer(void);
    bool checkEventsBlocked(void);
    void setEventsBlocked(bool v);
    void onLongpress(int pin, VoidFunctionWithOneParameter onclick_function);
    void onClick(int pin, VoidFunctionWithOneParameter onclick_function, int click_count=-1);
    void setButtonClickFlowFimit(int l);
    
    const static int button_recheck_interval_ms=1000;
    static int button_click_flow_limit;
    const static int button_min_recheck_interval_ms=100;
    const static int button_recheck_interval_longpress_ms=400;
  private:
    int _button_pin=-1;
    bool _firstRun=true;
    bool _eventsBlocked=false;
    int _button_press_time=0;
    int _button_unpress_time=0;
    qButton _button1;
    
};
}

#endif
