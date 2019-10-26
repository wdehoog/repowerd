#pragma once

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include "src/core/light_control.h"
#include "event_loop.h"

#include <memory>

#pragma GCC diagnostic ignored "-Wpedantic"
extern "C" {
#include <hardware/hardware.h>
#include <hardware/lights.h>
}
#pragma GCC diagnostic pop

namespace repowerd {

class UBPortsLightControl : public LightControl
{
public:
    UBPortsLightControl();

    void setState(State newState) override;
    State state() override;
    void setColor(int r, int g, int b) override;
    int onMillisec() override;
    void setOnMillisec(int onMs) override;
    int offMillisec() override;
    void setOffMillisec(int offMs) override;

protected:
    bool init();
    void turnOff();
    void turnOn();

    light_device_t* m_lightDevice;
    State m_state;
    int m_onMs;
    int m_offMs;
    uint m_color;


};

}
