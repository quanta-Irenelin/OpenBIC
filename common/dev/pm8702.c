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

#include <stdio.h>
#include "sensor.h"
#include "hal_i2c.h"
#include "cci.h"
#include "mctp.h"
#include "plat_mctp.h"
#include "plat_hook.h"
#include "plat_cci.h"

uint8_t pm8702_tmp_read(uint8_t sensor_num, int *reading)
{
	if (!reading || (sensor_num > SENSOR_NUM_MAX)) {
		return SENSOR_UNSPECIFIED_ERROR;
	}
	cci_platform_read(receiver_info->CCI_CMD, receiver_info->ext_params);
	int cxl_temp = get_cxl_temp();
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
