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


/*CCI_I2C_OFFSET_READ parameters */
#define addr_size_7_BIT  0x00  
#define addr_size_10_BIT  0x01

#define offset_size_8_BIT  0x01  
#define offset_size_16_BIT  0x02 

typedef struct {
	uint8_t addr_size;
	uint8_t rsvd_0;
	uint16_t address;
	uint8_t offset_size;
	uint8_t rsvd_1;
	uint16_t offset;
	uint32_t timeout_offset;
	uint32_t read_bytes;
    uint32_t timeout_ms;
} i2c_offset_read_req;

typedef struct _cci_receiver_info {
	mctp_ext_params ext_params;
	uint8_t receiver_bus;
} cci_receiver_info;

typedef struct _dimm_info {
	mctp_ext_params ext_params;
	i2c_offset_read_req dimm_data;
} cci_dimm_info;

typedef struct {
	uint8_t interger;
	uint8_t fraction;
} temp;

bool pm8702_get_dimm_temp(void *mctp_p, mctp_ext_params ext_params, i2c_offset_read_req pl_data, uint8_t *interger, uint8_t *fraction);

#endif