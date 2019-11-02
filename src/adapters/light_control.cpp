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
        notifyLightEvent:
        @event the LightEvent to notify: UnreadNotifications, BluetoothEnabled, BatteryLow, BatteryCharging, BatteryFull, Playing 
        @active 1 for active, 0 for inactive

        Notify a LightEvent. 
    -->
    <method name='notifyLightEvent'>
      <arg name='event' type='s' direction='in'/>
      <arg name='active' type='i' direction='in'/>
    </method>
    <!-- 
        enableLightEvent:
        @event the LightEvent to notify: see above

        Enable a LightEvent. When enabled the event state (active/inactive) will be used to determine the state of the led. 
    -->
    <method name='enableLightEvent'>
      <arg name='event' type='s' direction='in'/>
    </method>
    <!-- 
        disableLightEvent:
        @event the LightEvent to notify: see above

        Disable a LightEvent. When disabled the event state (active/inactive) will be ignored.
    -->
    <method name='disableLightEvent'>
      <arg name='event' type='s' direction='in'/>
    </method>
    <!-- 
        setPlayingData:
        @color the rgb hex string of the color to use. Example: 0xFF1234
        @onMS time (ms) the led must be on while pulsing
        @offMS time (ms) the led must be off while pulsing

        Play with the led. Enable the event (Playing) and set the color and pulse rates.
    -->
    <method name='setPlayingData'>
      <arg name='color' type='s' direction='in'/>
      <arg name='onMS' type='i' direction='in'/>
      <arg name='offMS' type='i' direction='in'/>
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
    memset(lightEventsActive, 0, sizeof(lightEventsActive)); // inactive
    memset(lightEventsEnabled, 1, sizeof(lightEventsEnabled)); // enabled

    // ToDo: load from DeviceConfig of System xxx
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

    indicatorLightStates[BatteryLow].color = 0xFF0000; // red
    indicatorLightStates[BatteryLow].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[BatteryLow].flashOnMS = 1000;
    indicatorLightStates[BatteryLow].flashOffMS = 0;
    indicatorLightStates[BatteryLow].brightnessMode = BRIGHTNESS_MODE_USER;

    indicatorLightStates[UnreadNotifications].color = 0x006400; // dark green
    indicatorLightStates[UnreadNotifications].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[UnreadNotifications].flashOnMS = 1000;
    indicatorLightStates[UnreadNotifications].flashOffMS = 3000;
    indicatorLightStates[UnreadNotifications].brightnessMode = BRIGHTNESS_MODE_USER;

    indicatorLightStates[BluetoothEnabled].color = 0x0000FF; // blue
    indicatorLightStates[BluetoothEnabled].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[BluetoothEnabled].flashOnMS = 1000;
    indicatorLightStates[BluetoothEnabled].flashOffMS = 0;
    indicatorLightStates[BluetoothEnabled].brightnessMode = BRIGHTNESS_MODE_USER;

    indicatorLightStates[Playing].color = 0x0000FF; // blue
    indicatorLightStates[Playing].flashMode = LIGHT_FLASH_TIMED;
    indicatorLightStates[Playing].flashOnMS = 1000;
    indicatorLightStates[Playing].flashOffMS = 500;
    indicatorLightStates[Playing].brightnessMode = BRIGHTNESS_MODE_USER;
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

    name_owner_changed_handler_registration = dbus_event_loop.register_signal_handler(
        dbus_connection,
        "org.freedesktop.DBus",
        "org.freedesktop.DBus",
        "NameOwnerChanged",
        "/org/freedesktop/DBus",
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
        });

    try {
        dbus_connection.request_name(dbus_lightcontrol_servicename);
    } catch (...) {
        log->log(log_tag, "Exception while requesting dbus name: %s", dbus_lightcontrol_servicename);
    }

    // listen to session bus
    char * sbAddress = getenv("DBUS_SESSION_BUS_ADDRESS");
    log->log(log_tag, "dbus session address: %s", sbAddress);
    if(sbAddress != NULL && strlen(sbAddress) > 0) {

        try {
            dbus_session_connection = new DBusConnectionHandle(sbAddress);
            if(dbus_session_connection == NULL) {
                log->log(log_tag, "faile to create new DBusConnectionHandle for session bus");
            } else {

                dbus_signal_handler_registration = dbus_event_loop.register_signal_handler(
                        *dbus_session_connection,
                        NULL, //"com.canonical.indicator.messages",  // bus name
                        "org.gtk.Actions",                   // interface
                        "Changed",                           // signal 
                        "/com/canonical/indicator/messages", // path
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
                        });

            }
        } catch (std::exception& e) {
            log->log(log_tag, "Exception while registering signal handler for session bus: %s", e.what());
        }
    }
    
}

static repowerd::UBPortsLightControl::LightEvent getLightEventFromString(char const * str) {
    if(!strcmp(str, "UnreadNotifications"))
        return repowerd::UBPortsLightControl::LE_UnreadNotifications;
    else if(!strcmp(str, "BluetoothEnabled"))
        return repowerd::UBPortsLightControl::LE_BluetoothEnabled;
    else if(!strcmp(str, "BatteryLow"))
        return repowerd::UBPortsLightControl::LE_BatteryLow;
    else if(!strcmp(str, "BatteryCharging"))
        return repowerd::UBPortsLightControl::LE_BatteryCharging;
    else if(!strcmp(str, "BatteryFull"))
        return repowerd::UBPortsLightControl::LE_BatteryFull;
    else if(!strcmp(str, "Playing"))
        return repowerd::UBPortsLightControl::LE_Playing;
    else
        return repowerd::UBPortsLightControl::LE_NUM_ITEMS;
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
    bool requestOk = false;
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const method_name{method_name_cstr ? method_name_cstr : ""};

    if (method_name == "notifyLightEvent")
    {
        char const* event{""};
        int32_t active{-1};
        g_variant_get(parameters, "(&si)", &event, &active);
        log->log(log_tag, "dbus_method_call(%s, %s, %d)", method_name_cstr, event, active);
        LightEvent lev = getLightEventFromString(event);
        if(lev < LE_NUM_ITEMS)
            lightEventsActive[lev] = active>0;
        requestOk = true;
    }
    else if (method_name == "enableLightEvent")
    {
        char const* event{""};
        g_variant_get(parameters, "(&s)", &event);
        log->log(log_tag, "dbus_method_call(%s, %s)", method_name_cstr, event);
        LightEvent lev = getLightEventFromString(event);
        if(lev < LE_NUM_ITEMS)
            lightEventsEnabled[lev] = 1;
        requestOk = true;
    }
    else if (method_name == "disableLightEvent")
    {
        char const* event{""};
        g_variant_get(parameters, "(&s)", &event);
        log->log(log_tag, "dbus_method_call(%s, %s)", method_name_cstr, event);
        LightEvent lev = getLightEventFromString(event);
        if(lev < LE_NUM_ITEMS)
            lightEventsEnabled[lev] = 0;
        requestOk = true;
    }
    else if (method_name == "setPlayingData")
    {
        char const* color{""};
        int32_t onMS{-1};
        int32_t offMS{-1};
        g_variant_get(parameters, "(&sii)", &color, &onMS, &offMS);
        log->log(log_tag, "dbus_method_call(%s, %s, %d, %d)", method_name_cstr, color, onMS, offMS);

        std::stringstream sStream;
        sStream << std::hex << color;
        sStream >> indicatorLightStates[LE_Playing].color;
        indicatorLightStates[LE_Playing].flashOnMS = onMS;
        indicatorLightStates[LE_Playing].flashOffMS = offMS;

        requestOk = true;
    }

    if(requestOk)
    {
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

static bool hasNewMessage = false;

void repowerd::UBPortsLightControl::handle_dbus_signal(
    GDBusConnection*, // connection
    gchar const* sender_cstr,
    gchar const* object_path_cstr,
    gchar const* interface_name_cstr,
    gchar const* signal_name_cstr,
    GVariant* parameters)
{
    std::string const sender{sender_cstr ? sender_cstr : ""};
    std::string const object_path{object_path_cstr ? object_path_cstr : ""};
    std::string const interface_name{interface_name_cstr ? interface_name_cstr : ""};
    std::string const signal_name{signal_name_cstr ? signal_name_cstr : ""};

    log->log(log_tag, "handle_dbus_signal %s:%s:%s:%s", sender_cstr, object_path_cstr, interface_name_cstr, signal_name_cstr);
    if (sender == "org.freedesktop.DBus" &&
        object_path == "/org/freedesktop/DBus" &&
        interface_name == "org.freedesktop.DBus" &&
        signal_name == "NameOwnerChanged")
    {
        char const* name = "";
        char const* old_owner = "";
        char const* new_owner = "";
        g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);

        log->log(log_tag, "dbus owner changed(%s,%s,%s)",
                 name, old_owner, new_owner);
    } 
    else if (//sender == "com.canonical.indicator.messages" && why is this a number like 1.54?
        object_path == "/com/canonical/indicator/messages" &&
        interface_name == "org.gtk.Actions" &&
        signal_name == "Changed")
    {
        // dump 1
        //log->log(log_tag, "print: %s", g_variant_print(parameters, true));
        // dump and parse
        gchar *vs;
        for(gsize i = 0; i < g_variant_n_children(parameters); i++) {
            g_autoptr(GVariant) child = g_variant_get_child_value (parameters, i);
            vs = g_variant_print(child, true);
            log->log(log_tag, "  Child %" G_GSIZE_FORMAT ": %s -> %s", i, g_variant_get_type_string (child), vs);
            vs = g_variant_print(child, true);
            if(strstr(vs, "<{'icon':") != NULL || strstr(vs, "<{'icons':") != NULL) {
                hasNewMessage = strstr(vs, "indicator-messages-new") != NULL;
            }
        }

        // load icons
        /*GVariantIter iter;
        GVariant *vvalue;
        gchar *key, *vs;
        bool hasIcon; 
        bool hasNewMessage = false; 

        g_variant_iter_init (&iter, parameters);
        while (g_variant_iter_loop (&iter, "{sv}", &key, &vvalue)) {
            hasIcon = false; 
            if (strcmp(key, "icon")==0)
                hasIcon = true; 
            else if (strcmp(key, "icons")==0)
                hasIcon = true;
            if(hasIcon) {
                vs = g_variant_print(vvalue, true);
                log->log(log_tag, "  icon: %s", vs);
                if(strstr(vs, "indicator-messages-new") != NULL)
                    hasNewMessage = true; 
            }
        }*/

        if(hasNewMessage) 
            log->log(log_tag, "UnreadNotifications ACTIVE");
        else
            log->log(log_tag, "UnreadNotifications INACTIVE");
        lightEventsActive[LE_UnreadNotifications] = hasNewMessage ? 1 : 0;
        update_light_state();

    }
        
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
    //log->log(log_tag, "updateLight");
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
    log->log(log_tag, "update_light_state displayState: %d, batteryState: %d", displayState, batteryInfo.state);
    //for(int i=0;i<LE_NUM_ITEMS;i++) {
    //    log->log(log_tag, "  [%d]: enabled=%d, active=%d", i, lightEventsEnabled[i], lightEventsActive[i]);
    //}

    // show charging and full but only when display is off
    if(displayState == DisplayOff) {
        if(lightEventsEnabled[LE_BatteryLow] && lightEventsActive[LE_BatteryLow])
            updateLight(&indicatorLightStates[BatteryLow]);
        else if(lightEventsEnabled[LE_UnreadNotifications] && lightEventsActive[LE_UnreadNotifications])
            return; // UnreadNotifications is handled by unity updateLight(&indicatorLightStates[UnreadNotifications]);
        else if(lightEventsEnabled[LE_BluetoothEnabled] && lightEventsActive[LE_BluetoothEnabled])
            updateLight(&indicatorLightStates[BluetoothEnabled]);
        else if(lightEventsEnabled[LE_BatteryFull] && lightEventsActive[LE_BatteryFull])
            updateLight(&indicatorLightStates[BatteryFull]);
        else if(lightEventsEnabled[LE_BatteryCharging] && lightEventsActive[LE_BatteryCharging])
            updateLight(&indicatorLightStates[BatteryCharging]);
        else if(lightEventsEnabled[LE_Playing] && lightEventsActive[LE_Playing])
            updateLight(&indicatorLightStates[Playing]);
        else
            setState(LightControl::State::Off);
    } else
        setState(LightControl::State::Off);
}
