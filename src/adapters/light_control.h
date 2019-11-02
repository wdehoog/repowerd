#pragma once

#include "dbus_connection_handle.h"
#include "dbus_event_loop.h"

#include "src/core/light_control.h"
#include "event_loop.h"

#pragma GCC diagnostic ignored "-Wpedantic"
extern "C" {
#include <hardware/hardware.h>
#include <hardware/lights.h>
}
#pragma GCC diagnostic pop


namespace repowerd {

class Log;

class UBPortsLightControl : public LightControl
{
public:

    UBPortsLightControl(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void setState(LedState newState) override;
    LedState state() override;
    void setColor(uint r, uint g, uint b) override;
    int onMillisec() override;
    void setOnMillisec(int onMs) override;
    int offMillisec() override;
    void setOffMillisec(int offMs) override;

    void start_processing() override;

protected:
    bool init();
    void turnOn();
    void turnOff();

    light_device_t* m_lightDevice;
    LedState m_state;
    int m_onMs;
    int m_offMs;
    uint m_color;

    void handle_dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    HandlerRegistration name_owner_changed_handler_registration;

    std::shared_ptr<Log> const log;

    void updateLight();
    void update_light_state();
    void updateLight(light_state_t * lightState);

    // These need to be at the end, so that handlers are unregistered first on
    // destruction, to avoid accessing other members if an event arrives
    // on destruction.
    HandlerRegistration lightcontrol_handler_registration;

};

}
