/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "cci.h"
#include "mctp.h"
#include <logging/log.h>
#include "pm8702.h"


LOG_MODULE_REGISTER(pm8702);


bool pm8702_get_dimm_temp(void *mctp_p, mctp_ext_params ext_params, i2c_offset_read_req pl_data, uint8_t *interger, uint8_t *fraction)
{
	mctp_cci_msg msg = { 0 };
	memcpy(&msg.ext_params, &ext_params, sizeof(mctp_ext_params));

	msg.hdr.op = CCI_I2C_OFFSET_READ;
	msg.hdr.pl_len = I2C_OFFSET_READ_REQ_PL_LEN;

	msg.pl_data = (uint8_t *)malloc(I2C_OFFSET_READ_REQ_PL_LEN);
	if (msg.pl_data == NULL) {
		LOG_ERR("Failed to allocate payload data.");
		return false;
	}
	memcpy(msg.pl_data, &pl_data, I2C_OFFSET_READ_REQ_PL_LEN);
	
	uint8_t rbuf[I2C_OFFSET_READ_RESP_PL_LEN];
	mctp_cci_read(mctp_p, &msg, rbuf, I2C_OFFSET_READ_RESP_PL_LEN);

	LOG_HEXDUMP_INF(rbuf, I2C_OFFSET_READ_RESP_PL_LEN, __func__);
	*interger = (rbuf[0] << 4) | (rbuf[1] >> 4);
	*fraction = ((rbuf[1] & 0x0F) >> 2) * 25;


	free(msg.pl_data);
	return true;
}

uint8_t pm8702_tmp_read(uint8_t sensor_num, int *reading)
{	
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}
	uint8_t port = sensor_config[sensor_config_index_map[sensor_num]].port;
	uint8_t offset = sensor_config[sensor_config_index_map[sensor_num]].offset;

	mctp *mctp_inst = { 0 };
	
	if(get_mctp_inst_by_eid(port, &mctp_inst) == false){
		return SENSOR_UNSPECIFIED_ERROR;
	}

	if(offset == 0x01){
		cci_receiver_info *init_args;
		init_args = (cci_receiver_info *)sensor_config[sensor_config_index_map[sensor_num]].init_args;

		int16_t cxl_temp = 0;
		if (cci_get_chip_temp(mctp_inst, init_args->ext_params, &cxl_temp) == false) {
			return SENSOR_NOT_PRESENT;
		}
		printk("device temp : %d\n", cxl_temp);
		sensor_val *sval = (sensor_val *)reading;
		sval->integer = cxl_temp;
		sval->fraction = 0;
		
	}else if(offset == 0x05){
		cci_dimm_info *init_args;
		init_args = (cci_dimm_info *)sensor_config[sensor_config_index_map[sensor_num]].init_args;
		uint8_t interger = 0;
		uint8_t fraction = 0;
		pm8702_get_dimm_temp(mctp_inst, init_args->ext_params,init_args->dimm_data, &interger, &fraction);
		sensor_val *sval = (sensor_val *)reading;
		sval->integer = interger;
		sval->fraction = fraction;
	}

	return SENSOR_READ_SUCCESS;
}


uint8_t pm8702_init(uint8_t sensor_num)
{
	if (sensor_num > SENSOR_NUM_MAX) {
		return SENSOR_INIT_UNSPECIFIED_ERROR;
	}
	sensor_config[sensor_config_index_map[sensor_num]].read = pm8702_tmp_read;
	return SENSOR_INIT_SUCCESS;
}
