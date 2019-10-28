#pragma once

#include "handler_registration.h"
#include "power_source.h"

namespace repowerd
{

class LightControl
{
public:

    enum DisplayState { DisplayUnknown, DisplayOn, DisplayOff};
    enum State { Off, On, };

    virtual ~LightControl() = default;

    virtual void setState(State newState) = 0;
    virtual State state() = 0;
    virtual void setColor(uint r, uint g, uint b) = 0;
    virtual int onMillisec() = 0;
    virtual void setOnMillisec(int onMs) = 0;
    virtual int offMillisec() = 0;
    virtual void setOffMillisec(int offMs) = 0;

    virtual void start_processing() = 0;
    virtual void notify_battery_info(BatteryInfo *) = 0;
    virtual void notify_display_state(DisplayState) = 0;

protected:
    LightControl() = default;
    LightControl (LightControl const&) = default;
    LightControl& operator=(LightControl const&) = default;
};

}

