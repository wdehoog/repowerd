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

#include "src/default_daemon_config.h"

#include <gmock/gmock.h>

namespace repowerd
{
namespace test
{

class MockBrightnessControl;
class FakeClientRequests;
class MockDisplayPowerControl;
class FakeNotificationService;
class FakePowerButton;
class MockPowerButtonEventSink;
class FakeProximitySensor;
class FakeTimer;
class FakeUserActivity;
class FakeVoiceCallService;

class DaemonConfig : public repowerd::DefaultDaemonConfig
{
public:
    std::shared_ptr<BrightnessControl> the_brightness_control() override;
    std::shared_ptr<ClientRequests> the_client_requests() override;
    std::shared_ptr<DisplayPowerControl> the_display_power_control() override;
    std::shared_ptr<NotificationService> the_notification_service() override;
    std::shared_ptr<PowerButton> the_power_button() override;
    std::shared_ptr<PowerButtonEventSink> the_power_button_event_sink() override;
    std::shared_ptr<ProximitySensor> the_proximity_sensor() override;
    std::shared_ptr<Timer> the_timer() override;
    std::shared_ptr<UserActivity> the_user_activity() override;
    std::shared_ptr<VoiceCallService> the_voice_call_service() override;

    std::shared_ptr<testing::NiceMock<MockBrightnessControl>> the_mock_brightness_control();
    std::shared_ptr<FakeClientRequests> the_fake_client_requests();
    std::shared_ptr<testing::NiceMock<MockDisplayPowerControl>> the_mock_display_power_control();
    std::shared_ptr<FakeNotificationService> the_fake_notification_service();
    std::shared_ptr<FakePowerButton> the_fake_power_button();
    std::shared_ptr<testing::NiceMock<MockPowerButtonEventSink>> the_mock_power_button_event_sink();
    std::shared_ptr<FakeProximitySensor> the_fake_proximity_sensor();
    std::shared_ptr<FakeTimer> the_fake_timer();
    std::shared_ptr<FakeUserActivity> the_fake_user_activity();
    std::shared_ptr<FakeVoiceCallService> the_fake_voice_call_service();

private:
    std::shared_ptr<testing::NiceMock<MockBrightnessControl>> mock_brightness_control;
    std::shared_ptr<FakeClientRequests> fake_client_requests;
    std::shared_ptr<testing::NiceMock<MockDisplayPowerControl>> mock_display_power_control;
    std::shared_ptr<FakeNotificationService> fake_notification_service;
    std::shared_ptr<FakePowerButton> fake_power_button;
    std::shared_ptr<testing::NiceMock<MockPowerButtonEventSink>> mock_power_button_event_sink;
    std::shared_ptr<FakeProximitySensor> fake_proximity_sensor;
    std::shared_ptr<FakeTimer> fake_timer;
    std::shared_ptr<FakeUserActivity> fake_user_activity;
    std::shared_ptr<FakeVoiceCallService> fake_voice_call_service;
};

}
}
