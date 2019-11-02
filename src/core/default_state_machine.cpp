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

#include "default_state_machine.h"

#include "display_information.h"
#include "brightness_control.h"
#include "display_power_control.h"
#include "display_power_event_sink.h"
#include "infinite_timeout.h"
#include "light_control.h"
#include "log.h"
#include "modem_power_control.h"
#include "performance_booster.h"
#include "power_button_event_sink.h"
#include "power_source.h"
#include "proximity_sensor.h"
#include "state_machine_options.h"
#include "system_power_control.h"
#include "timer.h"

namespace
{
char const* const suspend_id = "DefaultStateMachine";

std::string power_action_to_str(repowerd::PowerAction power_action)
{
    if (power_action == repowerd::PowerAction::none)
        return "none";
    else if (power_action == repowerd::PowerAction::display_off)
        return "display_off";
    else if (power_action == repowerd::PowerAction::suspend)
        return "suspend";
    else if (power_action == repowerd::PowerAction::power_off)
        return "power_off";

    return "unknown";
}

std::string power_supply_to_str(repowerd::PowerSupply power_supply)
{
    if (power_supply == repowerd::PowerSupply::battery)
        return "battery";
    else if (power_supply == repowerd::PowerSupply::line_power)
        return "line_power";

    return "unknown";
}

}

repowerd::DefaultStateMachine::DefaultStateMachine(
    DaemonConfig& config,
    std::string const& name)
    : log_tag_str{std::string{"DefaultStateMachine["} + name + "]"},
      log_tag{log_tag_str.c_str()},
      display_information{config.the_display_information()},
      brightness_control{config.the_brightness_control()},
      display_power_control{config.the_display_power_control()},
      display_power_event_sink{config.the_display_power_event_sink()},
      light_control{config.the_light_control()},
      log{config.the_log()},
      modem_power_control{config.the_modem_power_control()},
      performance_booster{config.the_performance_booster()},
      power_button_event_sink{config.the_power_button_event_sink()},
      power_source{config.the_power_source()},
      proximity_sensor{config.the_proximity_sensor()},
      system_power_control{config.the_system_power_control()},
      timer{config.the_timer()},
      display_power_mode{DisplayPowerMode::off},
      display_power_mode_at_power_button_press{DisplayPowerMode::unknown},
      display_power_mode_reason{DisplayPowerChangeReason::unknown},
      power_button_long_press_alarm_id{AlarmId::invalid},
      power_button_long_press_detected{false},
      power_button_long_press_timeout{config.the_state_machine_options()->power_button_long_press_timeout()},
      user_inactivity_display_dim_alarm_id{AlarmId::invalid},
      user_inactivity_display_off_alarm_id{AlarmId::invalid},
      user_inactivity_normal_display_dim_duration{
          config.the_state_machine_options()->user_inactivity_normal_display_dim_duration()},
      user_inactivity_normal_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_normal_display_off_timeout(),
          config.the_state_machine_options()->user_inactivity_normal_display_off_timeout(),
          true},
      user_inactivity_normal_suspend_timeout{
          config.the_state_machine_options()->user_inactivity_normal_suspend_timeout(),
          config.the_state_machine_options()->user_inactivity_normal_suspend_timeout(),
          true},
      user_inactivity_reduced_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_reduced_display_off_timeout()},
      user_inactivity_post_notification_display_off_timeout{
          config.the_state_machine_options()->user_inactivity_post_notification_display_off_timeout()},
      notification_expiration_timeout{
          config.the_state_machine_options()->notification_expiration_timeout()},
      treat_power_button_as_user_activity{
          config.the_state_machine_options()->treat_power_button_as_user_activity()},
      turn_on_display_at_startup{
          config.the_state_machine_options()->turn_on_display_at_startup()},
      scheduled_timeout_type{ScheduledTimeoutType::none},
      lid_power_action{PowerAction::suspend, PowerAction::suspend, true},
      critical_power_action{PowerAction::power_off},
      paused{false},
      autobrightness_enabled{false},
      normal_brightness_value{0.5},
      lid_closed{false},
      suspend_allowed{true},
      suspend_pending{false}
{
      inactivity_timeout_allowances.fill(true);
      proximity_enablements.fill(false);
}

void repowerd::DefaultStateMachine::handle_alarm(AlarmId id)
{
    if (id == power_button_long_press_alarm_id)
    {
        log->log(log_tag, "handle_alarm(long_press)");
        power_button_event_sink->notify_long_press();
        power_button_long_press_detected = true;
        power_button_long_press_alarm_id = AlarmId::invalid;
    }
    else if (id == user_inactivity_display_dim_alarm_id)
    {
        log->log(log_tag, "handle_alarm(display_dim)");
        user_inactivity_display_dim_alarm_id = AlarmId::invalid;
        if (is_inactivity_timeout_application_allowed())
            dim_display();
    }
    else if (id == user_inactivity_display_off_alarm_id)
    {
        log->log(log_tag, "handle_alarm(display_off)");
        user_inactivity_display_off_alarm_id = AlarmId::invalid;
        if (is_inactivity_timeout_application_allowed())
            turn_off_display(DisplayPowerChangeReason::activity);
        scheduled_timeout_type = ScheduledTimeoutType::none;
    }
    else if (id == user_inactivity_suspend_alarm_id)
    {
        log->log(log_tag, "handle_alarm(suspend)");
        user_inactivity_suspend_alarm_id = AlarmId::invalid;
        if (is_inactivity_timeout_application_allowed())
            suspend_when_allowed();
    }
    else if (id == proximity_disable_alarm_id)
    {
        log->log(log_tag, "handle_alarm(proximity_disable)");
        proximity_disable_alarm_id = AlarmId::invalid;
        disable_proximity(ProximityEnablement::until_far_event_or_timeout);
    }
    else if (id == notification_expiration_alarm_id)
    {
        log->log(log_tag, "handle_alarm(notification_expiration)");
        notification_expiration_alarm_id = AlarmId::invalid;
        if (display_power_mode == DisplayPowerMode::on)
            schedule_immediate_user_inactivity_alarm();

        allow_inactivity_timeout(InactivityTimeoutAllowance::notification);
        disable_proximity(ProximityEnablement::until_far_event_or_notification_expiration);
    }
}

void repowerd::DefaultStateMachine::handle_active_call()
{
    log->log(log_tag, "handle_active_call");

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
    }
    else if (proximity_sensor->proximity_state() == ProximityState::far)
    {
        turn_on_display_with_normal_timeout(DisplayPowerChangeReason::call);
    }

    enable_proximity(ProximityEnablement::until_disabled);
}

void repowerd::DefaultStateMachine::handle_no_active_call()
{
    log->log(log_tag, "handle_no_active_call");

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_reduced_user_inactivity_alarm();
    }
    else
    {
        if (proximity_sensor->proximity_state() == ProximityState::far)
        {
            turn_on_display_with_reduced_timeout(DisplayPowerChangeReason::call_done);
        }
        else
        {
            enable_proximity(ProximityEnablement::until_far_event_or_timeout);
            schedule_proximity_disable_alarm();
        }
    }

    disable_proximity(ProximityEnablement::until_disabled);
}

void repowerd::DefaultStateMachine::handle_enable_inactivity_timeout()
{
    log->log(log_tag, "handle_enable_inactivity_timeout");

    allow_inactivity_timeout(InactivityTimeoutAllowance::client);
}

void repowerd::DefaultStateMachine::handle_disable_inactivity_timeout()
{
    log->log(log_tag, "handle_disable_inactivity_timeout");

    disallow_inactivity_timeout(InactivityTimeoutAllowance::client);

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
    }
    else
    {
        turn_on_display_without_timeout(DisplayPowerChangeReason::unknown);
    }
}

void repowerd::DefaultStateMachine::handle_set_inactivity_behavior(
    PowerAction power_action,
    PowerSupply power_supply,
    std::chrono::milliseconds timeout)
{
    log->log(log_tag, "handle_set_inactivity_behavior(%s,%s,%ld)",
             power_action_to_str(power_action).c_str(),
             power_supply_to_str(power_supply).c_str(),
             static_cast<long>(timeout.count()));

    if (timeout <= std::chrono::milliseconds::zero())
        return;

    if (power_action != PowerAction::display_off &&
        power_action != PowerAction::suspend)
    {
        return;
    }

    bool power_supply_is_active{false};
    auto& inactivity_timeout =
        power_action == PowerAction::display_off ?
        user_inactivity_normal_display_off_timeout :
        user_inactivity_normal_suspend_timeout;

    if (power_supply == PowerSupply::battery)
    {
        inactivity_timeout.on_battery = timeout;
        power_supply_is_active = user_inactivity_normal_display_off_timeout.is_on_battery;
    }
    else
    {
        inactivity_timeout.on_line_power = timeout;
        power_supply_is_active = !user_inactivity_normal_display_off_timeout.is_on_battery;
    }

    if (scheduled_timeout_type == ScheduledTimeoutType::normal && power_supply_is_active)
        schedule_normal_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::handle_lid_closed()
{
    log->log(log_tag, "handle_lid_closed()");

    lid_closed = true;

    if (!display_information->has_active_external_displays())
    {
        if (display_power_mode == DisplayPowerMode::on)
            turn_off_display(DisplayPowerChangeReason::unknown);

        if (lid_power_action.get() == PowerAction::suspend)
            system_power_control->suspend();
    }
    else
    {
        display_power_control->turn_off(DisplayPowerControlFilter::internal);
    }
}

void repowerd::DefaultStateMachine::handle_lid_open()
{
    log->log(log_tag, "handle_lid_open()");

    lid_closed = false;

    if (display_power_mode == DisplayPowerMode::on)
    {
        display_power_control->turn_on(DisplayPowerControlFilter::internal);
        brighten_display();
        schedule_normal_user_inactivity_alarm();
        display_power_mode_reason = DisplayPowerChangeReason::activity;
    }
    else
    {
        turn_on_display_with_normal_timeout(DisplayPowerChangeReason::activity);
    }

}

void repowerd::DefaultStateMachine::handle_set_lid_behavior(
    PowerAction power_action, PowerSupply power_supply)
{
    log->log(log_tag, "handle_set_lid_behavior(%s,%s)",
             power_action_to_str(power_action).c_str(),
             power_supply_to_str(power_supply).c_str());

    if (power_action != PowerAction::none &&
        power_action != PowerAction::suspend)
    {
        return;
    }

    if (power_supply == PowerSupply::battery)
        lid_power_action.on_battery = power_action;
    else
        lid_power_action.on_line_power = power_action;
}

void repowerd::DefaultStateMachine::handle_set_critical_power_behavior(
    PowerAction power_action)
{
    log->log(log_tag, "handle_set_critical_power_behavior(%s)",
             power_action_to_str(power_action).c_str());

    if (power_action != PowerAction::suspend &&
        power_action != PowerAction::power_off)
    {
        return;
    }

    critical_power_action = power_action;
}

void repowerd::DefaultStateMachine::handle_no_notification()
{
    log->log(log_tag, "handle_no_notification");

    if (display_power_mode == DisplayPowerMode::on)
    {
        schedule_post_notification_user_inactivity_alarm();
    }

    allow_inactivity_timeout(InactivityTimeoutAllowance::notification);
    disable_proximity(ProximityEnablement::until_far_event_or_notification_expiration);
    cancel_notification_expiration_alarm();
}

void repowerd::DefaultStateMachine::handle_notification()
{
    log->log(log_tag, "handle_notification");

    disallow_inactivity_timeout(InactivityTimeoutAllowance::notification);

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
    }
    else
    {
        if (proximity_sensor->proximity_state() == ProximityState::far)
        {
            turn_on_display_without_timeout(DisplayPowerChangeReason::notification);
        }
        else
        {
            enable_proximity(ProximityEnablement::until_far_event_or_notification_expiration);
        }
    }

    schedule_notification_expiration_alarm();
}

void repowerd::DefaultStateMachine::handle_power_button_press()
{
    log->log(log_tag, "handle_power_button_press");

    display_power_mode_at_power_button_press = display_power_mode;

    if (treat_power_button_as_user_activity &&
        display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
        display_power_mode_reason = DisplayPowerChangeReason::power_button;
    }
    else if (display_power_mode == DisplayPowerMode::off)
    {
        turn_on_display_with_normal_timeout(DisplayPowerChangeReason::power_button);
    }

    power_button_long_press_alarm_id =
        timer->schedule_alarm_in(power_button_long_press_timeout);
}

void repowerd::DefaultStateMachine::handle_power_button_release()
{
    log->log(log_tag, "handle_power_button_release");

    if (power_button_long_press_detected)
    {
        power_button_long_press_detected = false;
    }
    else if (display_power_mode_at_power_button_press == DisplayPowerMode::on &&
             !treat_power_button_as_user_activity)
    {
        turn_off_display(DisplayPowerChangeReason::power_button);
    }

    display_power_mode_at_power_button_press = DisplayPowerMode::unknown;
    power_button_long_press_alarm_id = AlarmId::invalid;
}

void repowerd::DefaultStateMachine::handle_power_source_change()
{
    log->log(log_tag, "handle_power_source_change");

    auto const is_on_battery = power_source->is_using_battery_power();

    user_inactivity_normal_display_off_timeout.is_on_battery = is_on_battery;
    user_inactivity_normal_suspend_timeout.is_on_battery = is_on_battery;
    lid_power_action.is_on_battery = is_on_battery;

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
        display_power_mode_reason = DisplayPowerChangeReason::activity;
    }
    else if (proximity_sensor->proximity_state() == ProximityState::far)
    {
        turn_on_display_with_reduced_timeout(DisplayPowerChangeReason::notification);
    }

    schedule_normal_user_inactivity_suspend_alarm();
}

void repowerd::DefaultStateMachine::handle_power_source_critical()
{
    log->log(log_tag, "handle_power_source_critical");

    if (critical_power_action == PowerAction::power_off)
        system_power_control->power_off();
    else if (critical_power_action == PowerAction::suspend)
        system_power_control->suspend();
}

void repowerd::DefaultStateMachine::handle_proximity_far()
{
    log->log(log_tag, "handle_proximity_far");

    auto const use_reduced_timeout =
        is_proximity_enabled_only_until_far_event_or_notification_expiration();
    disable_proximity(ProximityEnablement::until_far_event_or_notification_expiration);
    disable_proximity(ProximityEnablement::until_far_event_or_timeout);

    if (display_power_mode == DisplayPowerMode::off)
    {
        if (use_reduced_timeout)
        {
            turn_on_display_with_reduced_timeout(DisplayPowerChangeReason::proximity);
        }
        else
        {
            turn_on_display_with_normal_timeout(DisplayPowerChangeReason::proximity);
        }
    }
}

void repowerd::DefaultStateMachine::handle_proximity_near()
{
    log->log(log_tag, "handle_proximity_near");

    if (display_power_mode == DisplayPowerMode::on)
        turn_off_display(DisplayPowerChangeReason::proximity);
}

void repowerd::DefaultStateMachine::handle_user_activity_changing_power_state()
{
    log->log(log_tag, "handle_user_activity_changing_power_state");

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
        display_power_mode_reason = DisplayPowerChangeReason::activity;
    }
    else if (proximity_sensor->proximity_state() == ProximityState::far)
    {
        turn_on_display_with_normal_timeout(DisplayPowerChangeReason::activity);
    }
}

void repowerd::DefaultStateMachine::handle_user_activity_extending_power_state()
{
    log->log(log_tag, "handle_user_activity_extending_power_state");

    if (display_power_mode == DisplayPowerMode::on)
    {
        brighten_display();
        schedule_normal_user_inactivity_alarm();
        display_power_mode_reason = DisplayPowerChangeReason::activity;
    }
}

void repowerd::DefaultStateMachine::handle_set_normal_brightness_value(double value)
{
    log->log(log_tag, "handle_set_normal_brightness_value(%.2f)", value);

    normal_brightness_value = value;

    if (paused) return;

    brightness_control->set_normal_brightness_value(value);
}

void repowerd::DefaultStateMachine::handle_enable_autobrightness()
{
    log->log(log_tag, "enable_autobrightness");

    autobrightness_enabled = true;

    if (paused) return;

    brightness_control->enable_autobrightness();
}

void repowerd::DefaultStateMachine::handle_disable_autobrightness()
{
    log->log(log_tag, "disable_autobrightness");

    autobrightness_enabled = false;

    if (paused) return;

    brightness_control->disable_autobrightness();
}

void repowerd::DefaultStateMachine::handle_allow_suspend()
{
    log->log(log_tag, "allow_suspend");

    suspend_allowed = true;

    if (display_power_mode == DisplayPowerMode::off &&
        display_power_mode_reason == DisplayPowerChangeReason::activity)
    {
        system_power_control->allow_automatic_suspend(suspend_id);
    }

    if (suspend_pending)
        suspend_when_allowed();
}

void repowerd::DefaultStateMachine::handle_disallow_suspend()
{
    log->log(log_tag, "disallow_suspend");

    suspend_allowed = false;
}

void repowerd::DefaultStateMachine::handle_system_resume()
{
    log->log(log_tag, "handle_system_resume");

    turn_on_display_with_normal_timeout(DisplayPowerChangeReason::activity);
}

void repowerd::DefaultStateMachine::start()
{
    log->log(log_tag, "start");

    auto const is_on_battery = power_source->is_using_battery_power();

    user_inactivity_normal_display_off_timeout.is_on_battery = is_on_battery;
    user_inactivity_normal_suspend_timeout.is_on_battery = is_on_battery;
    lid_power_action.is_on_battery = is_on_battery;

    system_power_control->disallow_default_system_handlers();

    if (turn_on_display_at_startup)
        turn_on_display_with_normal_timeout(DisplayPowerChangeReason::unknown);
}

void repowerd::DefaultStateMachine::pause()
{
    log->log(log_tag, "pause");

    if (power_button_long_press_alarm_id != AlarmId::invalid)
    {
        timer->cancel_alarm(power_button_long_press_alarm_id);
        power_button_long_press_alarm_id = AlarmId::invalid;
    }

    proximity_sensor->disable_proximity_events();
    brightness_control->disable_autobrightness();
    system_power_control->allow_default_system_handlers();

    paused = true;
}

void repowerd::DefaultStateMachine::resume()
{
    log->log(log_tag, "resume");

    paused = false;

    system_power_control->disallow_default_system_handlers();

    if (autobrightness_enabled)
        brightness_control->enable_autobrightness();
    else
        brightness_control->disable_autobrightness();

    brightness_control->set_normal_brightness_value(normal_brightness_value);

    turn_on_display_with_normal_timeout(DisplayPowerChangeReason::unknown);

    if (is_proximity_enabled())
        proximity_sensor->enable_proximity_events();
}

void repowerd::DefaultStateMachine::cancel_user_inactivity_display_off_alarm()
{
    if (user_inactivity_display_dim_alarm_id != AlarmId::invalid)
    {
        timer->cancel_alarm(user_inactivity_display_dim_alarm_id);
        user_inactivity_display_dim_alarm_id = AlarmId::invalid;
    }

    if (user_inactivity_display_off_alarm_id != AlarmId::invalid)
    {
        timer->cancel_alarm(user_inactivity_display_off_alarm_id);
        user_inactivity_display_off_alarm_id = AlarmId::invalid;
    }

    user_inactivity_display_off_time_point = {};
    scheduled_timeout_type = ScheduledTimeoutType::none;
}

void repowerd::DefaultStateMachine::cancel_user_inactivity_suspend_alarm()
{
    if (user_inactivity_suspend_alarm_id != AlarmId::invalid)
    {
        timer->cancel_alarm(user_inactivity_suspend_alarm_id);
        user_inactivity_suspend_alarm_id = AlarmId::invalid;
    }
}

void repowerd::DefaultStateMachine::cancel_notification_expiration_alarm()
{
    if (notification_expiration_alarm_id != AlarmId::invalid)
    {
        timer->cancel_alarm(notification_expiration_alarm_id);
        notification_expiration_alarm_id = AlarmId::invalid;
    }
}

void repowerd::DefaultStateMachine::schedule_normal_user_inactivity_alarm()
{
    schedule_normal_user_inactivity_display_off_alarm();
    schedule_normal_user_inactivity_suspend_alarm();
}

void repowerd::DefaultStateMachine::schedule_normal_user_inactivity_display_off_alarm()
{
    cancel_user_inactivity_display_off_alarm();
    scheduled_timeout_type = ScheduledTimeoutType::normal;

    if (user_inactivity_normal_display_off_timeout.get() == repowerd::infinite_timeout)
    {
        user_inactivity_display_off_time_point = std::chrono::steady_clock::time_point::max();
    }
    else
    {
        user_inactivity_display_off_time_point =
            timer->now() + user_inactivity_normal_display_off_timeout.get();
        if (user_inactivity_normal_display_off_timeout.get() > user_inactivity_normal_display_dim_duration)
        {
            user_inactivity_display_dim_alarm_id =
                timer->schedule_alarm_in(
                    user_inactivity_normal_display_off_timeout.get() -
                    user_inactivity_normal_display_dim_duration);
        }

        user_inactivity_display_off_alarm_id =
            timer->schedule_alarm_in(user_inactivity_normal_display_off_timeout.get());
    }
}

void repowerd::DefaultStateMachine::schedule_normal_user_inactivity_suspend_alarm()
{
    cancel_user_inactivity_suspend_alarm();
    cancel_suspend_when_allowed();

    if (user_inactivity_normal_suspend_timeout.get() != repowerd::infinite_timeout)
    {
        user_inactivity_suspend_alarm_id =
            timer->schedule_alarm_in(user_inactivity_normal_suspend_timeout.get());
    }
}

void repowerd::DefaultStateMachine::schedule_post_notification_user_inactivity_alarm()
{
    auto const tp = timer->now() + user_inactivity_post_notification_display_off_timeout;
    if (tp > user_inactivity_display_off_time_point)
    {
        cancel_user_inactivity_display_off_alarm();
        user_inactivity_display_off_alarm_id =
            timer->schedule_alarm_in(user_inactivity_post_notification_display_off_timeout);
        user_inactivity_display_off_time_point = tp;
        scheduled_timeout_type = ScheduledTimeoutType::post_notification;
    }
}

void repowerd::DefaultStateMachine::schedule_reduced_user_inactivity_alarm()
{
    auto const tp = timer->now() + user_inactivity_reduced_display_off_timeout;
    if (tp > user_inactivity_display_off_time_point)
    {
        cancel_user_inactivity_display_off_alarm();
        user_inactivity_display_off_alarm_id =
            timer->schedule_alarm_in(user_inactivity_reduced_display_off_timeout);
        user_inactivity_display_off_time_point = tp;
        scheduled_timeout_type = ScheduledTimeoutType::reduced;
    }
}

void repowerd::DefaultStateMachine::schedule_proximity_disable_alarm()
{
    if (proximity_disable_alarm_id != AlarmId::invalid)
        timer->cancel_alarm(proximity_disable_alarm_id);

    proximity_disable_alarm_id =
        timer->schedule_alarm_in(user_inactivity_reduced_display_off_timeout);
}

void repowerd::DefaultStateMachine::schedule_notification_expiration_alarm()
{
    cancel_notification_expiration_alarm();

    auto const timeout =
        std::min(
            user_inactivity_normal_display_off_timeout.get(),
            notification_expiration_timeout);

    notification_expiration_alarm_id =
        timer->schedule_alarm_in(timeout);
}

void repowerd::DefaultStateMachine::schedule_immediate_user_inactivity_alarm()
{
    auto const tp = timer->now();
    if (tp > user_inactivity_display_off_time_point)
    {
        cancel_user_inactivity_display_off_alarm();
        user_inactivity_display_off_alarm_id =
            timer->schedule_alarm_in(std::chrono::milliseconds{0});
        user_inactivity_display_off_time_point = tp;
        scheduled_timeout_type = ScheduledTimeoutType::post_notification;
    }
}

void repowerd::DefaultStateMachine::turn_off_display(
    DisplayPowerChangeReason reason)
{
    if (paused) return;

    brightness_control->set_off_brightness();
    display_power_control->turn_off(DisplayPowerControlFilter::all);
    if (reason != DisplayPowerChangeReason::proximity)
        modem_power_control->set_low_power_mode();
    display_power_mode = DisplayPowerMode::off;
    display_power_mode_reason = reason;
    cancel_user_inactivity_display_off_alarm();
    display_power_event_sink->notify_display_power_off(reason);
    performance_booster->disable_interactive_mode();
    if (reason != DisplayPowerChangeReason::proximity)
    {
        if ((reason == DisplayPowerChangeReason::activity && suspend_allowed) ||
            reason != DisplayPowerChangeReason::activity)
        {
            system_power_control->allow_automatic_suspend(suspend_id);
        }
    }
}

void repowerd::DefaultStateMachine::turn_on_display_without_timeout(
    DisplayPowerChangeReason reason)
{
    if (paused) return;

    system_power_control->disallow_automatic_suspend(suspend_id);
    performance_booster->enable_interactive_mode();
    if (lid_closed)
        display_power_control->turn_on(DisplayPowerControlFilter::external);
    else
        display_power_control->turn_on(DisplayPowerControlFilter::all);
    display_power_mode = DisplayPowerMode::on;
    display_power_mode_reason = reason;
    if (!lid_closed)
        brighten_display();
    modem_power_control->set_normal_power_mode();
    display_power_event_sink->notify_display_power_on(reason);
}

void repowerd::DefaultStateMachine::turn_on_display_with_normal_timeout(
    DisplayPowerChangeReason reason)
{
    turn_on_display_without_timeout(reason);
    schedule_normal_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::turn_on_display_with_reduced_timeout(
    DisplayPowerChangeReason reason)
{
    turn_on_display_without_timeout(reason);
    schedule_reduced_user_inactivity_alarm();
}

void repowerd::DefaultStateMachine::brighten_display()
{
    if (paused) return;

    brightness_control->set_normal_brightness();
}

void repowerd::DefaultStateMachine::dim_display()
{
    if (paused) return;

    brightness_control->set_dim_brightness();
}

void repowerd::DefaultStateMachine::allow_inactivity_timeout(
    InactivityTimeoutAllowance allowance)
{
    if (!is_inactivity_timeout_allowed())
    {
        inactivity_timeout_allowances[allowance] = true;

        if (is_inactivity_timeout_allowed() &&
            display_power_mode == DisplayPowerMode::on)
        {
            if (allowance == InactivityTimeoutAllowance::notification &&
                scheduled_timeout_type == ScheduledTimeoutType::none)
            {
                turn_off_display(DisplayPowerChangeReason::activity);
            }
            else if (allowance == InactivityTimeoutAllowance::client)
            {
                schedule_normal_user_inactivity_alarm();
            }
        }
    }
}

void repowerd::DefaultStateMachine::disallow_inactivity_timeout(
    InactivityTimeoutAllowance allowance)
{
    inactivity_timeout_allowances[allowance] = false;
}

bool repowerd::DefaultStateMachine::is_inactivity_timeout_allowed()
{
    auto const client = inactivity_timeout_allowances[InactivityTimeoutAllowance::client];
    auto const notification = inactivity_timeout_allowances[InactivityTimeoutAllowance::notification];

    return notification && client;
}

bool repowerd::DefaultStateMachine::is_inactivity_timeout_application_allowed()
{
    if (is_inactivity_timeout_allowed())
        return true;

    if (display_power_mode_reason == DisplayPowerChangeReason::notification ||
        display_power_mode_reason == DisplayPowerChangeReason::call)
    {
        return true;
    }

    return false;
}

void repowerd::DefaultStateMachine::enable_proximity(
    ProximityEnablement enablement)
{
    auto const proximity_previously_enabled = is_proximity_enabled();

    proximity_enablements[enablement] = true;

    if (!proximity_previously_enabled && is_proximity_enabled())
    {
        proximity_sensor->enable_proximity_events();
    }
}

void repowerd::DefaultStateMachine::disable_proximity(
    ProximityEnablement enablement)
{
    auto const proximity_previously_enabled = is_proximity_enabled();

    proximity_enablements[enablement] = false;

    if (proximity_previously_enabled && !is_proximity_enabled())
    {
        proximity_sensor->disable_proximity_events();
    }
}

bool repowerd::DefaultStateMachine::is_proximity_enabled()
{
    for (auto const& enabled : proximity_enablements)
        if (enabled) return true;
    return false;
}

bool repowerd::DefaultStateMachine::is_proximity_enabled_only_until_far_event_or_notification_expiration()
{
    auto num_enabled = 0;
    for (auto const& enabled : proximity_enablements)
        if (enabled) ++num_enabled;

    return num_enabled == 1 && proximity_enablements[ProximityEnablement::until_far_event_or_notification_expiration];
}

void repowerd::DefaultStateMachine::suspend_when_allowed()
{
    if (suspend_allowed)
    {
        suspend_pending = false;
        if (!paused)
            system_power_control->suspend();
    }
    else
    {
        suspend_pending = true;
    }
}

void repowerd::DefaultStateMachine::cancel_suspend_when_allowed()
{
    suspend_pending = false;
}
