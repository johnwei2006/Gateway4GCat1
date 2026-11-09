#include "rtthread.h"
#include "sensor_read.h"

rt_err_t sensor_read_by_type(uint8_t sensor_type, uint8_t* target, uint8_t* source, uint8_t source_len)
{
    if(target == NULL || source == NULL || source_len < 1)
    {
        rt_kprintf("sensor_read_by_type: invalid parameters\n");
        return RT_EINVAL;
    }
    if(sensor_type == 0){

    } else if(sensor_type == 1){

    } else {
        for(int i = 0; i < source_len; i++){
            target[0] |= source[i] << 8*(source_len-1-i);
        }
    }
    return RT_EOK;
}
