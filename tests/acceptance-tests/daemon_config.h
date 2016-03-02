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

namespace repowerd
{
namespace test
{

class FakePowerButton;
class MockDisplayPowerControl;

class DaemonConfig : public repowerd::DefaultDaemonConfig
{
public:
    std::shared_ptr<DisplayPowerControl> the_display_power_control() override;
    std::shared_ptr<PowerButton> the_power_button() override;

    std::shared_ptr<MockDisplayPowerControl> the_mock_display_power_control();
    std::shared_ptr<FakePowerButton> the_fake_power_button();

private:
    std::shared_ptr<MockDisplayPowerControl> mock_display_power_control;
    std::shared_ptr<FakePowerButton> fake_power_button;
};

}
}
