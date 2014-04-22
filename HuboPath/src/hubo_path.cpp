
#include "../hubo_path.hpp"
#include <sstream>

HuboCan::error_result_t HuboPath::send_trajectory(ach_channel_t &output_channel,
                                            ach_channel_t &feedback_channel,
                                            const Trajectory& trajectory,
                                            int max_wait_time)
{
    std::cout << "Attempted to send trajectory... " << std::endl;
    
    hubo_path_rx_t feedback;
    memset(&feedback, 0, sizeof(feedback));

    ach_status_t result;
    struct timespec t;
    size_t fs;
    int attempts = 0;

    do {

        clock_gettime(ACH_DEFAULT_CLOCK, &t);
        t.tv_sec += 1; ++attempts;
        result = ach_get( &feedback_channel, &feedback, sizeof(feedback), &fs, &t,
                         ACH_O_WAIT | ACH_O_LAST );

        if(ACH_OK != result && ACH_TIMEOUT != result && ACH_MISSED_FRAME != result)
        {
            std::cout << "Unexpected ach result while sending trajectory: "
                      << ach_result_to_string(result) << std::endl;
            return HuboCan::ACH_ERROR;
        }

    } while( feedback.state != PATH_RX_READ_READY && attempts <= max_wait_time );

    if(attempts > max_wait_time)
    {
        std::cout << "We did not receive readiness acknowledgment from listener\n"
                  << " -- Status: " << feedback << "\n"
                  << " -- We will NOT send off the trajectory!" << std::endl;
    }

    uint32_t total_chunks = ceil((double)(trajectory.size())/(double)(HUBO_PATH_CHUNK_MAX_SIZE));

    hubo_path_chunk_t chunk;
    size_t counter = 0;
    for(uint32_t i=0; i<total_chunks; ++i)
    {
        memset(&chunk, 0, sizeof(chunk));
        chunk.chunk_id = i;
        chunk.total_chunks = total_chunks;
        
        size_t c = 0;
        while( c < HUBO_PATH_CHUNK_MAX_SIZE && counter < trajectory.size() )
        {
            chunk.elements[c] = trajectory[counter];
            ++c; ++counter;
        }
        chunk.chunk_size = c;
        
        ach_put(&output_channel, &chunk, sizeof(chunk));
        
        clock_gettime( ACH_DEFAULT_CLOCK, &t );
        t.tv_sec += max_wait_time;
        result = ach_get( &feedback_channel, &feedback, sizeof(feedback), &fs,
                          &t, ACH_O_WAIT | ACH_O_LAST );
        
        if( ACH_TIMEOUT == result )
        {
            std::cout << "We did not receive acknowledgment from the listener at chunk "
                      << i << " of the trajectory\n"
                      << " -- Last received acknowledgment: " << feedback << "\n"
                      << " -- We will STOP sending off the trajectory!" << std::endl;
            return HuboCan::TIMEOUT;
        }
        
        if( feedback.chunk_id != i )
        {
            std::cout << "Inconsistent Chunk ID in acknowledgment from listener!\n"
                      << " -- Received:" << feedback.chunk_id << ", Expected:" << i << "\n"
                      << " -- Status: " << feedback << std::endl;
            return HuboCan::SYNCH_ERROR;
        }
        
        if( PATH_RX_FINISHED == feedback.state && feedback.chunk_id == total_chunks-1 )
        {
            std::cout << "Finished sending trajectory!" << std::endl;
            return HuboCan::OKAY;
        }
        else if( PATH_RX_FINISHED == feedback.state && feedback.chunk_id != total_chunks-1 )
        {
            std::cout << "Received inappropriate report of being finished!\n"
                      << " -- Current chunk:" << i << ", Final chunk:" << total_chunks-1 << "\n"
                      << " -- Status: " << feedback << std::endl;
            return HuboCan::SYNCH_ERROR;
        }
        
        if( PATH_RX_LISTENING != feedback.state )
        {
            std::cout << "Error reported by trajectory receiver: " << feedback << std::endl;
            return HuboCan::INTERRUPTED;
        }
    }

    return HuboCan::UNDEFINED_ERROR;
}

HuboCan::error_result_t HuboPath::receive_trajectory(ach_channel_t &input_channel,
                                               ach_channel_t &feedback_channel,
                                               Trajectory &new_trajectory,
                                               int max_wait_time)
{
    std::cout << "Attempting to receive trajectory... " << std::endl;
    
    new_trajectory.clear();
    
    hubo_path_rx_t feedback;
    memset(&feedback, 0, sizeof(feedback));
    feedback.state = PATH_RX_READ_READY;
    ach_put(&feedback_channel, &feedback, sizeof(feedback));
    
    hubo_path_chunk_t chunk;
    int chunk_id=-1, total_chunks=1;
    struct timespec t;
    size_t fs;
    ach_status_t result;
    
    while(chunk_id < total_chunks-1)
    {
        clock_gettime(ACH_DEFAULT_CLOCK, &t);
        t.tv_sec += max_wait_time;
        result = ach_get(&input_channel, &chunk, sizeof(chunk), &fs, &t, ACH_O_WAIT);
        
        feedback.chunk_id = chunk.chunk_id;
        feedback.expected_size = chunk.total_chunks;
        
        if( ACH_TIMEOUT == result )
        {
            std::cout << "Did not receive next chunk from the sender in "
                      << max_wait_time << "seconds\n"
                      << " -- We are giving up on this trajectory!" << std::endl;
            feedback.state = PATH_RX_TIMEOUT;
            ach_put(&feedback_channel, &feedback, sizeof(feedback));
            return HuboCan::TIMEOUT;
        }
        else if( ACH_OK != result )
        {
            std::cout << "Unexpected Ach result: " << ach_result_to_string(result)
                      << " -- We are giving up on this trajectory!" << std::endl;
            feedback.state = PATH_RX_ACH_ERROR;
            ach_put(&feedback_channel, &feedback, sizeof(feedback));
            return HuboCan::ACH_ERROR;
        }
        
        if( chunk.chunk_id == PATH_TX_CANCEL )
        {
            std::cout << "Received a request to cancel the trajectory transfer\n"
                      << " -- We are giving up on this trajectory!" << std::endl;
            feedback.state = PATH_RX_CANCELED;
            ach_put(&feedback_channel, &feedback, sizeof(feedback));
            return HuboCan::INTERRUPTED;
        }
        
        if(chunk.chunk_id == chunk_id+1)
        {
            for(uint32_t i=0; i<chunk.chunk_size; ++i)
            {
                new_trajectory.push_back(chunk.elements[i]);
            }
        }
        else
        {
            std::cout << "Received discontinuous Chunk ID: "
                      << chunk_id << " -> " << chunk.chunk_id
                      << " -- We are giving up on this trajectory!" << std::endl;
            feedback.state = PATH_RX_DISCONTINUITY;
            ach_put(&feedback_channel, &feedback, sizeof(feedback));
            return HuboCan::SYNCH_ERROR;
        }
        
        chunk_id = chunk.chunk_id;
        total_chunks = chunk.total_chunks;
        
        if(chunk_id < total_chunks-1)
            feedback.state = PATH_RX_LISTENING;
        else if(chunk_id == total_chunks-1)
            feedback.state = PATH_RX_FINISHED;
        
        ach_put(&feedback_channel, &feedback, sizeof(feedback));
    }
    
    std::cout << "Finished receiving trajectory!" << std::endl;
    return HuboCan::OKAY;
}

const char* hubo_path_interp_to_string(hubo_path_interp_t type)
{
    switch(type)
    {
        case HUBO_PATH_SPLINE:      return "HUBO_PATH_SPLINE";  break;
        case HUBO_PATH_OPTIMIZE:    return "HUBO_PATH_OPTIMIZE";break;
        case HUBO_PATH_DENSIFY:     return "HUBO_PATH_DENSIFY"; break;
        case HUBO_PATH_RAW:         return "HUBO_PATH_RAW";     break;
        default:                    return "HUBO_PATH_UNKNOWN"; break;
    }

    return "HUBO_PATH_IMPOSSIBLE";
}

std::ostream& operator<<(std::ostream& stream, const hubo_path_interp_t& type)
{
    return (stream << hubo_path_interp_to_string(type));
}

const char* hubo_path_instruction_to_string(hubo_path_instruction_t type)
{
    switch(type)
    {
        case HUBO_PATH_QUIT:        return "HUBO_PATH_QUIT";    break;
        case HUBO_PATH_RUN:         return "HUBO_PATH_RUN";     break;
        case HUBO_PATH_PAUSE:       return "HUBO_PATH_PAUSE";   break;
        case HUBO_PATH_REVERSE:     return "HUBO_PATH_REVERSE"; break;
        case HUBO_PATH_LOAD:        return "HUBO_PATH_LOAD";    break;
        default:                    return "HUBO_PATH_UNKNOWN"; break;
    }

    return "HUBO_PATH_IMPOSSIBLE";
}

std::ostream& operator<<(std::ostream& stream, const hubo_path_instruction_t& type)
{
    return (stream << hubo_path_instruction_to_string(type));
}

const char* hubo_path_rx_state_to_string(hubo_path_rx_state_t state)
{
    switch(state)
    {
        case PATH_RX_IGNORING:      return "PATH_RX_IGNORING";      break;
        case PATH_RX_READ_READY:    return "PATH_RX_READ_READY";    break;
        case PATH_RX_LISTENING:     return "PATH_RX_LISTENING";     break;
        case PATH_RX_TIMEOUT:       return "PATH_RX_TIMEOUT";       break;
        case PATH_RX_DISCONTINUITY: return "PATH_RX_DISCONTINUITY"; break;
        case PATH_RX_ACH_ERROR:     return "PATH_RX_ACH_ERROR";     break;
        case PATH_RX_FINISHED:      return "PATH_RX_FINISHED";      break;
        case PATH_RX_CANCELED:      return "PATH_RX_CANCELED";      break;
        default:                    return "PATH_RX_UNKNOWN";       break;
    }

    return "PATH_RX_IMPOSSIBLE";
}

std::ostream& operator<<(std::ostream& stream, const hubo_path_rx_state_t& state)
{
    return (stream << hubo_path_rx_state_to_string(state));
}

const char* hubo_path_rx_to_string(hubo_path_rx_t rx)
{
    std::stringstream sstream;
    sstream << "Status:" << rx.state << ", Chunk ID:" << rx.chunk_id
               << ", Expected Size:" << rx.expected_size;
    return sstream.str().c_str();
}

std::ostream& operator<<(std::ostream& stream, const hubo_path_rx_t& rx)
{
    return (stream << hubo_path_rx_to_string(rx));
}


