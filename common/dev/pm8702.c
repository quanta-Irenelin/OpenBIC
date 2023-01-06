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
#include <stdlib.h>
#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "cci.h"
#include "mctp.h"
#include <logging/log.h>
#include "plat_mctp.h"
#include "plat_hook.h"
#include "pm8702.h"

LOG_MODULE_REGISTER(pm8702);

uint8_t pm8702_tmp_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}
	mctp *mctp_inst = get_mctp_init();
	int16_t cxl_temp = 0;
	if(cci_get_chip_temp(mctp_inst, receiver_info->ext_params, &cxl_temp) == false){
		return SENSOR_NOT_PRESENT;
	}
	sensor_val *sval = (sensor_val *)reading;
	sval->integer = cxl_temp;
	sval->fraction = 0;

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

static uint8_t pl_data[20] =
{
    0x00, 0x00, 0x19, 0x00, 
    0x01, 0x00, 0x05, 0x00, 
    0x01, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 
};


uint8_t pm8702_get_dimm_temp(mctp_ext_params ext_params)
{
    mctp *mctp_init = get_mctp_init();

    mctp_cci_msg msg = { 0 };
    memcpy(&msg.ext_params, &ext_params, sizeof(mctp_ext_params));
	
	msg.hdr.op = CCI_I2C_OFFSET_READ;
    msg.hdr.pl_len = I2C_OFFSET_READ_REQ_PL_LEN;
	msg.pl_data = (uint8_t *)malloc(I2C_OFFSET_READ_REQ_PL_LEN);
	if (msg.pl_data == NULL) {
		LOG_ERR("Failed to allocate payload data.");
		return -1;
	}
    memcpy(msg.pl_data, pl_data, I2C_OFFSET_READ_REQ_PL_LEN);

	int resp_len = I2C_OFFSET_READ_RESP_PL_LEN;
    uint8_t rbuf[resp_len];
    mctp_cci_read(mctp_init, &msg, rbuf, resp_len);

	LOG_HEXDUMP_INF(rbuf, resp_len, __func__);
    // uint8_t dev_temp = rbuf[DEV_TEMP_OFFSET];
    printf("dimm temp: %02x, %02x\n", rbuf[0], rbuf[1]);	
	free(msg.pl_data);
	return 0;
}
