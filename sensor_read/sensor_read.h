#ifndef SENSOR_READ_H_
#define SENSOR_READ_H_

rt_err_t sensor_read_by_type(uint8_t sensor_type, uint8_t* target, uint8_t* source, uint8_t source_len);

#endif /* SENSOR_READ_H_*/
