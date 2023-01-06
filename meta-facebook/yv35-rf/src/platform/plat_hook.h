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

#ifndef PLAT_HOOK_H
#define PLAT_HOOK_H
#include "cci.h"
#include "mctp.h"
#include "plat_mctp.h"
#include "pm8702.h"

typedef struct _isl69254iraz_t_pre_arg_ {
	uint8_t vr_page;
} isl69254iraz_t_pre_arg;

typedef struct _vr_pre_proc_arg {
	/* vr page to set */
	uint8_t vr_page;
} vr_pre_proc_arg;

typedef struct _cci_receiver_info {
	uint16_t CCI_CMD;
	uint16_t CCI_CMD_RESP_PL_LEN;
	mctp_ext_params ext_params;
	uint8_t receiver_bus;
} cci_receiver_info;

typedef struct _dimm_info {
	i2c_offset_read_req dimm_data;
} cci_dimm_info;

/**************************************************************************************************
 * INIT ARGS
**************************************************************************************************/
extern adc_asd_init_arg adc_asd_init_args[];
extern ina230_init_arg SQ5220x_init_args[];
extern ina233_init_arg ina233_init_args[];
extern vr_pre_proc_arg vr_page_select[];
extern cci_receiver_info receiver_info[];
extern cci_dimm_info dimm_info[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK ARGS
 **************************************************************************************************/
extern isl69254iraz_t_pre_arg isl69254iraz_t_pre_read_args[];

/**************************************************************************************************
 *  PRE-HOOK/POST-HOOK FUNC
 **************************************************************************************************/
bool pre_isl69254iraz_t_read(uint8_t sensor_num, void *args);
bool pre_vr_read(uint8_t sensor_num, void *args);
void mctp_cci_set_route_info();
#endif
