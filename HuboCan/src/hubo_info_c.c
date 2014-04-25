
#include "HuboCan/AchIncludes.h"
#include "HuboCan/hubo_info_c.h"
#include <stdlib.h>
#include <stdio.h>

size_t hubo_info_get_joint_count(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->joint_count;
}

size_t hubo_info_get_jmc_count(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->jmc_count;
}

size_t hubo_info_get_sensor_count(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->sensor_count;
}

size_t hubo_info_get_joint_location(const hubo_info_data* data, size_t joint_index)
{
    return sizeof(hubo_meta_info_t)
            + joint_index*sizeof(hubo_joint_info_t);
}

size_t hubo_info_get_jmc_location(const hubo_info_data* data, size_t jmc_index)
{
    return sizeof(hubo_meta_info_t)
            + hubo_info_get_joint_count(data)*sizeof(hubo_joint_info_t)
            + jmc_index*sizeof(hubo_jmc_info_t);
}

size_t hubo_info_get_sensor_location(const hubo_info_data* data, size_t sensor_index)
{
    return sizeof(hubo_meta_info_t)
            + hubo_info_get_joint_count(data)*sizeof(hubo_joint_info_t)
            + hubo_info_get_jmc_count(data)*sizeof(hubo_jmc_info_t)
            + sensor_index*sizeof(hubo_sensor_info_t);
}

size_t hubo_info_predict_data_size(size_t joint_count, size_t jmc_count, size_t sensor_count)
{
    return sizeof(hubo_meta_info_t)
            + joint_count*sizeof(hubo_joint_info_t)
            + jmc_count*sizeof(hubo_jmc_info_t)
            + sensor_count*sizeof(hubo_sensor_info_t);
}

inline size_t hubo_info_get_data_size(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->data_size;
}

hubo_info_data* hubo_info_init_data(size_t joint_count, size_t jmc_count, size_t sensor_count)
{
    size_t data_size = hubo_info_predict_data_size(joint_count, jmc_count, sensor_count);
    hubo_info_data* new_info = malloc(data_size);
    memset(new_info, 0, data_size);

    hubo_meta_info_t* meta_info = (hubo_meta_info_t*)new_info;
    strcpy(meta_info->code, HUBO_INFO_META_CODE);
    meta_info->joint_count = joint_count;
    meta_info->jmc_count = jmc_count;
    meta_info->sensor_count = sensor_count;
    meta_info->data_size = data_size;

    return new_info;
}

hubo_info_data* hubo_info_receive_data(double timeout)
{
    ach_channel_t meta_channel;
    ach_status_t r = ach_open(&meta_channel, HUBO_INFO_META_CHANNEL, NULL);
    if( ACH_OK != r )
    {
        fprintf(stderr, "Could not open Hubo's meta info channel \"%s\"\n"
                " -- Ach Error: %s (%d)\n"
                " -- Make sure you are using the Manager correctly\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
        return NULL;
    }

    hubo_meta_info_t meta_info;

    size_t fs;
    struct timespec wait_time;
    clock_gettime( ACH_DEFAULT_CLOCK, &wait_time );
    int nano_wait = wait_time.tv_nsec + (int)(timeout*1E9);
    wait_time.tv_sec += (int)(nano_wait/1E9);
    wait_time.tv_nsec = (int)(nano_wait%((int)1E9));
    r = ach_get(&meta_channel, &meta_info, sizeof(hubo_meta_info_t),
                &fs, &wait_time, ACH_O_LAST | ACH_O_WAIT);

    ach_close(&meta_channel);

    if( ACH_TIMEOUT == r )
    {
        fprintf(stderr, "Hubo Meta Info has not been published within the time limit\n"
                " -- We are giving up and returning a NULL pointer\n");
        return NULL;
    }

    if( ACH_OK != r && ACH_STALE_FRAMES != r  && ACH_MISSED_FRAME != r )
    {
        fprintf(stderr, "Unexpected ach result on Hubo's meta info channel \"%s\":\n"
                " -- Ach Error: %s (%d)\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
        return NULL;
    }

    size_t joint_count = meta_info.joint_count;
    size_t jmc_count = meta_info.jmc_count;
    size_t sensor_count = meta_info.sensor_count;

    ach_channel_t info_channel;
    r = ach_open(&info_channel, HUBO_INFO_DATA_CHANNEL, NULL);
    if( ACH_OK != r )
    {
        fprintf(stderr, "Could not open Hubo's joint info channel \"%s\"\n"
                " -- Ach Error: %s (%d)\n"
                " -- Make sure you are using the Manager correctly\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
        return NULL;
    }

    size_t info_data_size = hubo_info_predict_data_size(joint_count, jmc_count, sensor_count);
    hubo_info_data* data = malloc(info_data_size);
    memset(data, 0, info_data_size);

    clock_gettime( ACH_DEFAULT_CLOCK, &wait_time );
    nano_wait = wait_time.tv_nsec + (int)(timeout*1E9);
    wait_time.tv_sec += (int)(nano_wait/1E9);
    wait_time.tv_nsec = (int)(nano_wait%((int)1E9));
    r = ach_get(&info_channel, data, info_data_size,
                &fs, &wait_time, ACH_O_LAST | ACH_O_WAIT);

    ach_close(&info_channel);

    if( ACH_TIMEOUT == r )
    {
        fprintf(stderr, "Hubo Info Data has not been published within the time limit\n"
                " -- The contents of the hubo_info_data_t will be left blank\n");
        return data;
    }

    if( ACH_OVERFLOW == r )
    {
        fprintf(stderr, "Ach Overflow! Mismatch between predicted data size (%zu) and the size of the message in the channel (%zu)!\n",
                info_data_size, fs);
    }
    else if( ACH_OK != r && ACH_STALE_FRAMES != r  && ACH_MISSED_FRAME != r )
    {
        fprintf(stderr, "Unexpected ach result on Hubo's info data channel \"%s\":\n"
                " -- Ach Error: %s (%d)\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
    }

    if( hubo_info_get_data_size(data) != info_data_size )
    {
        fprintf(stderr, "Mismatch between predicted data size (%zu) and believed data size (%zu) for Hubo Info Data!\n",
                info_data_size, hubo_info_get_data_size(data));
    }

    return data;
}

int hubo_info_send_data(const hubo_info_data *data)
{
    hubo_meta_info_t* meta_info = (hubo_meta_info_t*)data;

    ach_channel_t meta_channel;
    ach_status_t r = ach_open(&meta_channel, HUBO_INFO_META_CHANNEL, NULL);
    if( ACH_OK != r )
    {
        fprintf(stderr, "Could not open Hubo's meta info channel \"%s\"\n"
                " -- Ach Error: %s (%d)\n"
                " -- The Manager should open this channel if you are using it correctly\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
        return -1;
    }

    ach_put(&meta_channel, meta_info, sizeof(hubo_meta_info_t));

    ach_close(&meta_channel);

    ach_channel_t info_channel;
    r = ach_open(&info_channel, HUBO_INFO_DATA_CHANNEL, NULL);
    if( ACH_OK != r )
    {
        fprintf(stderr, "Could not open Hubo's joint info channel \"%s\"\n"
                " -- Ach Error: %s (%d)\n"
                " -- The Manager should open this channel if you are using it correctly\n",
                HUBO_INFO_META_CHANNEL, ach_result_to_string(r), (int)r);
        return -2;
    }

    ach_put(&info_channel, data, meta_info->data_size);

    ach_close(&info_channel);

    return 0;
}

hubo_params_info_t* hubo_info_get_params_info(hubo_info_data* data)
{
    if(data == NULL)
    {
        return NULL;
    }

    return &( ( (hubo_meta_info_t*)data )->params );
}

hubo_joint_info_t* hubo_info_get_joint_info(hubo_info_data *data, size_t joint_index)
{
    if(joint_index >= hubo_info_get_joint_count(data))
    {
        fprintf(stderr, "Error: requested joint info (index %zu) which is out of bounds (%zu)!\n",
                joint_index, hubo_info_get_joint_count(data));
        return NULL;
    }

    return (hubo_joint_info_t*)(data+hubo_info_get_joint_location(data, joint_index));
}

hubo_jmc_info_t* hubo_info_get_jmc_info(const hubo_info_data *data, size_t jmc_index)
{
    if(jmc_index >= hubo_info_get_jmc_count(data))
    {
        fprintf(stderr, "Error: requested jmc info (index %zu) which is out of bounds (%zu)!\n",
                jmc_index, hubo_info_get_jmc_count(data));
        return NULL;
    }

    return (hubo_jmc_info_t*)(data+hubo_info_get_jmc_location(data, jmc_index));
}

hubo_sensor_info_t* hubo_info_get_sensor_info(const hubo_info_data *data, size_t sensor_index)
{
    if(sensor_index >= hubo_info_get_sensor_count(data))
    {
        fprintf(stderr, "Error: requesting sensor info (index %zu) which is out of bounds (%zu)!\n",
                sensor_index, hubo_info_get_sensor_count(data));
        return NULL;
    }

    return (hubo_sensor_info_t*)(data+hubo_info_get_sensor_location(data, sensor_index));
}
