#pragma once

#include "handler_registration.h"


namespace repowerd
{

class LightControl
{
public:

    enum State {
        Off,
        On,
    };

    virtual ~LightControl() = default;

    virtual void setState(State newState) = 0;
    virtual State state() = 0;
    virtual void setColor(int r, int g, int b) = 0;
    virtual int onMillisec() = 0;
    virtual void setOnMillisec(int onMs) = 0;
    virtual int offMillisec() = 0;
    virtual void setOffMillisec(int offMs) = 0;

protected:
    LightControl() = default;
    LightControl (LightControl const&) = default;
    LightControl& operator=(LightControl const&) = default;
};

}

