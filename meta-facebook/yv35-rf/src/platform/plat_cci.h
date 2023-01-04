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

#ifndef _PLAT_CCI_H
#define _PLAT_CCI_H
#include "mctp.h"
typedef struct _opcode_route_entry {
	uint16_t op_code;
	uint16_t CCI_REQ_PL_LEN; /*CCI Request paypload length */
	uint16_t CCI_RESP_PL_LEN; /*CCI Response paypload length */
} opcode_route_entry;


uint16_t cci_platform_read(uint32_t cci_opcode, mctp_ext_params ext_params);
void health_info_handler(uint8_t *buf, uint16_t len);
void dimm_temp_handler(uint8_t *buf, uint16_t len);


// void health_info_handler(void *args, uint8_t *buf, uint16_t len);
int get_cxl_temp();

#endif /* _PLAT_CCI_H */