/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#pragma once

#include "core/daemon_config.h"

namespace repowerd
{

class Backlight;
class BacklightBrightnessControl;
class BrightnessNotification;
class Chrono;
class DeviceConfig;
class DeviceQuirks;
class Filesystem;
class LightSensor;
class OfonoVoiceCallService;
class TemporarySuspendInhibition;
class UBPortsLightControl;
class UnityScreenService;
class UnityPowerButton;
class WakeupService;

class DefaultDaemonConfig : public DaemonConfig
{
public:
    std::shared_ptr<BrightnessControl> the_brightness_control() override;
    std::shared_ptr<ClientRequests> the_client_requests() override;
    std::shared_ptr<DisplayPowerControl> the_display_power_control() override;
    std::shared_ptr<DisplayPowerEventSink> the_display_power_event_sink() override;
    std::shared_ptr<Log> the_log() override;
    std::shared_ptr<ModemPowerControl> the_modem_power_control() override;
    std::shared_ptr<NotificationService> the_notification_service() override;
    std::shared_ptr<PerformanceBooster> the_performance_booster() override;
    std::shared_ptr<PowerButton> the_power_button() override;
    std::shared_ptr<PowerButtonEventSink> the_power_button_event_sink() override;
    std::shared_ptr<PowerSource> the_power_source() override;
    std::shared_ptr<ProximitySensor> the_proximity_sensor() override;
    std::shared_ptr<ShutdownControl> the_shutdown_control() override;
    std::shared_ptr<StateMachine> the_state_machine() override;
    std::shared_ptr<SuspendControl> the_suspend_control() override;
    std::shared_ptr<Timer> the_timer() override;
    std::shared_ptr<UserActivity> the_user_activity() override;
    std::shared_ptr<VoiceCallService> the_voice_call_service() override;

    std::chrono::milliseconds notification_expiration_timeout() override;
    std::chrono::milliseconds power_button_long_press_timeout() override;
    std::chrono::milliseconds user_inactivity_normal_display_dim_duration() override;
    std::chrono::milliseconds user_inactivity_normal_display_off_timeout() override;
    std::chrono::milliseconds user_inactivity_post_notification_display_off_timeout() override;
    std::chrono::milliseconds user_inactivity_reduced_display_off_timeout() override;

    bool turn_on_display_at_startup() override;

    std::shared_ptr<Backlight> the_backlight();
    std::shared_ptr<BacklightBrightnessControl> the_backlight_brightness_control();
    std::shared_ptr<BrightnessNotification> the_brightness_notification();
    std::shared_ptr<Chrono> the_chrono();
    std::string the_dbus_bus_address();
    std::shared_ptr<DeviceConfig> the_device_config();
    std::shared_ptr<DeviceQuirks> the_device_quirks();
    std::shared_ptr<Filesystem> the_filesystem();
    std::shared_ptr<LightSensor> the_light_sensor();
    std::shared_ptr<OfonoVoiceCallService> the_ofono_voice_call_service();
    std::shared_ptr<TemporarySuspendInhibition> the_temporary_suspend_inhibition();
    std::shared_ptr<UBPortsLightControl> the_light_control();
    std::shared_ptr<UnityScreenService> the_unity_screen_service();
    std::shared_ptr<UnityPowerButton> the_unity_power_button();
    std::shared_ptr<WakeupService> the_wakeup_service();

private:
    std::shared_ptr<Backlight> backlight;
    std::shared_ptr<BacklightBrightnessControl> backlight_brightness_control;
    std::shared_ptr<BrightnessControl> brightness_control;
    std::shared_ptr<BrightnessNotification> brightness_notification;
    std::shared_ptr<Chrono> chrono;
    std::shared_ptr<DeviceConfig> device_config;
    std::shared_ptr<DeviceQuirks> device_quirks;
    std::shared_ptr<DisplayPowerControl> display_power_control;
    std::shared_ptr<Filesystem> filesystem;
    std::shared_ptr<LightSensor> light_sensor;
    std::shared_ptr<Log> log;
    std::shared_ptr<ModemPowerControl> modem_power_control;
    std::shared_ptr<OfonoVoiceCallService> ofono_voice_call_service;
    std::shared_ptr<PerformanceBooster> performance_booster;
    std::shared_ptr<PowerSource> power_source;
    std::shared_ptr<ProximitySensor> proximity_sensor;
    std::shared_ptr<ShutdownControl> shutdown_control;
    std::shared_ptr<StateMachine> state_machine;
    std::shared_ptr<SuspendControl> suspend_control;
    std::shared_ptr<Timer> timer;
    std::shared_ptr<TemporarySuspendInhibition> temporary_suspend_inhibition;
    std::shared_ptr<UBPortsLightControl> light_control;
    std::shared_ptr<UnityPowerButton> unity_power_button;
    std::shared_ptr<UnityScreenService> unity_screen_service;
    std::shared_ptr<UserActivity> user_activity;
    std::shared_ptr<WakeupService> wakeup_service;
};

}

