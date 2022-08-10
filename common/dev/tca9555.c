#include <stdio.h>
#include <string.h>

#include "tca9555.h"
#include "sensor.h"
#include "hal_i2c.h"

#define I2C_RETRY 5

bool tca9555_config_gpio_direction(uint8_t sensor_num, void *args)
{
	if (!args || (sensor_num > SENSOR_NUM_MAX)) {
		return false;
	}

	struct tca9555 *p = (struct tca9555 *)args;

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	//msg.bus = cfg->port;
	/* change address to 7-bit */
	msg.target_addr = p->regs;
	msg.tx_len = 1;
	msg.data[0] = p -> gpio_direction;

	if (i2c_master_write(&msg, retry))
		return false;

	return true;
}


uint8_t tca9555_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port; //i2c
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;// i/o expander address
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = sensor_config[sensor_config_index_map[sensor_num]].offset; //TCA9555_OUTPUT_PORT0

	if (i2c_master_read(&msg, retry))
		return SENSOR_FAIL_TO_ACCESS;

	gpio_val *gval = (gpio_val *)reading;
	gval-> regs = msg.data[0];
	gval-> gpio_direction = 0;
	gval-> gpio_status = 0;

	return SENSOR_READ_SUCCESS;
}

bool tca9555_write(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = sensor_config[sensor_config_index_map[sensor_num]].port; //i2c
	msg.target_addr = sensor_config[sensor_config_index_map[sensor_num]].target_addr;// i/o expander address
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = sensor_config[sensor_config_index_map[sensor_num]].offset; //TCA9555_OUTPUT_PORT0
	//msg.data[1] = sensor_config[sensor_config_index_map[sensor_num]].offset; //write data

	if (i2c_master_write(&msg, retry))
		return false;
	return true;
}

uint8_t tca9555_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}

	sensor_config[sensor_config_index_map[sensor_num]].read = tca9555_read;
	return SENSOR_INIT_SUCCESS;
}
