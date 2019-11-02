#pragma once

namespace repowerd
{

class LightControl
{
public:

    enum class DisplayState { DisplayUnknown, DisplayOn, DisplayOff};
    enum class LedState { Off, On, };

    virtual ~LightControl() = default;

    virtual void setState(LedState newState) = 0;
    virtual LedState state() = 0;
    virtual void setColor(uint r, uint g, uint b) = 0;
    virtual int onMillisec() = 0;
    virtual void setOnMillisec(int onMs) = 0;
    virtual int offMillisec() = 0;
    virtual void setOffMillisec(int offMs) = 0;

    virtual void start_processing() = 0;

protected:
    LightControl() = default;
    LightControl (LightControl const&) = default;
    LightControl& operator=(LightControl const&) = default;
};

}

