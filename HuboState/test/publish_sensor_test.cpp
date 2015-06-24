/*
 * Copyright (c) 2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Michael X. Grey <greyxmike@gmail.com>
 *
 * Humanoid Robotics Lab
 *
 * Directed by Prof. Mike Stilman <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include "unistd.h"

#include "HuboState/State.hpp"

int main(int , char* [])
{
    HuboCan::HuboDescription desc;
    desc.parseFile("/home/grey/projects/HuboCan/devices/Hubo2Plus.dd");
    desc.broadcastInfo();

    HuboState::State state(desc);

    if(state.initialized())
    {
        std::cout << "Joint count: " << state.joints.size() << std::endl
                  << "IMU count:   " << state.imus.size() << std::endl
                  << "FT count:    " << state.force_torques.size() << std::endl;
    }

    if(!desc.okay())
        return 1;

    while(state.initialized())
    {
        for(size_t i=0; i<state.imus.size(); ++i)
        {
            hubo_imu_state_t& imu = state.imus[i];
            imu.angular_position[0] = 0.1;
            imu.angular_position[1] = 1.2;
            imu.angular_position[2] = 2.3;

            imu.angular_velocity[0] = 1.0;
            imu.angular_velocity[1] = 2.1;
            imu.angular_position[2] = 3.2;
        }

        for(size_t i=0; i<state.force_torques.size(); ++i)
        {
            hubo_ft_state_t& ft = state.force_torques[i];
            ft.force[0] = 0.12;
            ft.force[1] = 1.23;
            ft.force[2] = 2.34;

            ft.torque[0] = 4.32;
            ft.torque[1] = 3.21;
            ft.torque[2] = 2.10;
        }

        state.publish();
        usleep(1e6/200);
    }
}
