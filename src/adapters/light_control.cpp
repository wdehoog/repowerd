#include <cstring>
#include <sstream>

#include "light_control.h"

#include "src/core/log.h"

namespace
{
char const* const log_tag = "UBPortsLightControl";
char const* const dbus_lightcontrol_servicename = "com.ubports.lightcontrol";
char const* const dbus_lightcontrol_path = "/com/ubports/lightcontrol";
char const* const dbus_lightcontrol_interface = "com.ubports.lightcontrol";
}
char const* const lightcontrol_service_introspection = R"(<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN' 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>
<node>
  <interface name='com.ubports.lightcontrol'>
    <!-- 
        set_led_attributes:
        @color the rgb value of the color to use. Must be a hex string (examples: 0xFF1234, FF1234);
        @on_ms time (ms) the led must be on while pulsing
        @off_ms time (ms) the led must be off while pulsing. Set to 0 to have the led continuously on.

        Set some attributes of the led. 
    -->
    <method name='set_led_attributes'>
      <arg name='color' type='s' direction='in'/>
      <arg name='on_ms' type='i' direction='in'/>
      <arg name='off_ms' type='i' direction='in'/>
    </method>
    <!-- 
        turn_led_on

        Turn the led on.
    -->
    <method name='turm_led_on'>
    </method>
    <!-- 
        turn_led_off

        Turn the led off.
    -->
    <method name='turn_led_off'>
    </method>
  </interface>
</node>)";

repowerd::UBPortsLightControl::UBPortsLightControl(
   std::shared_ptr<Log> const& log,
   std::string const& dbus_bus_address)
  : m_light_device(0),
    m_state(LedState::Off),
    dbus_connection{dbus_bus_address},
    dbus_event_loop{"UBPortsLightControl"},
    log{log} {
    log->log(log_tag, "contructor");
}

void repowerd::UBPortsLightControl::start_processing()
{
    log->log(log_tag, "start_processing");

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
            handle_dbus_method_call(
                connection, sender, object_path, interface_name,
                method_name, parameters, invocation);
        });

    try {
        dbus_connection.request_name(dbus_lightcontrol_servicename);
    } catch (...) {
        log->log(log_tag, "Exception while requesting dbus name: %s", dbus_lightcontrol_servicename);
    }

}

void repowerd::UBPortsLightControl::handle_dbus_method_call(
    GDBusConnection* /*connection*/,
    gchar const* sender_cstr,
    gchar const* /*object_path_cstr*/,
    gchar const* /*interface_name_cstr*/,
    gchar const* method_name_cstr,
    GVariant* parameters,
    GDBusMethodInvocation* invocation)
{
    bool requestOk = false;
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};

    if (method_name == "set_led_attributes")
    {
        char const* color{""};
        int32_t onMS{-1};
        int32_t offMS{-1};
        g_variant_get(parameters, "(&sii)", &color, &onMS, &offMS);
        log->log(log_tag, "dbus_method_call(%s, %s, %d, %d)", method_name_cstr, color, onMS, offMS);

        std::stringstream sStream;
        sStream << std::hex << color;
        sStream >> m_color;
        m_on_ms = onMS;
        m_off_ms = offMS;

        if (m_state == LedState::On)
          update_light();

        requestOk = true;
    }
    else if (method_name == "turn_led_on")
    {
        turn_on();
        requestOk = true;
    }
    else if (method_name == "turn_led_on")
    {
        turn_off();
        requestOk = true;
    }

    if(requestOk)
    {
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        log->log(log_tag, "dbus_unknown_method(%s,%s)", sender.c_str(), method_name.c_str());

        g_dbus_method_invocation_return_error_literal(
            invocation, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED, "");
    }
}

void repowerd::UBPortsLightControl::set_state(LedState new_state) {
    if (!init()) {
        log->log(log_tag, "setState: No lights device");
        return;
    }

    if (m_state != new_state) {
        if (new_state == LedState::On) {
            turn_on();
        } else {
            turn_off();
        }
        m_state = new_state;
    }
}

repowerd::UBPortsLightControl::LedState repowerd::UBPortsLightControl::get_state() {
    return m_state;
}

void repowerd::UBPortsLightControl::update_light() {
    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.color = m_color;
    state.flashMode = LIGHT_FLASH_TIMED;
    state.flashOnMS = m_on_ms;
    state.flashOffMS = m_off_ms;
    state.brightnessMode = BRIGHTNESS_MODE_USER;
    update_light(&state);
}

void repowerd::UBPortsLightControl::update_light(light_state_t * lightState) {
    //log->log(log_tag, "update_light");
    if (!init()) {
        log->log(log_tag, "  No lights device");
        return;
    }

    if (m_light_device->set_light(m_light_device, lightState) != 0) {
	    log->log(log_tag, "Failed to update the light");
    } else {
      m_state = lightState->flashMode != LIGHT_FLASH_NONE 
          ? LedState::On 
          : LedState::Off;
    }
}

void repowerd::UBPortsLightControl::set_color(uint r, uint g, uint b) {
    log->log(log_tag, "setColor");
    uint color = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
    if (m_color != color) {
        m_color = color;
        if (m_state == LedState::On)
            update_light();
    }
}

int repowerd::UBPortsLightControl::get_on_ms() {
    return m_on_ms;
}

void repowerd::UBPortsLightControl::set_on_ms(int on_ms) {
    if (m_on_ms != on_ms) {
        m_on_ms = on_ms;
        if (m_state == LedState::On)
            update_light();
    }
}

int repowerd::UBPortsLightControl::get_off_ms() {
    return m_off_ms;
}

void repowerd::UBPortsLightControl::set_off_ms(int off_ms) {
    if (m_off_ms != off_ms) {
        m_off_ms = off_ms;
        if (m_state == LedState::On)
            update_light();
    }
}

bool repowerd::UBPortsLightControl::init() {
    if (m_light_device) {
        return true;
    }

    int err;
    hw_module_t* module;

    err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, LIGHT_ID_NOTIFICATIONS, &device);
        if (err == 0) {
            m_light_device = (light_device_t*)device;
            turn_off();
            return true;
        } else {
            log->log(log_tag, "Failed to access notification lights");
        }
    } else {
        log->log(log_tag, "Failed to initialize lights hardware.");
    }
    return false;
}

void repowerd::UBPortsLightControl::turn_off() {
    log->log(log_tag, "turnOff");
    light_state_t state;
    memset(&state, 0, sizeof(light_state_t));
    state.color = 0;
    state.flashMode = LIGHT_FLASH_NONE;
    state.flashOnMS = 0;
    state.flashOffMS = 0;
    state.brightnessMode = 0;
    update_light(&state);
}

void repowerd::UBPortsLightControl::turn_on() {
    log->log(log_tag, "turnOn");
    update_light();
}

