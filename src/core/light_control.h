#pragma once

namespace repowerd
{

class LightControl
{
public:

    enum class DisplayState { DisplayUnknown, DisplayOn, DisplayOff};
    enum class LedState { Off, On, };

    virtual ~LightControl() = default;

    virtual void set_state(LedState new_state) = 0;
    virtual LedState get_state() = 0;
    virtual void set_color(uint r, uint g, uint b) = 0;
    virtual int get_on_ms() = 0;
    virtual void set_on_ms(int on_ms) = 0;
    virtual int get_off_ms() = 0;
    virtual void set_off_ms(int off_ms) = 0;

    virtual void start_processing() = 0;

protected:
    LightControl() = default;
    LightControl (LightControl const&) = default;
    LightControl& operator=(LightControl const&) = default;
};

}

