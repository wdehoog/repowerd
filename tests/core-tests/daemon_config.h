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

#include "src/core/daemon_config.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class MockBrightnessControl;
class FakeClientRequests;
class MockDisplayPowerControl;
class MockDisplayPowerEventSink;
class FakeLog;
class MockModemPowerControl;
class FakeNotificationService;
class MockPerformanceBooster;
class FakePowerButton;
class MockPowerButtonEventSink;
class FakePowerSource;
class FakeProximitySensor;
class MockShutdownControl;
class FakeSuspendControl;
class FakeTimer;
class FakeUserActivity;
class FakeVoiceCallService;

class DaemonConfig : public repowerd::DaemonConfig
{
public:
    std::shared_ptr<BrightnessControl> the_brightness_control() override;
    std::shared_ptr<ClientRequests> the_client_requests() override;
    std::shared_ptr<DisplayPowerControl> the_display_power_control() override;
    std::shared_ptr<DisplayPowerEventSink> the_display_power_event_sink() override;
    std::shared_ptr<Log> the_log() override;
    std::shared_ptr<ModemPowerControl> the_modem_power_control() override;
    std::shared_ptr<LightControl> the_light_control() override;
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

    std::shared_ptr<testing::NiceMock<MockBrightnessControl>> the_mock_brightness_control();
    std::shared_ptr<FakeClientRequests> the_fake_client_requests();
    std::shared_ptr<testing::NiceMock<MockDisplayPowerControl>> the_mock_display_power_control();
    std::shared_ptr<testing::NiceMock<MockDisplayPowerEventSink>> the_mock_display_power_event_sink();
    std::shared_ptr<FakeLog> the_fake_log();
    std::shared_ptr<testing::NiceMock<MockModemPowerControl>> the_mock_modem_power_control();
    std::shared_ptr<FakeNotificationService> the_fake_notification_service();
    std::shared_ptr<testing::NiceMock<MockPerformanceBooster>> the_mock_performance_booster();
    std::shared_ptr<FakePowerButton> the_fake_power_button();
    std::shared_ptr<testing::NiceMock<MockPowerButtonEventSink>> the_mock_power_button_event_sink();
    std::shared_ptr<FakePowerSource> the_fake_power_source();
    std::shared_ptr<FakeProximitySensor> the_fake_proximity_sensor();
    std::shared_ptr<testing::NiceMock<MockShutdownControl>> the_mock_shutdown_control();
    std::shared_ptr<FakeSuspendControl> the_fake_suspend_control();
    std::shared_ptr<FakeTimer> the_fake_timer();
    std::shared_ptr<FakeUserActivity> the_fake_user_activity();
    std::shared_ptr<FakeVoiceCallService> the_fake_voice_call_service();

private:
    std::shared_ptr<StateMachine> state_machine;

    std::shared_ptr<testing::NiceMock<MockBrightnessControl>> mock_brightness_control;
    std::shared_ptr<FakeClientRequests> fake_client_requests;
    std::shared_ptr<testing::NiceMock<MockDisplayPowerControl>> mock_display_power_control;
    std::shared_ptr<testing::NiceMock<MockDisplayPowerEventSink>> mock_display_power_event_sink;
    std::shared_ptr<FakeLog> fake_log;
    std::shared_ptr<testing::NiceMock<MockModemPowerControl>> mock_modem_power_control;
    std::shared_ptr<FakeNotificationService> fake_notification_service;
    std::shared_ptr<testing::NiceMock<MockPerformanceBooster>> mock_performance_booster;
    std::shared_ptr<FakePowerButton> fake_power_button;
    std::shared_ptr<testing::NiceMock<MockPowerButtonEventSink>> mock_power_button_event_sink;
    std::shared_ptr<FakePowerSource> fake_power_source;
    std::shared_ptr<FakeProximitySensor> fake_proximity_sensor;
    std::shared_ptr<testing::NiceMock<MockShutdownControl>> mock_shutdown_control;
    std::shared_ptr<FakeSuspendControl> fake_suspend_control;
    std::shared_ptr<FakeTimer> fake_timer;
    std::shared_ptr<FakeUserActivity> fake_user_activity;
    std::shared_ptr<FakeVoiceCallService> fake_voice_call_service;
};

}
}
