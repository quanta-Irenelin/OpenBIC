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
#include <stdlib.h>
#include <logging/log.h>
#include "ipmi.h"
#include "libutil.h"
#include "plat_ipmi.h"
#include "plat_ipmb.h"
#include "expansion_board.h"

#include "oem_1s_handler.h"
#include "cci.h"
#include "mctp.h"
#include "plat_mctp.h"

LOG_MODULE_REGISTER(plat_ipmi);

void OEM_1S_GET_BOARD_ID(ipmi_msg *msg)
{
	if (msg == NULL) {
		printf("%s failed due to parameter *msg is NULL\n", __func__);
		return;
	}

	if (msg->data_len != 0) {
		msg->data_len = 0;
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	msg->data_len = 1;
	msg->data[0] = get_board_id();
	msg->completion_code = CC_SUCCESS;
	return;
}

void OEM_1S_GET_FW_VERSION(ipmi_msg *msg)
{
	CHECK_NULL_ARG(msg);

	if (msg->data_len != 1) {
		msg->completion_code = CC_INVALID_LENGTH;
		return;
	}

	uint8_t version[16] = { 0 }; 
	mctp *mctp_inst = NULL;
	mctp_ext_params ext_params = { 0 };

	uint8_t component;
	component = msg->data[0];
#if MAX_IPMB_IDX
	ipmb_error status;
	ipmi_msg *bridge_msg;
#endif

#ifdef ENABLE_ISL69260
	I2C_MSG i2c_msg;
	uint8_t retry = 3;
#endif

	switch (component) {
	case RF_COMPNT_CPLD:
		msg->completion_code = CC_UNSPECIFIED_ERROR;
		break;
	case RF_COMPNT_BIC:
		msg->data[0] = BIC_FW_YEAR_MSB;
		msg->data[1] = BIC_FW_YEAR_LSB;
		msg->data[2] = BIC_FW_WEEK;
		msg->data[3] = BIC_FW_VER;
		msg->data[4] = BIC_FW_platform_0;
		msg->data[5] = BIC_FW_platform_1;
		msg->data[6] = BIC_FW_platform_2;
		msg->data_len = 7;
		msg->completion_code = CC_SUCCESS;
		break;
	case RF_COMPNT_CXL:
		get_mctp_info_by_eid(0x2E, &mctp_inst, &ext_params);
		cci_get_chip_version(mctp_inst,ext_params, version);
		for(int i=0; i<16; i++){ 
			msg->data[i] = version[i];
		}
		msg->data_len = 16;
		msg->completion_code = CC_SUCCESS;
		break;
#if MAX_IPMB_IDX
	case RF_COMPNT_ME:
		if ((IPMB_inf_index_map[ME_IPMB] == RESERVED) ||
		    (IPMB_config_table[IPMB_inf_index_map[ME_IPMB]].enable_status == DISABLE)) {
			LOG_ERR("IPMB ME interface not enabled.");
			msg->completion_code = CC_UNSPECIFIED_ERROR;
			return;
		}

		bridge_msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
		if (bridge_msg == NULL) {
			msg->completion_code = CC_OUT_OF_SPACE;
			return;
		}
		bridge_msg->data_len = 0;
		bridge_msg->seq_source = 0xff;
		bridge_msg->InF_source = SELF;
		bridge_msg->InF_target = ME_IPMB;
		bridge_msg->netfn = NETFN_APP_REQ;
		bridge_msg->cmd = CMD_APP_GET_DEVICE_ID;

		status = ipmb_read(bridge_msg, IPMB_inf_index_map[bridge_msg->InF_target]);
		if (status != IPMB_ERROR_SUCCESS) {
			LOG_ERR("ipmb read fail status: %x", status);
			SAFE_FREE(bridge_msg);
			msg->completion_code = CC_BRIDGE_MSG_ERR;
			return;
		} else {
			msg->data[0] = bridge_msg->data[2] & 0x7F;
			msg->data[1] = bridge_msg->data[3] >> 4;
			msg->data[2] = bridge_msg->data[3] & 0x0F;
			msg->data[3] = bridge_msg->data[12];
			msg->data[4] = bridge_msg->data[13] >> 4;
			msg->data_len = 5;
			msg->completion_code = CC_SUCCESS;
			SAFE_FREE(bridge_msg);
		}
		break;
#endif
#ifdef ENABLE_ISL69260
	case RF_COMPNT_PVCCIN:
	case RF_COMPNT_PVCCFA_EHV_FIVRA:
	case RF_COMPNT_PVCCD_HV:
	case RF_COMPNT_PVCCINFAON:
	case RF_COMPNT_PVCCFA_EHV:
		if ((component == COMPNT_PVCCIN) || (component == COMPNT_PVCCFA_EHV_FIVRA)) {
			i2c_msg.target_addr = PVCCIN_ADDR;
		}
		if (component == COMPNT_PVCCD_HV) {
			i2c_msg.target_addr = PVCCD_HV_ADDR;
		}
		if ((component == COMPNT_PVCCINFAON) || (component == COMPNT_PVCCFA_EHV)) {
			i2c_msg.target_addr = PVCCFA_EHV_ADDR;
		}
		i2c_msg.tx_len = 1;
		i2c_msg.rx_len = 7;
		i2c_msg.bus = I2C_BUS5;
		i2c_msg.data[0] = PMBUS_IC_DEVICE_ID;

		if (i2c_master_read(&i2c_msg, retry)) {
			msg->completion_code = CC_UNSPECIFIED_ERROR;
			return;
		}

		if (i2c_msg.data[0] == 0x04 && i2c_msg.data[1] == 0x00 && i2c_msg.data[2] == 0x81 &&
		    i2c_msg.data[3] == 0xD2 && i2c_msg.data[4] == 0x49) {
			/* Renesas isl69259 */
			i2c_msg.tx_len = 3;
			i2c_msg.data[0] = 0xC7;
			i2c_msg.data[1] = 0x94;
			i2c_msg.data[2] = 0x00;

			if (i2c_master_write(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			i2c_msg.tx_len = 1;
			i2c_msg.rx_len = 4;
			i2c_msg.data[0] = 0xC5;

			if (i2c_master_read(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			msg->data[0] = i2c_msg.data[3];
			msg->data[1] = i2c_msg.data[2];
			msg->data[2] = i2c_msg.data[1];
			msg->data[3] = i2c_msg.data[0];
			msg->data_len = 4;
			msg->completion_code = CC_SUCCESS;

		} else if (i2c_msg.data[0] == 0x06 && i2c_msg.data[1] == 0x54 &&
			   i2c_msg.data[2] == 0x49 && i2c_msg.data[3] == 0x53 &&
			   i2c_msg.data[4] == 0x68 && i2c_msg.data[5] == 0x90 &&
			   i2c_msg.data[6] == 0x00) {
			/* TI tps53689 */
			i2c_msg.tx_len = 1;
			i2c_msg.rx_len = 2;
			i2c_msg.data[0] = 0xF4;

			if (i2c_master_read(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			msg->data[0] = i2c_msg.data[1];
			msg->data[1] = i2c_msg.data[0];
			msg->data_len = 2;
			msg->completion_code = CC_SUCCESS;

		} else if (i2c_msg.data[0] == 0x02 && i2c_msg.data[2] == 0x8A) {
			/* Infineon xdpe15284 */
			i2c_msg.tx_len = 6;
			i2c_msg.data[0] = 0xFD;
			i2c_msg.data[1] = 0x04;
			i2c_msg.data[2] = 0x00;
			i2c_msg.data[3] = 0x00;
			i2c_msg.data[4] = 0x00;
			i2c_msg.data[5] = 0x00;

			if (i2c_master_write(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			i2c_msg.tx_len = 2;
			i2c_msg.data[0] = 0xFE;
			i2c_msg.data[1] = 0x2D;

			if (i2c_master_write(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			i2c_msg.tx_len = 1;
			i2c_msg.rx_len = 5;
			i2c_msg.data[0] = 0xFD;

			if (i2c_master_read(&i2c_msg, retry)) {
				msg->completion_code = CC_UNSPECIFIED_ERROR;
				return;
			}

			msg->data[0] = i2c_msg.data[4];
			msg->data[1] = i2c_msg.data[3];
			msg->data[2] = i2c_msg.data[2];
			msg->data[3] = i2c_msg.data[1];
			msg->data_len = 4;
			msg->completion_code = CC_SUCCESS;
		} else {
			msg->completion_code = CC_UNSPECIFIED_ERROR;
		}
		break;
#endif
	default:
		msg->completion_code = CC_UNSPECIFIED_ERROR;
		break;
	}
	return;
}

// void OEM_1S_GET_FW_VERSION(ipmi_msg *msg)
// {
// 	CHECK_NULL_ARG(msg);

// 	if (msg->data_len != 1) {
// 		msg->completion_code = CC_INVALID_LENGTH;
// 		return;
// 	}

// 	uint8_t version[16] = { 0 }; 
// 	mctp *mctp_inst = NULL;
// 	mctp_ext_params ext_params = { 0 };
//     get_mctp_info_by_eid(0x2E, &mctp_inst, &ext_params);
//     cci_get_chip_version(mctp_inst,ext_params, version);

// 	uint8_t component;
// 	component = msg->data[0];
// #if MAX_IPMB_IDX
// 	ipmb_error status;
// 	ipmi_msg *bridge_msg;
// #endif

// 	switch (component) {
// 	case RF_COMPNT_CPLD:
// 		msg->completion_code = CC_UNSPECIFIED_ERROR;
// 		break;
// 	case RF_COMPNT_BIC:
// 		msg->data[0] = BIC_FW_YEAR_MSB;
// 		msg->data[1] = BIC_FW_YEAR_LSB;
// 		msg->data[2] = BIC_FW_WEEK;
// 		msg->data[3] = BIC_FW_VER;
// 		msg->data[4] = BIC_FW_platform_0;
// 		msg->data[5] = BIC_FW_platform_1;
// 		msg->data[6] = BIC_FW_platform_2;
// 		msg->data_len = 7;
// 		msg->completion_code = CC_SUCCESS;
// 		break;
// 	case RF_COMPNT_CXL:
// 		for(int i=0; i<16; i++){ 
// 			msg->data[i] = version[i];
// 		}
// 		msg->data_len = 16;
// 		msg->completion_code = CC_SUCCESS;

// #if MAX_IPMB_IDX
// 	case RF_COMPNT_ME:
// 		if ((IPMB_inf_index_map[ME_IPMB] == RESERVED) ||
// 		    (IPMB_config_table[IPMB_inf_index_map[ME_IPMB]].enable_status == DISABLE)) {
// 			LOG_ERR("IPMB ME interface not enabled.");
// 			msg->completion_code = CC_UNSPECIFIED_ERROR;
// 			return;
// 		}

// 		bridge_msg = (ipmi_msg *)malloc(sizeof(ipmi_msg));
// 		if (bridge_msg == NULL) {
// 			msg->completion_code = CC_OUT_OF_SPACE;
// 			return;
// 		}
// 		bridge_msg->data_len = 0;
// 		bridge_msg->seq_source = 0xff;
// 		bridge_msg->InF_source = SELF;
// 		bridge_msg->InF_target = ME_IPMB;
// 		bridge_msg->netfn = NETFN_APP_REQ;
// 		bridge_msg->cmd = CMD_APP_GET_DEVICE_ID;

// 		status = ipmb_read(bridge_msg, IPMB_inf_index_map[bridge_msg->InF_target]);
// 		if (status != IPMB_ERROR_SUCCESS) {
// 			LOG_ERR("ipmb read fail status: %x", status);
// 			SAFE_FREE(bridge_msg);
// 			msg->completion_code = CC_BRIDGE_MSG_ERR;
// 			return;
// 		} else {
// 			msg->data[0] = bridge_msg->data[2] & 0x7F;
// 			msg->data[1] = bridge_msg->data[3] >> 4;
// 			msg->data[2] = bridge_msg->data[3] & 0x0F;
// 			msg->data[3] = bridge_msg->data[12];
// 			msg->data[4] = bridge_msg->data[13] >> 4;
// 			msg->data_len = 5;
// 			msg->completion_code = CC_SUCCESS;
// 			SAFE_FREE(bridge_msg);
// 		}
// 		break;
// #endif

// 	default:
// 		msg->completion_code = CC_UNSPECIFIED_ERROR;
// 		break;
// 	}
// 	return;
// }
