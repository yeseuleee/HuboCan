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

#ifndef HUBOCMD_HUBO_CMD_C_H
#define HUBOCMD_HUBO_CMD_C_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "HuboCan/AchIncludes.h"

#define HUBO_CMD_CHANNEL "hubo_cmd"

//                            123456789012345
#define HUBO_CMD_HEADER_CODE "CMDHEADER_V0.01"
#define HUBO_CMD_HEADER_CODE_SIZE 16 // including null-terminator \0

typedef uint8_t hubo_cmd_data;

typedef enum hubo_data_error {

    HUBO_DATA_OKAY = 0,
    HUBO_DATA_NULL,
    HUBO_DATA_OUT_OF_BOUNDS,
    HUBO_DATA_READ_ONLY,
    HUBO_DATA_UNAVAILABLE_INDEX,
    HUBO_DATA_MALFORMED_HEADER,
    
    HUBO_DATA_IMPOSSIBLE

} hubo_data_error_t;

typedef struct hubo_cmd_header {

    char code[HUBO_CMD_HEADER_CODE_SIZE];
    uint16_t pid;
    uint8_t is_compressed;
    uint16_t total_num_joints;
    uint64_t bitmap;

}__attribute__((packed)) hubo_cmd_header_t;

typedef enum hubo_cmd_mode {

    HUBO_CMD_IGNORE = 0,
    HUBO_CMD_RIGID,
    HUBO_CMD_COMPLIANT,
    HUBO_CMD_HYBRID,

    HUBO_CMD_CLAIM,
    HUBO_CMD_RELEASE

} hubo_cmd_mode_t;

typedef struct hubo_joint_cmd {

    hubo_cmd_mode_t mode;
    float position; // No sense in having refs of greater precision than the CAN frame can use
    double base_torque; // Everything else will be involved in further calculations, so more precision might be good
    double kP_gain;
    double kD_gain;

}__attribute__((packed)) hubo_joint_cmd_t;

size_t hubo_cmd_data_predict_max_message_size(size_t num_joints);

size_t hubo_cmd_data_get_min_data_size(const hubo_cmd_data* data);

size_t hubo_cmd_data_location(size_t joint_index);

int hubo_cmd_data_is_compressed(const hubo_cmd_data* data);

size_t hubo_cmd_data_get_total_num_joints(const hubo_cmd_data* data);

hubo_data_error_t hubo_cmd_header_check(const hubo_cmd_data* cmd_message);

hubo_cmd_data* hubo_cmd_init_data(size_t num_total_joints);

size_t hubo_cmd_data_compressor(hubo_cmd_data* compressed, const hubo_cmd_data* uncompressed);

hubo_data_error_t hubo_cmd_data_set_joint_cmd(hubo_cmd_data* data, const hubo_joint_cmd_t* cmd, size_t joint_index);

hubo_data_error_t hubo_cmd_data_get_joint_cmd(hubo_joint_cmd_t* output, const hubo_cmd_data* data, size_t joint_index);

hubo_joint_cmd_t* hubo_cmd_data_access_joint_cmd(hubo_cmd_data* data, size_t joint_index);

hubo_data_error_t hubo_cmd_data_register_joint(hubo_cmd_data* data, size_t joint_index);

hubo_data_error_t hubo_cmd_data_unregister_released_joints(hubo_cmd_data* data);

size_t hubo_cmd_data_get_size(const hubo_cmd_data* data);

int hubo_cmd_data_check_if_joint_is_set(const hubo_cmd_data* data, size_t joint_index);

#endif // HUBOCMD_HUBO_CMD_C_H
