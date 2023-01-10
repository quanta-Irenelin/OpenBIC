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

bool pm8702_get_dimm_temp(void *mctp_p, mctp_ext_params ext_params, uint16_t reg_addr, int16_t *interger, int16_t *fraction)
{	
	if (!mctp_p) {
		return false;
	}
	mctp_cci_msg msg = { 0 };
	memcpy(&msg.ext_params, &ext_params, sizeof(mctp_ext_params));

	msg.hdr.op = CCI_I2C_OFFSET_READ;
	msg.hdr.pl_len = I2C_OFFSET_READ_REQ_PL_LEN;

	msg.pl_data = (uint8_t *)malloc(I2C_OFFSET_READ_REQ_PL_LEN);
	if (msg.pl_data == NULL) {
		LOG_ERR("Failed to allocate payload data.");
		return false;
	}

	i2c_offset_read_req req = { 0 }; 
	req.addr_size = ADDR_SIZE_7_BIT,
	req.address = reg_addr, //dimm temp register address  /*Refer to JEDEC SPD*/
	req.offset_size = OFFSET_SIZE_8_BIT,
	req.offset = DIMM_TEMP_REG_OFFSET,  // temperature register offset of the DIMM
	req.timeout_offset = 1,
	req.read_bytes = I2C_OFFSET_READ_RESP_PL_LEN,
	req.timeout_ms = I2C_READ_TIMEOUT_MS,

	memcpy(msg.pl_data, &req, I2C_OFFSET_READ_REQ_PL_LEN);
	
	uint8_t rbuf[I2C_OFFSET_READ_RESP_PL_LEN];
	mctp_cci_read(mctp_p, &msg, rbuf, I2C_OFFSET_READ_RESP_PL_LEN);

	LOG_HEXDUMP_INF(rbuf, I2C_OFFSET_READ_RESP_PL_LEN, __func__);
	int8_t dimm_int = (rbuf[0] << 4) | (rbuf[1] >> 4);
	int8_t dimm_frac = ((rbuf[1] & 0x0F) >> 2) * 25;

	*interger = dimm_int;
	*fraction = dimm_frac;

	free(msg.pl_data);
	return true;
}

uint8_t pm8702_tmp_read(uint8_t sensor_num, int *reading)
{	
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}
	uint8_t port = sensor_config[sensor_config_index_map[sensor_num]].port;
	uint8_t address = sensor_config[sensor_config_index_map[sensor_num]].target_addr;
	uint8_t pm8702_access = sensor_config[sensor_config_index_map[sensor_num]].offset;

	mctp *mctp_inst = NULL;
	mctp_ext_params ext_params = { 0 };
	sensor_val *sval = (sensor_val *)reading;

	if(get_mctp_info_by_eid(port, &mctp_inst, &ext_params) == false){
		return SENSOR_UNSPECIFIED_ERROR;
	}
	switch (pm8702_access) {
		case chip_temp:
			if(cci_get_chip_temp(mctp_inst, ext_params, &sval->integer) == false){
				return SENSOR_NOT_PRESENT;
			}
			sval->fraction = 0;
			break;
		case dimm_temp:
			if(pm8702_get_dimm_temp(mctp_inst, ext_params, address, &sval->integer, &sval->fraction) == false){
				return SENSOR_NOT_PRESENT;
			}
			sval->fraction = sval->fraction * 10;
			break;
		default:
			printf("%s: Invalid access offset %d\n", __func__, pm8702_access);
			break;
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
