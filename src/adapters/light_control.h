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

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

namespace repowerd {

class Log;

class UBPortsLightControl : public LightControl
{
public:
    UBPortsLightControl(std::shared_ptr<Log> const& log);
    //UBPortsLightControl(
    //    std::shared_ptr<Log> const& log,
    //    std::string const& dbus_bus_address);

    void setState(State newState) override;
    State state() override;
    void setColor(int r, int g, int b) override;
    int onMillisec() override;
    void setOnMillisec(int onMs) override;
    int offMillisec() override;
    void setOffMillisec(int offMs) override;

    void start_processing() override;

protected:
    bool init();
    void turnOff();
    void turnOn();

    light_device_t* m_lightDevice;
    State m_state;
    int m_onMs;
    int m_offMs;
    uint m_color;
    void handle_dbus_signal(

    GDBusConnection* connection,
    gchar const* sender,
    gchar const* object_path,
    gchar const* interface_name,
    gchar const* signal_name,
    GVariant* parameters);

    std::shared_ptr<Log> const log;

    //DBusConnectionHandle dbus_connection;
    //DBusEventLoop dbus_event_loop;
    //HandlerRegistration dbus_signal_handler_registration;

    void updateLight();

};

}
