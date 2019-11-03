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

    ~UBPortsLightControl();

    void set_state(LedState new_state) override;
    LedState get_state() override;
    void set_color(uint r, uint g, uint b) override;
    int get_on_ms() override;
    void set_on_ms(int on_ms) override;
    int get_off_ms() override;
    void set_off_ms(int off_ms) override;

    void start_processing() override;

protected:
    bool init();
    void turn_on();
    void turn_off();

    light_device_t* m_light_device;
    LedState m_state;
    int m_on_ms;
    int m_off_ms;
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

    void update_light();
    void update_light_state();
    void update_light(light_state_t * lightState);

    // These need to be at the end, so that handlers are unregistered first on
    // destruction, to avoid accessing other members if an event arrives
    // on destruction.
    HandlerRegistration lightcontrol_handler_registration;

};

}
