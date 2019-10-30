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
    // for light styles
    enum LightIndicationState {BatteryCharging, BatteryFull, BatteryLow, UnreadNotifications, BluetoothEnabled, Playing, LIS_NUM_ITEMS};

    // for notifying LightControl
    enum LightEvent {LE_UnreadNotifications, LE_BluetoothEnabled, LE_BatteryLow, LE_BatteryCharging, LE_BatteryFull, LE_Playing, LE_NUM_ITEMS};

    UBPortsLightControl(
        std::shared_ptr<Log> const& log,
        std::string const& dbus_bus_address);

    void setState(State newState) override;
    State state() override;
    void setColor(uint r, uint g, uint b) override;
    int onMillisec() override;
    void setOnMillisec(int onMs) override;
    int offMillisec() override;
    void setOffMillisec(int offMs) override;

    void start_processing() override;
    void notify_battery_info(BatteryInfo *) override;
    void notify_display_state(DisplayState) override;

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
    void dbus_method_call(
        GDBusConnection* connection,
        gchar const* sender,
        gchar const* object_path,
        gchar const* interface_name,
        gchar const* method_name,
        GVariant* parameters,
        GDBusMethodInvocation* invocation);
    void dbus_unknown_method(std::string const& sender, std::string const& name);

    std::shared_ptr<Log> const log;

    DBusConnectionHandle dbus_connection;
    DBusEventLoop dbus_event_loop;
    HandlerRegistration dbus_signal_handler_registration;
    HandlerRegistration name_owner_changed_handler_registration;


    BatteryInfo batteryInfo;
    DisplayState displayState;

    void updateLight();
    void updateLight(light_state_t * lightState);
    void update_light_state();

    light_state_t indicatorLightStates[LIS_NUM_ITEMS];
    bool lightEventsActive[LE_NUM_ITEMS];  // 1 for active, 0 for inactive
    bool lightEventsEnabled[LE_NUM_ITEMS]; // 1 for enabled (used), 0 for disabled (ignored)

    // These need to be at the end, so that handlers are unregistered first on
    // destruction, to avoid accessing other members if an event arrives
    // on destruction.
    HandlerRegistration lightcontrol_handler_registration;

};

}
