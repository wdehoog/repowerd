#include <cstring>

#include "light_control.h"

#include "src/core/log.h"

namespace
{
char const* const log_tag = "UBPortsLightControl";
char const* const dbus_lightcontrol_name = "com.ubports.lightcontrol";
char const* const dbus_lightcontrol_path = "/com/ubports/lightcontrol";
char const* const dbus_lightcontrol_interface = "com.ubports.lightcontrol";
}
char const* const lightcontrol_service_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
  <interface name='com.ubports.lightcontrol.'>
    <!-- 
        notifyLightEvent:
        @event the LightEvent to notify: UnreadNotifications, BluetoothEnabled, BatteryLow, BatteryCharging, BatteryFull 
        @active 1 for active, 0 for inactive

        Notify a LightEvent. 
    -->
    <method name='notifyLightEvent'>
      <arg name='event' type='s' direction='in'/>
      <arg name='active' type='i' direction='in'/>
    </method>
  </interface>
</node>)";

repowerd::UBPortsLightControl::UBPortsLightControl(
   std::shared_ptr<Log> const& log,
   std::string const& dbus_bus_address)
  : m_lightDevice(0),
    m_state(State::Off),
    log{log},
    dbus_connection{dbus_bus_address},
    displayState(DisplayState::DisplayUnknown) {
    log->log(log_tag, "contructor");

    memset(indicatorLightStates, 0, sizeof(indicatorLightStates));
    memset(lightEventsActive, 0, sizeof(lightEventsActive));

    // ToDo: load from DeviceConfig
    indicatorLightStates[BatteryCharging].color = 0xFFFFFF; // white
    indicatorLightStates[BatteryCharging].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[BatteryCharging].flashOnMS = 2000;
    indicatorLightStates[BatteryCharging].flashOffMS = 1000;
    indicatorLightStates[BatteryCharging].brightnessMode = BRIGHTNESS_MODE_USER;

    indicatorLightStates[BatteryFull].color = 0x00FF00; // lime green
    indicatorLightStates[BatteryFull].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[BatteryFull].flashOnMS = 1000;
    indicatorLightStates[BatteryFull].flashOffMS = 0;
    indicatorLightStates[BatteryFull].brightnessMode = BRIGHTNESS_MODE_USER;

    indicatorLightStates[MessagePending].color = 0x006400; // dark green
    indicatorLightStates[MessagePending].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[MessagePending].flashOnMS = 1000;
    indicatorLightStates[MessagePending].flashOffMS = 0;
    indicatorLightStates[MessagePending].brightnessMode = BRIGHTNESS_MODE_USER;
}

void repowerd::UBPortsLightControl::start_processing()
{
    log->log(log_tag, "start_processing");

    // call init() here?

    lightcontrol_handler_registration = dbus_event_loop.register_object_handler(
        dbus_connection,
        dbus_lightcontrol_path,
        lightcontrol_service_introspection,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* method_name,
            GVariant* parameters,
            GDBusMethodInvocation* invocation)
        {
            dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });

    /*dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_im_name,
        dbus_im_interface,
        nullptr,
        dbus_im_path,
        [this] (
            GDBusConnection* connection,
            gchar const* sender,
            gchar const* object_path,
            gchar const* interface_name,
            gchar const* signal_name,
            GVariant* parameters)
        {
            handle_dbus_signal(
                connection, sender, object_path, interface_name,
                signal_name, parameters);
        });*/
}

void repowerd::UBPortsLightControl::dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* sender_cstr,
    gchar const* /*object_path_cstr*/,
    gchar const* /*interface_name_cstr*/,
    gchar const* method_name_cstr,
    GVariant* parameters,
    GDBusMethodInvocation* invocation)
{
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};

    if (method_name == "notifyLightEvent")
    {
        char const* event{""};
        int32_t active{-1};
        g_variant_get(parameters, "(&si)", &event, &active);
        if(strcmp(event, "UnreadNotifications")!=0)
            lightEventsActive[LE_UnreadNotifications] = active>0;
        else if(strcmp(event, "BluetoothEnabled")!=0)
            lightEventsActive[LE_BluetoothEnabled] = active>0;
        else if(strcmp(event, "BatteryLow")!=0)
            lightEventsActive[LE_BatteryLow] = active>0;
        else if(strcmp(event, "BatteryCharging")!=0)
            lightEventsActive[LE_BatteryCharging] = active>0;
        else if(strcmp(event, "BatteryFull")!=0)
            lightEventsActive[LE_BatteryFull] = active>0;

        update_light_state();

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        dbus_unknown_method(sender, method_name);

        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }

}

void repowerd::UBPortsLightControl::dbus_unknown_method(
    std::string const& sender, std::string const& name)
{
    log->log(log_tag, "dbus_unknown_method(%s,%s)", sender.c_str(), name.c_str());
}

void repowerd::UBPortsLightControl::handle_dbus_signal(
    GDBusConnection*, // connection
    gchar const* ,    // sender
    gchar const* ,    // object_path
    gchar const* ,    // interface_name
    gchar const* signal_name_cstr,
    GVariant* )       // parameters
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    //if (signal_name == "PropertiesChanged") {
    //    log->log(log_tag, "dbus signal PropertiesChanged");
	// 
    //}
        
}

void repowerd::UBPortsLightControl::setState(State newState) {
    if (!init()) {
        log->log(log_tag, "setState: No lights device");
        return;
    }

    if (m_state != newState) {
        if (newState == UBPortsLightControl::On) {
            turnOn();
        } else {
            turnOff();
        }
        m_state = newState;
    }
}

repowerd::UBPortsLightControl::State repowerd::UBPortsLightControl::state() {
    return m_state;
}

void repowerd::UBPortsLightControl::updateLight() {
    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.color = m_color;
    state.flashMode = LIGHT_FLASH_TIMED;
    state.flashOnMS = m_onMs;
    state.flashOffMS = m_offMs;
    state.brightnessMode = BRIGHTNESS_MODE_USER;
    updateLight(&state);
}

void repowerd::UBPortsLightControl::updateLight(light_state_t * lightState) {
    log->log(log_tag, "updateLight");
    if (!init()) {
        log->log(log_tag, "  No lights device");
        return;
    }
    if (m_lightDevice->set_light(m_lightDevice, lightState) != 0) {
	    log->log(log_tag, "Failed to update the light");
    } else
      m_state = lightState->flashMode != LIGHT_FLASH_NONE 
          ? UBPortsLightControl::On 
          : UBPortsLightControl::Off;
}

void repowerd::UBPortsLightControl::setColor(uint r, uint g, uint b) {
    log->log(log_tag, "setColor");
    uint color = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
    if (m_color != color) {
        m_color = color;
        if (m_state == UBPortsLightControl::On)
            updateLight();
    }
}

int repowerd::UBPortsLightControl::onMillisec() {
    return m_onMs;
}

void repowerd::UBPortsLightControl::setOnMillisec(int onMs) {
    if (m_onMs != onMs) {
        m_onMs = onMs;
        if (m_state == UBPortsLightControl::On)
            updateLight();
    }
}

int repowerd::UBPortsLightControl::offMillisec() {
    return m_offMs;
}

void repowerd::UBPortsLightControl::setOffMillisec(int offMs) {
    if (m_offMs != offMs) {
        m_offMs = offMs;
        if (m_state == UBPortsLightControl::On)
            updateLight();
    }
}

bool repowerd::UBPortsLightControl::init() {
    if (m_lightDevice) {
        return true;
    }

    int err;
    hw_module_t* module;

    err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, LIGHT_ID_NOTIFICATIONS, &device);
        if (err == 0) {
            m_lightDevice = (light_device_t*)device;
            turnOff();
            return true;
        } else {
            log->log(log_tag, "Failed to access notification lights");
        }
    } else {
        log->log(log_tag, "Failed to initialize lights hardware.");
    }
    return false;
}

void repowerd::UBPortsLightControl::turnOff() {
    log->log(log_tag, "turnOff");
    if (!init()) {
        log->log(log_tag, "  No lights device");
        return;
    }
    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.color = 0x00000000;
    state.flashMode = LIGHT_FLASH_NONE;
    state.flashOnMS = 0;
    state.flashOffMS = 0;
    state.brightnessMode = 0;

    if (m_lightDevice->set_light(m_lightDevice, &state) != 0) {
        log->log(log_tag, "Failed to turn the light off");
    }
}

void repowerd::UBPortsLightControl::turnOn() {
    log->log(log_tag, "turnOn");
    if (!init()) {
        log->log(log_tag, "  No lights device");
        return;
    }
    // pulse
    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.color = m_color;
    state.flashMode = LIGHT_FLASH_TIMED;
    state.flashOnMS = m_onMs;
    state.flashOffMS = m_offMs;
    state.brightnessMode = BRIGHTNESS_MODE_USER;

    if (m_lightDevice->set_light(m_lightDevice, &state) != 0) {
        log->log(log_tag, "Failed to turn the light on");
    }
}

void repowerd::UBPortsLightControl::notify_battery_info(BatteryInfo * batteryInfo) {
    log->log(log_tag, "notify_battery_info state: %d, percentage: %f", batteryInfo->state, batteryInfo->percentage);
    this->batteryInfo.is_present = batteryInfo->is_present;
    this->batteryInfo.state = batteryInfo->state;
    this->batteryInfo.percentage = batteryInfo->percentage;
    this->batteryInfo.temperature = batteryInfo->temperature;
    lightEventsActive[LE_BatteryCharging] = batteryInfo->state == 1; // charging
    lightEventsActive[LE_BatteryLow] = batteryInfo->percentage < 10;
    lightEventsActive[LE_BatteryFull] = batteryInfo->percentage >= 100;
    update_light_state();
}

void repowerd::UBPortsLightControl::notify_display_state(DisplayState displayState) {
    log->log(log_tag, "notify_display_state %d", displayState);
    this->displayState = displayState;
    update_light_state();
}

void repowerd::UBPortsLightControl::update_light_state() {
    log->log(log_tag, "update_light_state ds: %d, bs: %d", displayState, batteryInfo.state);

    // show charging and full but only when display is off
    if(displayState == DisplayOff) {
        //if(lightEventsActive[LE_BatteryLow])
        //    updateLight(&indicatorLightStates[BatteryFull]);
        //else
        if(lightEventsActive[LE_BatteryFull])
            updateLight(&indicatorLightStates[BatteryFull]);
        else if(lightEventsActive[LE_BatteryCharging])
            updateLight(&indicatorLightStates[BatteryCharging]);
        else
            setState(LightControl::State::Off);
    } else
        setState(LightControl::State::Off);
}
