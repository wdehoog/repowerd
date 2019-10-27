#include <cstring>

#include "light_control.h"

#include "src/core/log.h"

namespace
{
char const* const log_tag = "UBPortsLightControl";
//char const* const dbus_upstart_name = "com.ubuntu.Upstart";
//char const* const dbus_upstart_path = "/com/ubuntu/Upstart";
//char const* const dbus_upstart_interface = "com.ubuntu.Upstart0_6";
}

repowerd::UBPortsLightControl::UBPortsLightControl(
   std::shared_ptr<Log> const& log)
  : m_lightDevice(0),
    m_state(State::Off),
    log{log} {
    log->log(log_tag, "contructor");
}

/*repowerd::UBPortsLightControl::UBPortsLightControl(
   std::shared_ptr<Log> const& log,
   std::string const& dbus_bus_address)
  : log{log},
    dbus_connection{dbus_bus_address} {
}*/

void repowerd::UBPortsLightControl::start_processing()
{
    log->log(log_tag, "start_processing");
    // call init() here?

    /*dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        dbus_upstart_name,
        dbus_upstart_interface,
        nullptr,
        dbus_upstart_path,
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

/*void repowerd::UBPortsLightControl::handle_dbus_signal(
    GDBusConnection* // connection,
    gchar const* ,   // sender
    gchar const* ,   // object_path
    gchar const* ,   // interface_name
    gchar const* signal_name_cstr,
    GVariant* )      // parameters
{
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    if (signal_name == "PropertiesChanged") {
        log->log(log_tag, "dbus signal PropertiesChanged");
	// get new battery info from UPowerSource
    }
        
}*/

void repowerd::UBPortsLightControl::setState(State newState) {
    if (!init()) {
	log->log(log_tag, "No lights device");
        return;
    }

    if (m_state != newState) {
        if (newState == UBPortsLightControl::On) {
            turnOn();
        } else {
            turnOff();
        }

        m_state = newState;
        //Q_EMIT stateChanged(m_state);
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
    if (m_lightDevice->set_light(m_lightDevice, &state) != 0) {
	log->log(log_tag, "Failed to update the light");
    }
}

void repowerd::UBPortsLightControl::setColor(int r, int g, int b) {
    uint color = (0xff << 24) | (r << 16) | (g << 8) | (b << 0);
    if (m_color != color) {
        m_color = color;
        if (m_state == UBPortsLightControl::On)
	    updateLight();
        //Q_EMIT colorChanged(m_color);
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
        //Q_EMIT onMillisecChanged(m_onMs);
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
        //Q_EMIT offMillisecChanged(m_offMs);
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

