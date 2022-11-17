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
#include <string.h>
#include "cmsis_os.h"
#include "hal_gpio.h"
#include "plat_gpio.h"
#include "plat_isr.h"
#include "plat_power_seq.h"

#define gpio_name_to_num(x) #x,
const char *const gpio_name[] = {
	name_gpioA name_gpioB name_gpioC name_gpioD name_gpioE name_gpioF name_gpioG name_gpioH
		name_gpioI name_gpioJ name_gpioK name_gpioL name_gpioM name_gpioN name_gpioO
			name_gpioP name_gpioQ name_gpioR name_gpioS name_gpioT name_gpioU
};
#undef gpio_name_to_num

void gpio_int_default()
{
	return;
}

GPIO_CFG plat_gpio_cfg[] = {
	//  chip,      number,   is_init, is_latch, direction,    status,     property,    int_type,              int_cb
	// GPIO A group
	{ CHIP_GPIO, 0, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_12v_fault_handler_dev3 },
	{ CHIP_GPIO, 1, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_12v_fault_handler_dev2 },
	{ CHIP_GPIO, 2, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_12v_fault_handler_dev1 },
	{ CHIP_GPIO, 3, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_12v_fault_handler_dev0 },
	{ CHIP_GPIO, 4, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 5, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 6, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 7, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	// GPIO B group
	{ CHIP_GPIO, 8, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  pwrgd_p12v_aux_int_handler },
	{ CHIP_GPIO, 9, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 10, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 11, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, OPEN_DRAIN, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 12, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 13, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  aux_pwr_en_int_handler },
	{ CHIP_GPIO, 14, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 15, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO C group
	{ CHIP_GPIO, 16, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 17, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 18, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 19, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 20, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 21, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 22, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 23, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	// GPIO D group
	{ CHIP_GPIO, 24, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 25, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 26, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 27, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 28, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 29, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_pwrgd_handler_dev0 },
	{ CHIP_GPIO, 30, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 31, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	// GPIO E group
	{ CHIP_GPIO, 32, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  power_en_int_handler },
	{ CHIP_GPIO, 33, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 34, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  rst_mb_n_int_handler },
	{ CHIP_GPIO, 35, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 36, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 37, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 38, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 39, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO F group
	{ CHIP_GPIO, 40, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 41, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 42, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 43, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 44, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 45, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 46, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  p12v_edge_fault_sel },
	{ CHIP_GPIO, 47, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO G group
	{ CHIP_GPIO, 48, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 49, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 50, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 51, ENABLE, DISABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 52, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 53, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  prsnt_int_handler_dev3 },
	{ CHIP_GPIO, 54, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  prsnt_int_handler_dev2 },
	{ CHIP_GPIO, 55, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  prsnt_int_handler_dev1 },
	// GPIO H group
	{ CHIP_GPIO, 56, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  prsnt_int_handler_dev0 },
	{ CHIP_GPIO, 57, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 58, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 59, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 60, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 61, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 62, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 63, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO I group
	{ CHIP_GPIO, 64, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 65, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 66, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 67, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 68, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 69, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 70, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 71, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO J group
	{ CHIP_GPIO, 72, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 73, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 74, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 75, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 76, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 77, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 78, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 79, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO K group
	{ CHIP_GPIO, 80, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 81, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 82, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 83, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 84, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 85, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 86, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 87, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO L group
	{ CHIP_GPIO, 88, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 89, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 90, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 91, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 92, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 93, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 94, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	{ CHIP_GPIO, 95, ENABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE, NULL },
	// GPIO M group
	{ CHIP_GPIO, 96, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 97, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 98, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 99, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 100, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 101, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 102, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 103, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO N group
	{ CHIP_GPIO, 104, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 105, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 106, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 107, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 108, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_pwrgd_handler_dev2 },
	{ CHIP_GPIO, 109, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_pwrgd_handler_dev1 },
	{ CHIP_GPIO, 110, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  ina231_alert_handler_m2_dev0 },
	{ CHIP_GPIO, 111, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO O group
	{ CHIP_GPIO, 112, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  dev_pwrgd_handler_dev3 },
	{ CHIP_GPIO, 113, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  ina231_alert_handler_m2_dev3 },
	{ CHIP_GPIO, 114, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  ina231_alert_handler_m2_dev2 },
	{ CHIP_GPIO, 115, ENABLE, DISABLE, GPIO_INPUT, GPIO_HIGH, PUSH_PULL, GPIO_INT_EDGE_BOTH,
	  ina231_alert_handler_m2_dev1 },
	{ CHIP_GPIO, 116, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 117, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 118, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 119, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO P group
	{ CHIP_GPIO, 120, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 121, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 122, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 123, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 124, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 125, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 126, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_HIGH, OPEN_DRAIN, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 127, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_HIGH, OPEN_DRAIN, GPIO_INT_DISABLE,
	  NULL },
	// GPIO Q group
	{ CHIP_GPIO, 128, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_HIGH, OPEN_DRAIN, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 129, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_HIGH, OPEN_DRAIN, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 130, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 131, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 132, ENABLE, ENABLE, GPIO_OUTPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 133, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 134, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 135, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO R group
	{ CHIP_GPIO, 136, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 137, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 138, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 139, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 140, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 141, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 142, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 143, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO S group
	{ CHIP_GPIO, 144, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 145, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 146, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 147, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 148, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 149, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 150, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 151, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO T group
	{ CHIP_GPIO, 152, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 153, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 154, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 155, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 156, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 157, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 158, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 159, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	// GPIO U group
	{ CHIP_GPIO, 160, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 161, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 162, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 163, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 164, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 165, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 166, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
	{ CHIP_GPIO, 167, DISABLE, DISABLE, GPIO_INPUT, GPIO_LOW, PUSH_PULL, GPIO_INT_DISABLE,
	  NULL },
};

bool pal_load_gpio_config(void)
{
	memcpy(&gpio_cfg[0], &plat_gpio_cfg[0], sizeof(plat_gpio_cfg));
	return 1;
};