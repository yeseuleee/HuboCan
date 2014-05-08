
#include "../Player.hpp"

const double eps = 1e-6;

using namespace HuboPath;

Player::Player(double timeout) :
    HuboCmd::Commander(timeout)
{
    _initialize_player();
}

Player::Player(const HuboCan::HuboDescription &description) :
    HuboCmd::Commander(description)
{
    _initialize_player();
}

void Player::_initialize_player()
{
    _channels_opened = false;
    _new_instruction = false;
    _new_trajectory = false;
    _first_step = true;
    memset(&_current_cmd, 0, sizeof(_current_cmd));
    memset(&_incoming_cmd, 0, sizeof(_incoming_cmd));
    memset(&_current_elem, 0, sizeof(_current_elem));
    memset(&_last_elem, 0, sizeof(_last_elem));
    _current_index = 0;
    open_channels();
    
    if(_desc.okay())
        _trajectory.desc = _desc;
}

bool Player::receive_description(double timeout_sec)
{
    bool success = HuboState::State::receive_description(timeout_sec);

    if(success)
        _trajectory.desc = _desc;

    return success;
}

void Player::load_description(const HuboCan::HuboDescription &description)
{
    HuboState::State::load_description(description);

    _trajectory.desc = _desc;
}

bool Player::open_channels()
{
    if(_channels_opened)
        return true;

    _channels_opened = true;

    ach_status_t result = ach_open(&_instruction_chan, HUBO_PATH_INSTRUCTION_CHANNEL, NULL);
    if( ACH_OK != result )
    {
        std::cout << "Error opening path instruction channel: "
                  << ach_result_to_string(result) << std::endl;
        _channels_opened = false;
    }
    ach_flush(&_instruction_chan);

    result = ach_open(&_input_chan, HUBO_PATH_INPUT_CHANNEL, NULL);
    if( ACH_OK != result )
    {
        std::cout << "Error opening path input channel: "
                  << ach_result_to_string(result) << std::endl;
        _channels_opened = false;
    }
    ach_flush(&_input_chan);

    result = ach_open(&_feedback_chan, HUBO_PATH_FEEDBACK_CHANNEL, NULL);
    if( ACH_OK != result )
    {
        std::cout << "Error opening path feedback channel: "
                  << ach_result_to_string(result) << std::endl;
        _channels_opened = false;
    }
    ach_flush(&_feedback_chan);

    result = ach_open(&_state_chan, HUBO_PATH_PLAYER_STATE_CHANNEL, NULL);
    if( ACH_OK != result )
    {
        std::cout << "Error opening player state channel: "
                  << ach_result_to_string(result) << std::endl;
        _channels_opened = false;
    }
    ach_flush(&_state_chan);

    _channels_opened = _channels_opened && HuboCmd::Commander::open_channels();

    return _channels_opened;
}

void Player::report_state()
{
    hubo_player_state_t state;
    state.current_index = _current_index;
    state.current_instruction = _current_cmd.instruction;
    state.trajectory_size = _trajectory.size();
    ach_put(&_state_chan, &state, sizeof(state));
}

void Player::_check_for_instructions()
{
    size_t fs;
    ach_status_t result = ach_get(&_instruction_chan, &_incoming_cmd,
                                  sizeof(_incoming_cmd), &fs, NULL, ACH_O_LAST);
    if( ACH_OK != result && ACH_STALE_FRAMES != result && ACH_MISSED_FRAME != result )
    {
        std::cout << "Unexpected ach result on the path instructions channel: "
                  << ach_result_to_string(result) << std::endl;
    }

    if( ACH_OK == result || ACH_MISSED_FRAME == result )
    {
        _new_instruction = true;
    }
    else
    {
        _new_instruction = false;
    }
}

bool Player::_receive_incoming_trajectory()
{
    ach_flush(&_input_chan);
    HuboCan::error_result_t result = receive_trajectory(_input_chan, _feedback_chan,
                                                        _trajectory, 10);
    if(result != HuboCan::OKAY)
    {
        return false;
    }

    // TODO: Should the trajectory be passed through the controller before evaluating the refs?
    // Almost certainly.
//    bool all_valid = true;
    std::vector<size_t> invalid_joints;
    for(size_t i=0; i<_desc.joints.size(); ++i)
    {
        if( ((_trajectory.params.bitmap >> i) & 0x01) == 0x01 )
        {
            if(_trajectory.elements[0].references[i] != joints[i].reference)
            {
                invalid_joints.push_back(i);
            }
        }
    }

    if(invalid_joints.size() == 0)
    {
        for(size_t i=0; i<_desc.joints.size(); ++i)
        {
            if( ((_trajectory.params.bitmap >> i) & 0x01) == 0x01 )
            {
                claim_joint(i);
            }
        }
    }
    else
    {
        std::cout << "Error! The following joints had invalid starting values: ";
        for(size_t i=0; i<invalid_joints.size(); ++i)
        {
            size_t invalid = invalid_joints[i];
            std::cout << _desc.getJointName(invalid) << " ("
                      << _trajectory.elements[0].references[invalid]
                      << ":" << joints[invalid].reference << ")";
            if(i+1 < invalid_joints.size())
                std::cout << ", ";
        }
        std::cout << std::endl;
        return false;
    }
    
    if(!_trajectory.interpolate())
    {
        std::cout << "Failed to interpolate the trajectory with the following params: "
                  << _trajectory.params << std::endl;
        return false;
    }
    
    if(!_trajectory.check_limits())
        return false;

    send_commands();
    _current_index = 0;

//    std::cout << _trajectory << std::endl;
    _new_trajectory = true;
    return true;
}

bool Player::step()
{
    HuboCan::error_result_t update_result = update();

    if(update_result != HuboCan::OKAY)
    {
        std::cout << "Player failed to update its states: " << update_result << std::endl;
//        report_state();
        return false;
    }

    if(_first_step)
    {
        _last_time = get_time();
        _first_step = false;
    }

    double dt = get_time() - _last_time;
    if( dt <= 0 )
    {
//        report_state();
        return true;
    }

    _check_for_instructions();
    if(_new_instruction)
    {
        std::cout << "Received new instruction: " << _incoming_cmd.instruction << std::endl;
    }

    if( HUBO_PATH_LOAD == _incoming_cmd.instruction
            || HUBO_PATH_LOAD_N_GO == _incoming_cmd.instruction )
    {
        if( _new_instruction )
        {
            if(_receive_incoming_trajectory())
            {
                if(HUBO_PATH_LOAD_N_GO == _incoming_cmd.instruction)
                {
                    _current_cmd.instruction = HUBO_PATH_RUN;
                    _incoming_cmd.instruction = HUBO_PATH_RUN;
                }
                else
                {
                    _current_cmd.instruction = HUBO_PATH_PAUSE;
                    _incoming_cmd.instruction = HUBO_PATH_PAUSE;
                }
            }
            else
            {
                _trajectory.clear();
                _current_cmd.instruction = HUBO_PATH_QUIT;
                _incoming_cmd.instruction = HUBO_PATH_QUIT;
            }
        }
//        report_state();
        return true;
    }

    if( HUBO_PATH_QUIT == _incoming_cmd.instruction
            && HUBO_PATH_QUIT != _current_cmd.instruction )
    {
        release_joints();
        _trajectory.clear();
        _current_cmd = _incoming_cmd;

//        report_state();
        return true;
    }

    if( HUBO_PATH_QUIT == _current_cmd.instruction )
    {
        if( HUBO_PATH_QUIT != _incoming_cmd.instruction )
        {
            if(_receive_incoming_trajectory())
            {
                _current_cmd = _incoming_cmd;
            }
            else
            {
                _trajectory.clear();
            }
        }
        else if( _new_instruction )
        {
            std::cout << "Quit command received -- but we do not have an active trajectory anyway!"
                      << std::endl;
        }
//        report_state();
        return true;
    }

    _current_cmd = _incoming_cmd;

    if(_trajectory.size() == 0)
    {
//        report_state();
        return true;
    }

    if(_new_trajectory)
    {
        _last_elem = _trajectory[0];
        _current_elem = _trajectory[0];
        _current_index = 0;
        _new_trajectory = false;
    }

    if( HUBO_PATH_RUN == _current_cmd.instruction )
    {
        ++_current_index;
        if(_current_index >= _trajectory.size())
            _current_index = _trajectory.size()-1;
    }
    else if( HUBO_PATH_PAUSE == _current_cmd.instruction )
    {
        // No-op
    }
    else if( HUBO_PATH_REVERSE == _current_cmd.instruction )
    {
        --_current_index;
        if(_current_index < 0)
            _current_index = 0;
    }

    _current_elem = _trajectory[_current_index];

    // TODO: Write a controller skeleton and use a controller class instance here

    _send_element_commands(_current_elem);

    _last_elem = _current_elem;
//    report_state();
    return true;
}

void Player::_send_element_commands(const hubo_path_element_t& elem)
{
    for(size_t i=0; i<_desc.joints.size(); ++i)
    {
        if( ((_trajectory.params.bitmap >> i) & 0x01) == 0x01 )
        {
            // TODO: Use different command modes based on the element's control parameters
            // (which do not exist yet)
            set_mode(i, HUBO_CMD_RIGID);

            set_position(i, elem.references[i]);
        }
    }
    send_commands();
}

const hubo_path_element_t& Player::current_element()
{
    return _current_elem;
}
