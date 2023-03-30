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
#include <stdio.h>
#include "ipmi.h"
#include "ipmb.h"
#include "libipmi.h"
#include "plat_gpio.h"
#include "plat_isr.h"
#include "expansion_board.h"
#include "plat_sensor_table.h"
#include "plat_power_seq.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(power_sequence);

static bool power_on_handler(uint8_t);
static bool power_off_handler(uint8_t);
static uint8_t cxl_power_status = CXL_POWER_OFF;

void init_cxl_power_status()
{
	if (gpio_get(ASIC_DEV_RST_N) == GPIO_HIGH)
		cxl_power_status = CXL_POWER_ON;
	else if (gpio_get(CLK_100M_OSC_EN) == GPIO_LOW)
		cxl_power_status = CXL_POWER_OFF;
	else
		LOG_ERR("Unknown CXL power status");
}

void set_MB_DC_status(uint8_t gpio_num)
{
	static bool is_MB_DC_ON = false;
	is_MB_DC_ON = (gpio_get(gpio_num) == GPIO_HIGH) ? true : false;
	LOG_INF("gpio number(%d) status(%d)", gpio_num, is_MB_DC_ON);
}

void control_power_on_sequence()
{
	if (cxl_power_status == CXL_POWER_ON)
		return;

	bool is_power_on = false;
	is_power_on = power_on_handler(ASIC_POWER_ON_STAGE);

	if (is_power_on == true) {
		gpio_set(PWRGD_CARD_PWROK, POWER_ON);
		k_usleep(100);
		control_power_stage(ENABLE_POWER_MODE, ASIC_DEV_RST_N);
		cxl_power_status = CXL_POWER_ON;
		LOG_INF("Power on success");
	} else {
		LOG_ERR("Power on fail");
	}
}

void control_power_off_sequence()
{
	if (cxl_power_status == CXL_POWER_OFF)
		return;

	bool is_power_off = false;
	// Inform Server Board expansion board power off
	gpio_set(PWRGD_CARD_PWROK, POWER_OFF);
	gpio_set(ASIC_DEV_RST_N, POWER_OFF);

	// Disable i2c synchronized during error recovery/ASIC i2c pin
	gpio_set(I2CS_SRSTB_GPIO, HIGH_INACTIVE);
	gpio_set(LSFT_SMB_DIMM_EN, HIGH_INACTIVE);

	// power_off_seq = DEFAULT_POWER_OFF_SEQ;
	is_power_off = power_off_handler(DIMM_POWER_OFF_STAGE1);

	if (is_power_off == true) {
		cxl_power_status = CXL_POWER_OFF;
		LOG_INF("Power off success");
	} else {
		LOG_ERR("Power off fail");
	}
}

void control_power_stage(uint8_t control_mode, uint8_t control_seq)
{
	switch (control_mode) {
	case ENABLE_POWER_MODE: // Control power on stage
		if (gpio_get(control_seq) != POWER_ON) {
			gpio_set(control_seq, POWER_ON);
		}
		break;
	case DISABLE_POWER_MODE: // Control power off stage
		if (gpio_get(control_seq) != POWER_OFF) {
			gpio_set(control_seq, POWER_OFF);
		}
		break;
	default:
		LOG_ERR("Not support control mode 0x%x", control_mode);
		break;
	}
}

int check_power_stage(uint8_t check_mode, uint8_t check_seq, uint8_t stage)
{
	int ret = 0;
	switch (check_mode) {
	case ENABLE_POWER_MODE: // Check power on stage
		if (gpio_get(check_seq) != POWER_ON) {
			ret = -1;
		}
		break;
	case DISABLE_POWER_MODE: // Check power off stage
		if (gpio_get(check_seq) != POWER_OFF) {
			ret = -1;
		}
		break;
	default:
		LOG_ERR("Check mode 0x%x not supported!", check_mode);
		ret = 1;
		break;
	}

	if (ret == -1) { // Addsel if check power stage fail
		bool is_addsel_success = false;
		common_addsel_msg_t sel_msg;
		memset(&sel_msg, 0, sizeof(common_addsel_msg_t));

		sel_msg.InF_target = CL_BIC_IPMB;
		sel_msg.sensor_type = IPMI_OEM_SENSOR_TYPE_OEM_C3;
		sel_msg.event_type = IPMI_EVENT_TYPE_SENSOR_SPECIFIC;
		sel_msg.sensor_number = SENSOR_NUM_POWER_ERROR;
		sel_msg.event_data1 =
			((check_mode == ENABLE_POWER_MODE) ? IPMI_OEM_EVENT_OFFSET_EXP_PWRON_FAIL :
							     IPMI_OEM_EVENT_OFFSET_EXP_PWROFF_FAIL);
		sel_msg.event_data2 = stage;
		sel_msg.event_data3 = get_board_id();

		is_addsel_success = common_add_sel_evt_record(&sel_msg);
		if (is_addsel_success == false) {
			LOG_ERR("Control power event addsel fail  mode: 0x%x  seq: 0x%x",
				check_mode, check_seq);
		}
	}

	return ret;
}

static bool power_on_handler(uint8_t initial_stage)
{
	int check_power_ret = -1;
	bool enable_power_on_handler = true;
	uint8_t control_stage = initial_stage;
	while (enable_power_on_handler == true) {
		switch (control_stage) { // Enable VR power machine
		case ASIC_POWER_ON_STAGE:
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_01);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_02);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_03);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_04);
			break;
		case DIMM_POWER_ON_STAGE1:
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_05);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_06);
			break;
		case DIMM_POWER_ON_STAGE2:
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_07);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_08);
			break;
		case DIMM_POWER_ON_STAGE3:
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_09);
			control_power_stage(ENABLE_POWER_MODE, CONTROL_POWER_SEQ_10);
			break;
		case BOARD_POWER_ON_STAGE:
			control_power_stage(ENABLE_POWER_MODE, CLK_100M_OSC_EN);
			break;
		default:
			LOG_ERR("Stage 0x%x not supported", initial_stage);
			enable_power_on_handler = false;
			break;
		}
		k_msleep(CHKPWR_DELAY_MSEC);

		switch (control_stage) { // Check VR power machine
		case ASIC_POWER_ON_STAGE:
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_01,
					      ASIC_POWER_ON_STAGE) != 0) {
				LOG_ERR("P0V8_ASICA_PWRGD is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_02,
					      ASIC_POWER_ON_STAGE) != 0) {
				LOG_ERR("P0V8_ASICD_PWRGD is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_03,
					      ASIC_POWER_ON_STAGE) != 0) {
				LOG_ERR("P0V9_ASICA_PWRGD is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_04,
					      ASIC_POWER_ON_STAGE) != 0) {
				LOG_ERR("P1V8_ASIC_PG_R is not enabled!");
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = DIMM_POWER_ON_STAGE1;
			break;
		case DIMM_POWER_ON_STAGE1:
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_05,
					      DIMM_POWER_ON_STAGE1) != 0) {
				LOG_ERR("PVPP_AB_PG_R is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_06,
					      DIMM_POWER_ON_STAGE1) != 0) {
				LOG_ERR("PVPP_CD_PG_R is not enabled!");
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = DIMM_POWER_ON_STAGE2;
			break;
		case DIMM_POWER_ON_STAGE2:
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_07,
					      DIMM_POWER_ON_STAGE2) != 0) {
				LOG_ERR("PWRGD_PVDDQ_AB is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_08,
					      DIMM_POWER_ON_STAGE2) != 0) {
				LOG_ERR("PWRGD_PVDDQ_CD is not enabled!");
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = DIMM_POWER_ON_STAGE3;
			break;
		case DIMM_POWER_ON_STAGE3:
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_09,
					      DIMM_POWER_ON_STAGE3) != 0) {
				LOG_ERR("PVTT_AB_PG_R is not enabled!");
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(ENABLE_POWER_MODE, CHECK_POWER_SEQ_10,
					      DIMM_POWER_ON_STAGE3) != 0) {
				LOG_ERR("PVTT_CD_PG_R is not enabled!");
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = BOARD_POWER_ON_STAGE;
			break;
		case BOARD_POWER_ON_STAGE:

			// Enable i2c synchronized during error recovery/ASIC i2c pin
			gpio_set(I2CS_SRSTB_GPIO, HIGH_ACTIVE);
			gpio_set(LSFT_SMB_DIMM_EN, HIGH_ACTIVE);

			check_power_ret = 0;
			enable_power_on_handler = false;
			break;
		default:
			LOG_ERR("Not support stage 0x%x", initial_stage);
			enable_power_on_handler = false;
			break;
		}

		if (check_power_ret != 0) {
			if (control_stage == ASIC_POWER_ON_STAGE) {
				power_off_handler(ASIC_POWER_OFF_STAGE1);
			} else {
				power_off_handler(DIMM_POWER_OFF_STAGE1);
			}
			enable_power_on_handler = false;
		}
	}
	if (check_power_ret == 0) {
		return true;
	} else {
		return false;
	}
}

static bool power_off_handler(uint8_t initial_stage)
{
	bool enable_power_off_handler = true;
	int check_power_ret = -1;
	uint8_t control_stage = initial_stage;
	while (enable_power_off_handler == true) {
		switch (control_stage) { // Disable VR power machine
		case DIMM_POWER_OFF_STAGE1:
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_09);
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_10);
			break;
		case DIMM_POWER_OFF_STAGE2:
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_07);
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_08);
			break;
		case DIMM_POWER_OFF_STAGE3:
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_05);
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_06);
			break;
		case ASIC_POWER_OFF_STAGE1:
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_04);
			break;
		case ASIC_POWER_OFF_STAGE2:
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_01);
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_02);
			control_power_stage(DISABLE_POWER_MODE, CONTROL_POWER_SEQ_03);
			break;
		case BOARD_POWER_OFF_STAGE:
			control_power_stage(DISABLE_POWER_MODE, CLK_100M_OSC_EN);
			break;
		default:
			LOG_ERR("Stage 0x%x not supported", initial_stage);
			enable_power_off_handler = false;
			break;
		}
		k_msleep(CHKPWR_DELAY_MSEC);

		switch (control_stage) { // Check VR power machine
		case DIMM_POWER_OFF_STAGE1:
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_10,
					      DIMM_POWER_OFF_STAGE1) != 0) {
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_09,
					      DIMM_POWER_OFF_STAGE1) != 0) {
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = DIMM_POWER_OFF_STAGE2;
			break;
		case DIMM_POWER_OFF_STAGE2:
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_08,
					      DIMM_POWER_OFF_STAGE2) != 0) {
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_07,
					      DIMM_POWER_OFF_STAGE2) != 0) {
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = DIMM_POWER_OFF_STAGE3;
			break;
		case DIMM_POWER_OFF_STAGE3:
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_06,
					      DIMM_POWER_OFF_STAGE3) != 0) {
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_05,
					      DIMM_POWER_OFF_STAGE3) != 0) {
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = ASIC_POWER_OFF_STAGE1;
			break;
		case ASIC_POWER_OFF_STAGE1:
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_04,
					      ASIC_POWER_OFF_STAGE1) != 0) {
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = ASIC_POWER_OFF_STAGE2;
			break;
		case ASIC_POWER_OFF_STAGE2:
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_03,
					      ASIC_POWER_OFF_STAGE2) != 0) {
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_02,
					      ASIC_POWER_OFF_STAGE2) != 0) {
				check_power_ret = -1;
				break;
			}
			if (check_power_stage(DISABLE_POWER_MODE, CHECK_POWER_SEQ_01,
					      ASIC_POWER_OFF_STAGE2) != 0) {
				check_power_ret = -1;
				break;
			}
			check_power_ret = 0;
			control_stage = BOARD_POWER_OFF_STAGE;
			break;
		case BOARD_POWER_OFF_STAGE:
			enable_power_off_handler = false;
			break;
		default:
			LOG_ERR("Stage 0x%x not supported", initial_stage);
			enable_power_off_handler = false;
			break;
		}

		if (check_power_ret != 0) {
			enable_power_off_handler = false;
		}
	}
	if (check_power_ret == 0) {
		return true;
	} else {
		return false;
	}
}
