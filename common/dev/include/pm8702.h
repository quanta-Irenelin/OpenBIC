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

#ifndef PM8702_H
#define PM8702_H

void dimm_temp_handler(uint8_t *buf, uint16_t len);
int get_dimm_temp();

/*CCI (pm8702 vendor CMD) */
#define CCI_I2C_OFFSET_READ 0xc401

/*CCI (pm8702 vendor CMD) Request paypload length */
#define I2C_OFFSET_READ_REQ_PL_LEN 20 /*Size Bytes*/

/*CCI (pm8702 vendor CMD) Response paypload length */
#define I2C_OFFSET_READ_RESP_PL_LEN 2 /*Size Bytes*/

uint8_t pm8702_get_dimm_temp(mctp_ext_params ext_params);

// struct cci_health_info_op_pl{
// 	uint32_t health_status;
// 	uint16_t dev_temp;
// 	uint8_t data[12];
// } __attribute__((packed));

#endif