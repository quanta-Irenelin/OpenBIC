#include <stdio.h>
#include <string.h>
#include "ast_adc.h"
#include "sensor.h"
#include "plat_hook.h"
#include "plat_i2c.h"
#include "plat_sensor_table.h"
#include "plat_gpio.h"

sensor_cfg plat_sensor_config[] = {
	/* number,                  type,       port,      address,      offset,
     access check arg0, arg1, cache, cache_status, mux_ADDRess, mux_offset,
     pre_sensor_read_fn, pre_sensor_read_args, post_sensor_read_fn, post_sensor_read_fn  */

	// temperature
	{ SENSOR_NUM_TEMP_TMP75, sensor_dev_tmp75, I2C_BUS3, TMP75_MB_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_CXL_CNTR, sensor_dev_tmp75, I2C_BUS2, TMP75_ASIC_ADDR, TMP75_TEMP_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, NULL },

	// ADC
	{ SENSOR_NUM_VOL_STBY5V, sensor_dev_ast_adc, ADC_PORT14, NONE, NONE, stby_access, 711, 200,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_STBY1V2, sensor_dev_ast_adc, ADC_PORT6, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_ASIC_1V8, sensor_dev_ast_adc, ADC_PORT7, NONE, NONE, stby_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_PVPP_AB, sensor_dev_ast_adc, ADC_PORT10, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_PVPP_CD, sensor_dev_ast_adc, ADC_PORT11, NONE, NONE, dc_access, 2, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_PVTT_AB, sensor_dev_ast_adc, ADC_PORT12, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },
	{ SENSOR_NUM_VOL_PVTT_CD, sensor_dev_ast_adc, ADC_PORT13, NONE, NONE, dc_access, 1, 1,
	  SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS,
	  NULL, NULL, NULL, NULL, &adc_asd_init_args[0] },

};

sensor_cfg EVT_BOM1_sensor_config_table[] = {
	// INA233
	{ SENSOR_NUM_VOL_STBY12V, sensor_dev_ina233, I2C_BUS1, INA233_12V_ADDR, SMBUS_VOL_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[0] },
	{ SENSOR_NUM_VOL_STBY3V3, sensor_dev_ina233, I2C_BUS1, INA233_3V3_ADDR, SMBUS_VOL_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[1] },
	{ SENSOR_NUM_CUR_STBY12V, sensor_dev_ina233, I2C_BUS1, INA233_12V_ADDR, SMBUS_CUR_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[0] },
	{ SENSOR_NUM_CUR_STBY3V3, sensor_dev_ina233, I2C_BUS1, INA233_3V3_ADDR, SMBUS_CUR_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[1] },
	{ SENSOR_NUM_PWR_STBY12V, sensor_dev_ina233, I2C_BUS1, INA233_12V_ADDR, SMBUS_PWR_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[0] },
	{ SENSOR_NUM_PWR_STBY3V3, sensor_dev_ina233, I2C_BUS1, INA233_3V3_ADDR, SMBUS_PWR_CMD,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_ina233_read, NULL, NULL, NULL, &ina233_init_args[1] },


	// VR temperature
	{ SENSOR_NUM_TEMP_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Voltage
	{ SENSOR_NUM_VOL_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_VOL_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_VOL_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Current
	{ SENSOR_NUM_CUR_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_CUR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_CUR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Power
	{ SENSOR_NUM_PWR_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_PWR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_PWR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
};

sensor_cfg EVT_BOM3_sensor_config_table[] = {
	// INA230
	{ SENSOR_NUM_VOL_STBY12V, sensor_dev_ina230, I2C_BUS1, INA233_12V_ADDR, INA230_BUS_VOL_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[0] },
	{ SENSOR_NUM_VOL_STBY3V3, sensor_dev_ina230, I2C_BUS1, INA233_3V3_ADDR, INA230_BUS_VOL_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[1] },
	{ SENSOR_NUM_CUR_STBY12V, sensor_dev_ina230, I2C_BUS1, INA233_12V_ADDR, INA230_CUR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[0] },
	{ SENSOR_NUM_CUR_STBY3V3, sensor_dev_ina230, I2C_BUS1, INA233_3V3_ADDR, INA230_CUR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[1] },
	{ SENSOR_NUM_PWR_STBY12V, sensor_dev_ina230, I2C_BUS1, INA233_12V_ADDR, INA230_PWR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[0] },
	{ SENSOR_NUM_PWR_STBY3V3, sensor_dev_ina230, I2C_BUS1, INA233_3V3_ADDR, INA230_PWR_OFFSET,
	  stby_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, NULL, NULL, NULL, NULL, &SQ52205_init_args[1] },

	// VR temperature
	{ SENSOR_NUM_TEMP_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_TEMP_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_TEMP_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Voltage
	{ SENSOR_NUM_VOL_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_VOL_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_VOL_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_VOL_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_VOL_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_VOL_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Current
	{ SENSOR_NUM_CUR_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_CUR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_CUR_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_CUR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_CUR_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQCD_ADDR,
	  SMBUS_CUR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },

	// VR Power
	{ SENSOR_NUM_PWR_VR0V9A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V9_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VR0V8A, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_A0V8_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[1], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VR0V8D, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_D0V8_ADDR, SMBUS_PWR_CMD,
	  vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT, ENABLE_SENSOR_POLLING, 0,
	  SENSOR_INIT_STATUS, pre_isl69254iraz_t_read, &isl69254iraz_t_pre_read_args[0], NULL, NULL,
	  NULL },
	{ SENSOR_NUM_PWR_VRVDDQAB, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_PWR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[1], NULL, NULL, NULL },
	{ SENSOR_NUM_PWR_VRVDDQCD, sensor_dev_isl69254iraz_t, I2C_BUS10, VR_VDDQAB_ADDR,
	  SMBUS_PWR_CMD, vr_access, 0, 0, SAMPLE_COUNT_DEFAULT, POLL_TIME_DEFAULT,
	  ENABLE_SENSOR_POLLING, 0, SENSOR_INIT_STATUS, pre_isl69254iraz_t_read,
	  &isl69254iraz_t_pre_read_args[0], NULL, NULL, NULL },
};

void pal_extend_sensor_config()
{
	uint8_t sensor_count = 0;
	uint8_t board_revision = 3;

	if (gpio_get(BOARD_ID0) == 1) {
		board_revision = 1;
	} else {
		board_revision = 3;
	}

	if (board_revision == 1){
		sensor_count = ARRAY_SIZE(EVT_BOM1_sensor_config_table);
		for (int index = 0; index < sensor_count; index++) {
			add_sensor_config(EVT_BOM1_sensor_config_table[index]);
		}
	}else{
		sensor_count = ARRAY_SIZE(EVT_BOM3_sensor_config_table);
		for (int index = 0; index < sensor_count; index++) {
			add_sensor_config(EVT_BOM3_sensor_config_table[index]);
		}
	}

	if (sensor_config_count != sdr_count) {
		printf("[%s] extend sensor SDR and config table not match, sdr size: 0x%x, sensor config size: 0x%x\n",
		       __func__, sdr_count, sensor_config_count);
	}
}

uint8_t pal_get_extend_sensor_config()
{
	uint8_t extend_sensor_config_size = 0;
	extend_sensor_config_size += ARRAY_SIZE(EVT_BOM3_sensor_config_table);

	return extend_sensor_config_size;
}


const int SENSOR_CONFIG_SIZE = ARRAY_SIZE(plat_sensor_config);

void load_sensor_config(void)
{
	memcpy(sensor_config, plat_sensor_config, SENSOR_CONFIG_SIZE * sizeof(sensor_cfg));
	sensor_config_count = SENSOR_CONFIG_SIZE;

	pal_extend_sensor_config();
}
