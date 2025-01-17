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

#include "plat_sensor_table.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <logging/log.h>
#include "libutil.h"
#include "ast_adc.h"
#include "sensor.h"
#include "hal_gpio.h"
#include "plat_i2c.h"
#include "plat_gpio.h"
#include "plat_hook.h"
#include "plat_class.h"
#include "pmbus.h"
#include "sq52205.h"
#include "power_status.h"
#include "emc1412.h"
#include "ltc2991.h"
#include "i2c-mux-pca984x.h"
#include "plat_ipmi.h"
#include "plat_dev.h"
#include "cci.h"
#include "plat_mctp.h"
#include "plat_class.h"

LOG_MODULE_REGISTER(plat_sensor_table);

#define E1S_13_14_MUX_BUS I2C_BUS4
#define E1S_0_SENSOR_CFG_INDEX 0
#define E1S_1_SENSOR_CFG_INDEX 1
#define CARD_13_E1S_0_MUX_CFG_INDEX 4
#define CARD_13_E1S_1_MUX_CFG_INDEX 5
#define CARD_14_E1S_0_MUX_CFG_INDEX 6
#define CARD_14_E1S_1_MUX_CFG_INDEX 7
#define PCIE_CARD_INIT_CFG_OFFSET_0 0
#define PCIE_CARD_INIT_CFG_OFFSET_1 1
#define CXL_INA233_INIT_OFFSET 2
#define CXL_LTC2991_INIT_OFFSET 1

struct k_mutex i2c_2_pca9548a_mutex;
struct k_mutex i2c_3_pca9546a_mutex;
struct k_mutex i2c_4_pca9548a_mutex;
struct k_mutex i2c_8_pca9548a_mutex;

sensor_cfg plat_sensor_config[] = {
	/* number,                  type,       port,      address,      offset,
	   access check arg0, arg1, sample_count, cache, cache_status, mux_ADDRess, mux_offset,
	   pre_sensor_read_fn, pre_sensor_read_args, post_sensor_read_fn, post_sensor_read_fn  */

	/** Temperature **/
	{ SENSOR_NUM_TEMP_TMP75_IN, sensor_dev_tmp75, I2C_BUS1, TMP75_IN_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },

	/** NVME **/
	{ SENSOR_NUM_TEMP_E1S_0, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, is_e1s_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_nvme_read, &bus_4_pca9548_configs[0], post_nvme_read, NULL, NULL },
	{ SENSOR_NUM_TEMP_E1S_1, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, is_e1s_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_nvme_read, &bus_4_pca9548_configs[1], post_nvme_read, NULL, NULL },
	{ SENSOR_NUM_TEMP_E1S_2, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, is_e1s_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_nvme_read, &bus_4_pca9548_configs[2], post_nvme_read, NULL, NULL },
	{ SENSOR_NUM_TEMP_E1S_3, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, is_e1s_access, 0,
	  0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  pre_nvme_read, &bus_4_pca9548_configs[3], post_nvme_read, NULL, NULL },

	/** ADC **/
	{ SENSOR_NUM_VOL_P3V3_AUX, sensor_dev_ast_adc, ADC_PORT0, NONE, NONE, stby_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_P1V2_AUX, sensor_dev_ast_adc, ADC_PORT1, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_P3V3, sensor_dev_ast_adc, ADC_PORT2, NONE, NONE, stby_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
};

sensor_cfg plat_mc_sq52205_sensor_config[] = {
	/** SQ52205_01 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD01, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &sq52205_init_args[0] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD01, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &sq52205_init_args[0] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD01, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &sq52205_init_args[0] },

	/** SQ52205_02 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD02, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &sq52205_init_args[1] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD02, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &sq52205_init_args[1] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD02, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &sq52205_init_args[1] },

	/** SQ52205_03 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD03, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &sq52205_init_args[2] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD03, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &sq52205_init_args[2] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD03, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &sq52205_init_args[2] },

	/** SQ52205_04 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD04, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &sq52205_init_args[3] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD04, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &sq52205_init_args[3] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD04, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &sq52205_init_args[3] },

	/** SQ52205_05 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD05, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &sq52205_init_args[4] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD05, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &sq52205_init_args[4] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD05, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &sq52205_init_args[4] },

	/** SQ52205_06 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD06, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &sq52205_init_args[5] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD06, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &sq52205_init_args[5] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD06, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &sq52205_init_args[5] },

	/** SQ52205_07 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD07, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &sq52205_init_args[6] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD07, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &sq52205_init_args[6] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD07, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &sq52205_init_args[6] },

	/** SQ52205_08 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD08, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &sq52205_init_args[7] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD08, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &sq52205_init_args[7] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD08, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &sq52205_init_args[7] },

	/** SQ52205_09 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD09, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &sq52205_init_args[8] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD09, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &sq52205_init_args[8] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD09, sensor_dev_sq52205, I2C_BUS3, SQ52205_1_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &sq52205_init_args[8] },

	/** SQ52205_10 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD10, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &sq52205_init_args[9] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD10, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &sq52205_init_args[9] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD10, sensor_dev_sq52205, I2C_BUS3, SQ52205_2_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &sq52205_init_args[9] },

	/** SQ52205_11 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD11, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &sq52205_init_args[10] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD11, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &sq52205_init_args[10] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD11, sensor_dev_sq52205, I2C_BUS3, SQ52205_3_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &sq52205_init_args[10] },

	/** SQ52205_12 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD12, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_VOL_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &sq52205_init_args[11] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD12, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_CUR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &sq52205_init_args[11] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD12, sensor_dev_sq52205, I2C_BUS3, SQ52205_4_ADDR,
	  SQ52205_READ_PWR_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &sq52205_init_args[11] },
};

sensor_cfg plat_mc_ina233_sensor_config[] = {
	/** INA233_01 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD01, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &mc_ina233_init_args[0] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD01, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &mc_ina233_init_args[0] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD01, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[0], post_sq52205_read, NULL, &mc_ina233_init_args[0] },

	/** INA233_02 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD02, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &mc_ina233_init_args[1] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD02, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &mc_ina233_init_args[1] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD02, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[1], post_sq52205_read, NULL, &mc_ina233_init_args[1] },

	/** INA233_03 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD03, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &mc_ina233_init_args[2] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD03, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &mc_ina233_init_args[2] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD03, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[2], post_sq52205_read, NULL, &mc_ina233_init_args[2] },

	/** INA233_04 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD04, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &mc_ina233_init_args[3] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD04, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &mc_ina233_init_args[3] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD04, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[3], post_sq52205_read, NULL, &mc_ina233_init_args[3] },

	/** INA233_05 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD05, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &mc_ina233_init_args[4] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD05, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &mc_ina233_init_args[4] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD05, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[4], post_sq52205_read, NULL, &mc_ina233_init_args[4] },

	/** INA233_06 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD06, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &mc_ina233_init_args[5] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD06, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &mc_ina233_init_args[5] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD06, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[5], post_sq52205_read, NULL, &mc_ina233_init_args[5] },

	/** INA233_07 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD07, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &mc_ina233_init_args[6] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD07, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &mc_ina233_init_args[6] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD07, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[6], post_sq52205_read, NULL, &mc_ina233_init_args[6] },

	/** INA233_08 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD08, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &mc_ina233_init_args[7] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD08, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &mc_ina233_init_args[7] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD08, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[7], post_sq52205_read, NULL, &mc_ina233_init_args[7] },

	/** INA233_09 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD09, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &mc_ina233_init_args[8] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD09, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &mc_ina233_init_args[8] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD09, sensor_dev_ina233, I2C_BUS3, SQ52205_1_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[8], post_sq52205_read, NULL, &mc_ina233_init_args[8] },

	/** INA233_10 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD10, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &mc_ina233_init_args[9] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD10, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &mc_ina233_init_args[9] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD10, sensor_dev_ina233, I2C_BUS3, SQ52205_2_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[9], post_sq52205_read, NULL, &mc_ina233_init_args[9] },

	/** INA233_11 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD11, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &mc_ina233_init_args[10] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD11, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &mc_ina233_init_args[10] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD11, sensor_dev_ina233, I2C_BUS3, SQ52205_3_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[10], post_sq52205_read, NULL, &mc_ina233_init_args[10] },

	/** INA233_12 **/
	{ SENSOR_NUM_VOL_P12V_AUX_CARD12, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &mc_ina233_init_args[11] },
	{ SENSOR_NUM_CUR_P12V_AUX_CARD12, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &mc_ina233_init_args[11] },
	{ SENSOR_NUM_PWR_P12V_AUX_CARD12, sensor_dev_ina233, I2C_BUS3, SQ52205_4_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_sq52205_read,
	  &pwr_monitor_pre_proc_args[11], post_sq52205_read, NULL, &mc_ina233_init_args[11] },
};

sensor_cfg plat_hsc_mp5990_sensor_config[] = {
	/** HSC **/
	{ SENSOR_NUM_TEMP_PU4, sensor_dev_mp5990, I2C_BUS6, MPS_MP5990_ADDR,
	  PMBUS_READ_TEMPERATURE_1, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL,
	  &mp5990_init_args[0] },
	{ SENSOR_NUM_VOL_P12V_AUX, sensor_dev_mp5990, I2C_BUS6, MPS_MP5990_ADDR, PMBUS_READ_VOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
	{ SENSOR_NUM_CUR_P12V_AUX, sensor_dev_mp5990, I2C_BUS6, MPS_MP5990_ADDR, PMBUS_READ_IOUT,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
	{ SENSOR_NUM_PWR_P12V_AUX, sensor_dev_mp5990, I2C_BUS6, MPS_MP5990_ADDR, PMBUS_READ_PIN,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &mp5990_init_args[0] },
};

sensor_cfg plat_e1s_1_12_sensor_config[] = {
	{ SENSOR_NUM_TEMP_JCN_E1S_0, sensor_dev_nvme, I2C_BUS8, E1S_ADDR, E1S_OFFSET, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_JCN_E1S_1, sensor_dev_nvme, I2C_BUS2, E1S_ADDR, E1S_OFFSET, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
};

sensor_cfg plat_e1s_13_14_sensor_config[] = {
	{ SENSOR_NUM_TEMP_JCN_E1S_0, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_JCN_E1S_1, sensor_dev_nvme, I2C_BUS4, E1S_ADDR, E1S_OFFSET, stby_access,
	  0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
};

sensor_cfg plat_cxl_sensor_config[] = {
	/** Temperature **/
	{ SENSOR_NUM_TEMP_CXL_TMP75_IN, sensor_dev_tmp75, I2C_BUS2, CXL_TMP75_IN_ADDR,
	  TMP75_TEMP_OFFSET, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[2] },
	{ SENSOR_NUM_TEMP_CXL_CTRL, sensor_dev_emc1412, I2C_BUS2, CXL_CTRL_ADDR, EMC1412_READ_TEMP,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL, &cxl_mux_configs[0] },
	{ SENSOR_NUM_TEMP_CXL, sensor_dev_pm8702, MCTP_EID_CXL, NONE, CHIP_TEMP_OFFSET,
	  is_cxl_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL, &cxl_mux_configs[0] },
	{ SENSOR_NUM_TEMP_CXL_DIMMA, sensor_dev_pm8702, MCTP_EID_CXL, CXL_DIMMA_TEMP_ADDR,
	  DIMM_TEMP_OFFSET, is_cxl_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[0] },
	{ SENSOR_NUM_TEMP_CXL_DIMMB, sensor_dev_pm8702, MCTP_EID_CXL, CXL_DIMMB_TEMP_ADDR,
	  DIMM_TEMP_OFFSET, is_cxl_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[0] },
	{ SENSOR_NUM_TEMP_CXL_DIMMC, sensor_dev_pm8702, MCTP_EID_CXL, CXL_DIMMC_TEMP_ADDR,
	  DIMM_TEMP_OFFSET, is_cxl_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[0] },
	{ SENSOR_NUM_TEMP_CXL_DIMMD, sensor_dev_pm8702, MCTP_EID_CXL, CXL_DIMMD_TEMP_ADDR,
	  DIMM_TEMP_OFFSET, is_cxl_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[0] },

	/** INA233 **/
	{ SENSOR_NUM_VOL_P12V_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U6_INA233_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },
	{ SENSOR_NUM_VOL_P3V3_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U7_INA233_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },
	{ SENSOR_NUM_CUR_P12V_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U6_INA233_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },
	{ SENSOR_NUM_CUR_P3V3_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U7_INA233_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },
	{ SENSOR_NUM_PWR_P12V_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U6_INA233_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },
	{ SENSOR_NUM_PWR_P3V3_STBY_4CP, sensor_dev_ina233, I2C_BUS2, CXL_U7_INA233_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[1] },

	/** Voltage monitor **/
	{ SENSOR_NUM_VOL_P5V_STBY, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V3_VOLTAGE, stby_access, 711, 200, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_P1V8_ASIC, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V5_VOLTAGE, stby_access, 1, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_P12V_STBY, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V1_VOLTAGE, stby_access, 66, 10, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_P3V3_STBY, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V2_VOLTAGE, stby_access, 2, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_PVPP_AB, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V4_VOLTAGE, stby_access, 2, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_PVTT_AB, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V7_VOLTAGE, stby_access, 1, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_PVPP_CD, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V6_VOLTAGE, stby_access, 2, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },
	{ SENSOR_NUM_VOL_PVTT_CD, sensor_dev_ltc2991, I2C_BUS2, CXL_U8_LTC2991_ADDR,
	  LTC2991_READ_V8_VOLTAGE, stby_access, 1, 1, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL,
	  &cxl_mux_configs[5] },

	/** VR Voltage **/
	{ SENSOR_NUM_VOL_P0V8_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V8_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_VOL_P0V9_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V9_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_VOL_P0V8_ASICD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_D0V8_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_VOL_PVDDQ_AB, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQAB_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_VOL_PVDDQ_CD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQCD_ADDR,
	  PMBUS_READ_VOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },

	/** VR Current **/
	{ SENSOR_NUM_CUR_P0V8_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V8_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_CUR_P0V9_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V9_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_CUR_P0V8_ASICD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_D0V8_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_CUR_PVDDQ_AB, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQAB_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_CUR_PVDDQ_CD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQCD_ADDR,
	  PMBUS_READ_IOUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },

	/** VR Power **/
	{ SENSOR_NUM_PWR_P0V8_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V8_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_PWR_P0V9_ASICA, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_A0V9_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_PWR_P0V8_ASICD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_D0V8_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_PWR_PVDDQ_AB, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQAB_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[1],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
	{ SENSOR_NUM_PWR_PVDDQ_CD, sensor_dev_xdpe12284c, I2C_BUS2, CXL_VR_VDDQCD_ADDR,
	  PMBUS_READ_POUT, stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_cxl_vr_read, &vr_page_select[0],
	  post_cxl_xdpe12284c_read, NULL, NULL, &cxl_mux_configs[3] },
};

sensor_cfg evt2_extend_sensor_config[] = {
	{ SENSOR_NUM_TEMP_TMP75_OUT, sensor_dev_tmp75, I2C_BUS1, TMP75_OUT_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
};

const int SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_sensor_config);
const int MC_SQ52205_SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_mc_sq52205_sensor_config);
const int MC_INA233_SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_mc_ina233_sensor_config);
const int E1S_SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_e1s_1_12_sensor_config);
const int CXL_SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_cxl_sensor_config);
const int HSC_SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_hsc_mp5990_sensor_config);
const int EVT2_EXTEND_SENSOR_CONFIG_SIZE = ARRAY_SIZE(evt2_extend_sensor_config);

void pal_extend_sensor_config()
{
	uint8_t board_revision = get_board_revision();

	switch (board_revision) {
	case REV_EVT1:
		LOG_ERR("add mp5990 config");
		for (int index = 0; index < HSC_SENSOR_CONFIG_SIZE; index++) {
			add_sensor_config(plat_hsc_mp5990_sensor_config[index]);
		}
		break;
	case REV_EVT2:
	case REV_DVT:
	case REV_PVT:
	case REV_MP:
		for (int index = 0; index < EVT2_EXTEND_SENSOR_CONFIG_SIZE; index++) {
			add_sensor_config(evt2_extend_sensor_config[index]);
		}
		break;
	default:
		LOG_ERR("Unknown board revision %x", board_revision);
		break;
	}

	if (gpio_get(BOARD_ID1) == GPIO_LOW) {
		LOG_ERR("add sq52205 config");
		for (int index = 0; index < MC_SQ52205_SENSOR_CONFIG_SIZE; index++) {
			add_sensor_config(plat_mc_sq52205_sensor_config[index]);
		}
	} else {
		LOG_ERR("add ina233 config");
		for (int index = 0; index < MC_INA233_SENSOR_CONFIG_SIZE; index++) {
			add_sensor_config(plat_mc_ina233_sensor_config[index]);
		}
	}
}

void load_sensor_config(void)
{
	memcpy(sensor_config, plat_sensor_config, sizeof(plat_sensor_config));
	sensor_config_count = ARRAY_SIZE(plat_sensor_config);

	// Fix config table in different system/config
	pal_extend_sensor_config();
}

uint8_t pal_get_extend_sensor_config()
{
	uint8_t extend_sensor_config_size = 0;

	uint8_t board_revision = get_board_revision();

	switch (board_revision) {
	case REV_EVT1:
		extend_sensor_config_size += HSC_SENSOR_CONFIG_SIZE;
		break;
	case REV_EVT2:
	case REV_DVT:
	case REV_PVT:
	case REV_MP:
		extend_sensor_config_size += EVT2_EXTEND_SENSOR_CONFIG_SIZE;
		break;
	default:
		LOG_ERR("Unknown board revision %x", board_revision);
		break;
	}

	extend_sensor_config_size += MC_SQ52205_SENSOR_CONFIG_SIZE;

	return extend_sensor_config_size;
}

bool is_dc_access(uint8_t sensor_num)
{
	return get_DC_status();
}

bool is_e1s_access(uint8_t sensor_num)
{
	int ret = false;
	uint8_t card_id = 0;
	uint8_t card_type = 0;

	switch (sensor_num) {
	case SENSOR_NUM_TEMP_E1S_0:
		card_id = CARD_5_INDEX;
		break;
	case SENSOR_NUM_TEMP_E1S_1:
		card_id = CARD_6_INDEX;
		break;
	case SENSOR_NUM_TEMP_E1S_2:
		card_id = CARD_7_INDEX;
		break;
	case SENSOR_NUM_TEMP_E1S_3:
		card_id = CARD_8_INDEX;
		break;
	default:
		return false;
	}

	ret = get_pcie_card_type(card_id, &card_type);
	if (ret < 0) {
		return false;
	}

	if (card_type == CARD_NOT_PRESENT) {
		return false;
	}

	return true;
}

bool is_cxl_access(uint8_t pcie_card_id)
{
	int ret = 0;
	bool is_set_eid = false;
	uint8_t cxl_id = 0;

	ret = pcie_card_id_to_cxl_id(pcie_card_id, &cxl_id);
	if (ret < 0) {
		return false;
	}

	is_set_eid = get_cxl_eid_flag(cxl_id);

	return is_set_eid;
}

struct k_mutex *get_i2c_mux_mutex(uint8_t i2c_bus)
{
	struct k_mutex *mutex = NULL;

	switch (i2c_bus) {
	case I2C_BUS2:
		mutex = &i2c_2_pca9548a_mutex;
		break;
	case I2C_BUS3:
		mutex = &i2c_3_pca9546a_mutex;
		break;
	case I2C_BUS4:
		mutex = &i2c_4_pca9548a_mutex;
		break;
	case I2C_BUS8:
		mutex = &i2c_8_pca9548a_mutex;
		break;
	default:
		LOG_ERR("No support for i2c bus %d mutex", i2c_bus);
		break;
	}

	return mutex;
}

bool get_cxl_sensor_config_index(uint8_t sensor_num, uint8_t *index)
{
	CHECK_NULL_ARG_WITH_RETURN(index, false);

	uint8_t i = 0;
	for (i = 0; i < CXL_SENSOR_CONFIG_SIZE; ++i) {
		if (sensor_num == plat_cxl_sensor_config[i].num) {
			*index = i;
			return true;
		}
	}
	return false;
}

bool get_pcie_card_mux_config(uint8_t card_id, uint8_t sensor_num, mux_config *card_mux_cfg,
			      mux_config *cxl_mux_cfg)
{
	CHECK_NULL_ARG_WITH_RETURN(card_mux_cfg, false);
	CHECK_NULL_ARG_WITH_RETURN(cxl_mux_cfg, false);

	int ret = -1;
	uint8_t cxl_id = 0;
	uint8_t sensor_config_index = 0;
	mux_config *tmp_mux_config = { 0 };

	ret = pcie_card_id_to_cxl_id(card_id, &cxl_id);
	if (ret != 0) {
		return false;
	}

	if (get_cxl_sensor_config_index(sensor_num, &sensor_config_index) != true) {
		LOG_ERR("Invalid sensor number: 0x%x to get sensor config", sensor_num);
		return false;
	}

	*card_mux_cfg = bus_2_pca9548_configs[cxl_id];
	card_mux_cfg->bus = MEB_CXL_BUS;

	tmp_mux_config = (mux_config *)plat_cxl_sensor_config[sensor_config_index].priv_data;
	CHECK_NULL_ARG_WITH_RETURN(tmp_mux_config, false);

	*cxl_mux_cfg = *tmp_mux_config;
	cxl_mux_cfg->bus = MEB_CXL_BUS;

	return true;
}

void pal_init_drive(sensor_cfg *cfg_table, uint8_t cfg_size, uint8_t device_type, uint8_t card_id)
{
	bool ret = false;
	uint8_t index = 0;
	uint8_t sensor_num = 0;
	uint8_t init_status = 0;
	int skip_init_sensor_num = -1;
	sensor_cfg *cfg = NULL;
	bool (*pre_switch_mux_func)(uint8_t, uint8_t) = NULL;
	bool (*post_switch_mux_func)(uint8_t, uint8_t) = NULL;

	switch (device_type) {
	case CXL_CARD:
		pre_switch_mux_func = pre_cxl_switch_mux;
		post_switch_mux_func = post_cxl_switch_mux;
		break;
	default:
		LOG_ERR("Invalid device type: 0x%x", device_type);
		return;
	}

	for (index = 0; index < cfg_size; index++) {
		cfg = &cfg_table[index];
		sensor_num = cfg->num;

		if (sensor_num == skip_init_sensor_num) {
			continue;
		}

		ret = pre_switch_mux_func(sensor_num, card_id);
		if (ret != true) {
			LOG_ERR("Pre switch mux fail, sensor num: 0x%x, card id: 0x%x", sensor_num,
				card_id);
			continue;
		}

		ret = pal_sensor_drive_init(card_id, cfg, &init_status);
		if (ret == true) {
			if (init_status != SENSOR_INIT_SUCCESS) {
				LOG_ERR("Initial sensor drive fail, sensor num: 0x%x, card id: 0x%x",
					sensor_num, card_id);
			}
		} else {
			LOG_ERR("Map initial sensor function fail, sensor num: 0x%x, card id: 0x%x",
				sensor_num, card_id);
		}

		ret = post_switch_mux_func(sensor_num, card_id);
		if (ret != true) {
			LOG_ERR("Post switch mux fail, sensor num: 0x%x, card id: 0x%x", sensor_num,
				card_id);
		}
	}
}

void *get_pcie_init_sensor_config(uint8_t card_id, uint8_t sensor_number)
{
	int ret = -1;
	uint8_t cxl_id = 0;

	ret = pcie_card_id_to_cxl_id(card_id, &cxl_id);
	if (ret != 0) {
		LOG_ERR("Invalid card id: 0x%x", card_id);
		return NULL;
	}

	uint8_t index = 0;
	uint8_t offset = 0;

	if (get_cxl_sensor_config_index(sensor_number, &index) != true) {
		LOG_ERR("Fail to find initial config, card: 0x%x, sensor num: 0x%x", card_id,
			sensor_number);
		return NULL;
	}

	sensor_cfg cfg = plat_cxl_sensor_config[index];
	switch (cfg.target_addr) {
	case CXL_U6_INA233_ADDR:
		offset = (CXL_INA233_INIT_OFFSET * cxl_id) + PCIE_CARD_INIT_CFG_OFFSET_0;
		return &ina233_init_args[offset];
	case CXL_U7_INA233_ADDR:
		offset = (CXL_INA233_INIT_OFFSET * cxl_id) + PCIE_CARD_INIT_CFG_OFFSET_1;
		return &ina233_init_args[offset];
	case CXL_U8_LTC2991_ADDR:
		offset = (CXL_LTC2991_INIT_OFFSET * cxl_id) + PCIE_CARD_INIT_CFG_OFFSET_0;
		return &ltc2991_init_args[offset];
	default:
		LOG_ERR("Invalid cxl address: 0x%x", cfg.target_addr);
		return NULL;
	}
}
