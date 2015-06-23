
#include <cmath>

#include "../HuboSensor.hpp"
#include "HuboCan/HuboCanId.hpp"
#include "HuboCmd/Aggregator.hpp"
#include "HuboState/State.hpp"
#include "utils.hpp"

namespace HuboCan {

HuboSensor::HuboSensor()
{
    memset(&_frame, 0, sizeof(_frame));
}

void HuboSensor::assign_pointers(HuboCmd::Aggregator* agg, HuboState::State* state)
{
    _agg = agg;
    _state = state;
}

void HuboSensor::auxiliary_command(const hubo_aux_cmd_t& command)
{
    _aux_commands.push_back(command);
}

HuboImu::HuboImu(size_t index)
    : _index(index)
{
    // Do nothing
}

size_t HuboImu::getImuIndex() const
{
    return _index;
}

Hubo2PlusImu::Hubo2PlusImu(size_t index)
  : HuboImu(index)
{
    // Do nothing
}

void Hubo2PlusImu::update()
{
    if(_aux_commands.size() > 0)
    {
        _process_auxiliary_commands();
    }
    else
    {
        _request_imu_readings();
    }
}

bool Hubo2PlusImu::decode(const can_frame_t& frame, size_t channel)
{
    if( channel != info.can_channel )
        return false;

    if( static_cast<int>(frame.can_id) == info.hardware_index - IMU_REPLY )
    {
        hubo_imu_state_t& data = _state->imus[_index];

        int16_t val = (frame.data[1] << 8) | frame.data[0];
        data.angular_position[0] = (double)(val)/100.0 * M_PI/180.0;

        val = (frame.data[3] << 8) | frame.data[2];
        data.angular_position[1] = (double)(val)/100.0 * M_PI/180.0;

        data.angular_position[2] = 0.0;

        val = (frame.data[5] << 8) | frame.data[4];
        data.angular_velocity[0] = (double)(val)/100.0 * M_PI/180.0;

        val = (frame.data[7] << 8) | frame.data[6];
        data.angular_velocity[1] = (double)(val)/100.0 * M_PI/180.0;

        data.angular_velocity[2] = 0.0;

        return true;
    }

    return false;
}

void Hubo2PlusImu::_request_imu_readings()
{
    _frame.can_id   = SENSOR_REQUEST;

    _frame.data[0]  = info.hardware_index;
    _frame.data[1]  = 0x00;
    _frame.data[2]  = 0x01;

    _frame.can_dlc  = 3;

    _pump->add_frame(_frame, info.can_channel, 1);
}

void Hubo2PlusImu::_process_auxiliary_commands()
{
    while(_aux_commands.size() > 0)
    {
        _handle_auxiliary_command(_aux_commands.back());
        _aux_commands.pop_back();
    }
}

void Hubo2PlusImu::_handle_auxiliary_command(const hubo_aux_cmd_t& cmd)
{
    switch(cmd.cmd_id)
    {
        case INIT_ALL_SENSORS:
        case INIT_ALL_IMUS:
        case INIT_SENSOR:
            _initialize_imu();
            break;

        default:
            std::cerr << "[Hubo2PlusImu] Unknown/Unsupported auxiliary command type: "
                      << cmd.cmd_id << std::endl;
            break;
    }
}

void Hubo2PlusImu::_initialize_imu()
{
    // Log the initialization so we know that the command arrived
    std::cout << "Sending initialization command for IMU " << info.name << std::endl;

    memset(&_frame, 0, sizeof(_frame));

    _frame.can_id   = CMD_BYTE;

    _frame.data[0]  = (uint8_t)info.hardware_index + SENSOR_INSTRUCTION;
    _frame.data[1]  = NULL_SENSOR;

    _frame.can_dlc = 2;

    _pump->add_frame(_frame, info.can_channel);
}

Hubo2PlusTilt::Hubo2PlusTilt(size_t index)
    : Hubo2PlusImu(index)
{
    // Do nothing
}

bool Hubo2PlusTilt::decode(const can_frame_t& frame, size_t channel)
{
    if( channel != info.can_channel )
        return false;

    if( static_cast<int>(frame.can_id) == info.hardware_index - IMU_REPLY )
    {
        hubo_imu_state_t& state = _state->imus[_index];

        int16_t val = (frame.data[1] << 8) | frame.data[0];
        state.angular_position[0] = (double)(val)/100.0 * M_PI/180.0;

        val = (frame.data[3] << 8) | frame.data[2];
        state.angular_position[1] = (double)(val)/100.0 * M_PI/180.0;

        val = (frame.data[5] << 8) | frame.data[4];
        state.angular_position[2] = (double)(val)/750.0 * M_PI/180.0;

        for(size_t i=0; i<3; ++i)
            state.angular_velocity[i] = 0.0;

        return true;
    }

    return false;
}

void Hubo2PlusTilt::_request_imu_readings()
{
    _frame.can_id   = SENSOR_REQUEST;

    _frame.data[0]  = info.hardware_index;
    _frame.data[1]  = GET_ACC_SCALED;

    _frame.can_dlc  = 2;

    _pump->add_frame(_frame, info.can_channel, 1);
}

void Hubo2PlusTilt::_initialize_imu()
{
    // Log the initialization so we know that the command arrived
    std::cout << "Sending initialization command for the tilt sensor " << info.name << std::endl;

    memset(&_frame, 0, sizeof(_frame));

    _frame.can_id   = CMD_BYTE;

    _frame.data[0]  = (uint8_t)info.hardware_index + SENSOR_INSTRUCTION;
    _frame.data[1]  = NULL_SENSOR;
    _frame.data[2]  = NULL_TILT;

    _frame.can_dlc  = 3;

    _pump->add_frame(_frame, info.can_channel);
}

DrcHuboImu::DrcHuboImu(size_t index)
    : Hubo2PlusImu(index)
{
    // Do nothing
}



HuboFt::HuboFt(size_t index)
    : _index(index)
{
    // Do nothing
}

size_t HuboFt::getFtIndex() const
{
    return _index;
}

Hubo2PlusFt::Hubo2PlusFt(size_t index)
  : HuboFt(index)
{
    // Do nothing
}

void Hubo2PlusFt::update()
{
    if(_aux_commands.size() > 0)
    {
        _process_auxiliary_commands();
    }
    else
    {
        _request_ft_readings();
    }
}

void Hubo2PlusFt::_request_ft_readings()
{
    _frame.can_id   = SENSOR_REQUEST;

    _frame.data[0]  = info.hardware_index;
    _frame.data[1]  = GET_FT_SCALED;

    _frame.can_dlc  = 2;

    _pump->add_frame(_frame, info.can_channel, 1);
}

void Hubo2PlusFt::_process_auxiliary_commands()
{
    while(_aux_commands.size() > 0)
    {
        _handle_auxiliary_command(_aux_commands.back());
        _aux_commands.pop_back();
    }
}

void Hubo2PlusFt::_handle_auxiliary_command(const hubo_aux_cmd_t& cmd)
{
    switch(cmd.cmd_id)
    {
        case INIT_ALL_SENSORS:
        case INIT_ALL_FTS:
        case INIT_SENSOR:
            _initialize_ft();
            break;

        default:
            std::cerr << "[Hubo2PlusFt] Unknown/Unsupported auxiliary command type: "
                      << cmd.cmd_id << std::endl;
            break;
    }
}

void Hubo2PlusFt::_initialize_ft()
{
    std::cout << "Sending initialization command for the ft sensor " << info.name << std::endl;

    memset(&_frame, 0, sizeof(_frame));

    _frame.can_id   = CMD_BYTE;

    _frame.data[0]  = (uint8_t)info.hardware_index + SENSOR_INSTRUCTION;
    _frame.data[1]  = NULL_SENSOR;
    _frame.data[2]  = NULL_FT;

    _frame.can_dlc  = 3;

    _pump->add_frame(_frame, info.can_channel);
}

bool Hubo2PlusFt::decode(const can_frame_t& frame, size_t channel)
{
    if( channel != info.can_channel )
        return false;

    if( static_cast<int>(frame.can_id) == info.hardware_index - FT_REPLY )
    {
        hubo_ft_state_t& state = _state->force_torques[_index];

        state.torque[0] = doubleFromBytePair(frame.data[1], frame.data[0])/100.0;
        state.torque[1] = doubleFromBytePair(frame.data[3], frame.data[2])/100.0;
        state.torque[2] = 0.0;

        state.force[0] = 0.0;
        state.force[0] = 0.0;
        state.force[2] = doubleFromBytePair(frame.data[5], frame.data[4])/10.0;
    }

    return false;
}

DrcHuboFt::DrcHuboFt(size_t index)
    : Hubo2PlusFt(index)
{
    // Do nothing
}

} // namespace HuboCan