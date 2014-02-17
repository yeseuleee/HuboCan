#ifndef HUBO_INFO_C_H
#define HUBO_INFO_C_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define HUBO_INFO_META_CHANNEL "hubo_info_meta"
#define HUBO_INFO_DATA_CHANNEL "hubo_info_data"
//#define HUBO_JMC_INFO_CHANNEL "hubo_jmc_info"

#define HUBO_JOINT_NAME_MAX_LENGTH 32
#define HUBO_JMC_TYPE_MAX_LENGTH 32

typedef uint8_t hubo_info_data;

typedef struct hubo_meta_info {

    uint8_t joint_count;
    size_t data_size;

}__attribute__((packed)) hubo_meta_info_t;

typedef struct hubo_joint_info {

    char name[HUBO_JOINT_NAME_MAX_LENGTH];
    char jmc_type[HUBO_JMC_TYPE_MAX_LENGTH];
    uint8_t hardware_index;
    uint8_t jmc_index;

}__attribute__((packed)) hubo_joint_info_t;

hubo_info_data* hubo_info_init_data(size_t joint_count);

hubo_info_data* hubo_info_receive_data(double timeout); ///< Will return NULL pointer if there is an error

int hubo_info_send_data(const hubo_info_data* data);

hubo_joint_info_t* hubo_info_get_joint_info(hubo_info_data* data, size_t joint_index);

inline size_t hubo_info_predict_data_size(size_t joint_count)
{
    return sizeof(hubo_meta_info_t) + joint_count*sizeof(hubo_joint_info_t);
}

inline size_t hubo_info_get_data_size(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->data_size;
}

inline size_t hubo_info_get_joint_count(const hubo_info_data* data)
{
    if(data == NULL)
        return 0;

    const hubo_meta_info_t* header = (hubo_meta_info_t*)data;
    return header->joint_count;
}

#endif // HUBO_INFO_C_H
