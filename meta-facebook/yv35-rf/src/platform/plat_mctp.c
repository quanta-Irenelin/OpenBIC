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

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <stdlib.h>
#include "mctp.h"
#include "mctp_ctrl.h"
#include "pldm.h"
#include "ipmi.h"
#include "sensor.h"
#include "plat_hook.h"
#include "plat_mctp.h"
#include "plat_gpio.h"
#include "cci.h"
#include "pm8702.h"

LOG_MODULE_REGISTER(plat_mctp);
K_TIMER_DEFINE(send_cmd_timer, send_cmd_to_dev, NULL);
K_WORK_DEFINE(send_cmd_work, send_cmd_to_dev_handler);
static mctp *cci_mctp_init;

mctp_smbus_port smbus_port[] = {
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_CXL },
};

mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_CXL, I2C_BUS_CXL, I2C_ADDR_CXL0 },
};

mctp *find_mctp_by_smbus(uint8_t bus)
{
	uint8_t i;
	for (i = 0; i < ARRAY_SIZE(smbus_port); i++) {
		mctp_smbus_port *p = smbus_port + i;
		if (bus == p->conf.smbus_conf.bus)
			return p->mctp_inst;
	}
	return NULL;
}

static void set_endpoint_resp_handler(void *args, uint8_t *buf, uint16_t len)
{
	if (!buf || !len)
		return;
	LOG_HEXDUMP_INF(buf, len, __func__);
}

static void set_endpoint_resp_timeout(void *args)
{
	mctp_route_entry *p = (mctp_route_entry *)args;
	printk("[%s] Endpoint 0x%x set endpoint failed on bus %d \n", __func__, p->endpoint,
	       p->bus);
}

static void set_dev_endpoint(void)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;
		for (uint8_t j = 0; j < ARRAY_SIZE(smbus_port); j++) {
			if (p->bus != smbus_port[j].conf.smbus_conf.bus) {
				continue;
			}
		printk("Prepare send endpoint bus 0x%x, addr 0x%x\n", p->bus, p->addr);
		struct _set_eid_req req = { 0 };
		req.op = SET_EID_REQ_OP_SET_EID;
		req.eid = p->endpoint;

		mctp_ctrl_msg msg;
		memset(&msg, 0, sizeof(msg));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
		msg.ext_params.smbus_ext_params.addr = p->addr;

		msg.hdr.cmd = MCTP_CTRL_CMD_SET_ENDPOINT_ID;
		msg.hdr.rq = 1;

		msg.cmd_data = (uint8_t *)&req;
		msg.cmd_data_len = sizeof(req);

		msg.recv_resp_cb_fn = set_endpoint_resp_handler;
		msg.timeout_cb_fn = set_endpoint_resp_timeout;
		msg.timeout_cb_fn_args = p;

		mctp_ctrl_send_msg(find_mctp_by_smbus(p->bus), &msg);
		}
	}
}

static uint8_t mctp_msg_recv(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params)
{
	if (!mctp_p || !buf || !len)
		return MCTP_ERROR;

	/* first byte is message type and ic */
	uint8_t msg_type = (buf[0] & MCTP_MSG_TYPE_MASK) >> MCTP_MSG_TYPE_SHIFT;
	uint8_t ic = (buf[0] & MCTP_IC_MASK) >> MCTP_IC_SHIFT;
	(void)ic;

	switch (msg_type) {
	case MCTP_MSG_TYPE_CTRL:
		mctp_ctrl_cmd_handler(mctp_p, buf, len, ext_params);
		break;

	case MCTP_MSG_TYPE_CCI:
		mctp_cci_cmd_handler(mctp_p, buf, len, ext_params);
		break;

	default:
		LOG_WRN("Cannot find message receive function!!");
		return MCTP_ERROR;
	}

	return MCTP_SUCCESS;
}

uint8_t get_mctp_route_info(uint8_t dest_endpoint, void **mctp_inst, mctp_ext_params *ext_params)
{	
	printk("_____EID: 0x%02x_______\n",dest_endpoint);

	if (!mctp_inst || !ext_params)
		return MCTP_ERROR;

	uint8_t rc = MCTP_ERROR;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;
		printk("_____EID: 0x%02x_______\n",dest_endpoint);
		if (p->endpoint == dest_endpoint) {
			if (gpio_get(p->dev_present_pin))
				return MCTP_ERROR;
			*mctp_inst = find_mctp_by_smbus(p->bus);
			ext_params->type = MCTP_MEDIUM_TYPE_SMBUS;
			ext_params->smbus_ext_params.addr = p->addr;
			rc = MCTP_SUCCESS;
			break;
		}
	}
	return rc;
}

void send_cmd_to_dev_handler(struct k_work *work)
{
	/* init the device endpoint */
	set_dev_endpoint();
}
void send_cmd_to_dev(struct k_timer *timer)
{
	k_work_submit(&send_cmd_work);
}

void plat_mctp_init()
{
	LOG_INF("plat_mctp_init");
	for (uint8_t i = 0; i < ARRAY_SIZE(smbus_port); i++) {
		mctp_smbus_port *p = smbus_port + i;
	/* init the mctp/cci instance */
	// mctp_smbus_port *p = smbus_port;
	LOG_DBG("bus = %x, addr = %x", p->conf.smbus_conf.bus, p->conf.smbus_conf.addr);

	p->mctp_inst = mctp_init();
	if (!p->mctp_inst) {
		LOG_ERR("mctp_init failed!!");
	}

	LOG_DBG("mctp_inst = %p", p->mctp_inst);
	uint8_t rc = mctp_set_medium_configure(p->mctp_inst, MCTP_MEDIUM_TYPE_SMBUS, p->conf);
	LOG_DBG("mctp_set_medium_configure %s", (rc == MCTP_SUCCESS) ? "success" : "failed");

	mctp_reg_endpoint_resolve_func(p->mctp_inst, get_mctp_route_info);
	mctp_reg_msg_rx_func(p->mctp_inst, mctp_msg_recv);

	mctp_start(p->mctp_inst);
	cci_mctp_init = p->mctp_inst;
	}
	/* Only send command to device when DC on */
	if (gpio_get(FM_POWER_EN) == POWER_ON) {
		k_timer_start(&send_cmd_timer, K_MSEC(3000), K_NO_WAIT);
	}
	
}

mctp *get_mctp_init()
{
	return cci_mctp_init;
}

#include <shell/shell.h>
#include "cci.h"
#include "mctp.h"
#include "plat_mctp.h"
#include "plat_hook.h"
static int test_pm8702_mctp_init(const struct shell *shell, size_t argc, char **argv)
{
	plat_mctp_init();
	return 0;
};

static int test_pm8702_set_eid(const struct shell *shell, size_t argc, char **argv)
{
	set_dev_endpoint();
	return 0;
}

static int read_dimm_temp(const struct shell *shell, size_t argc, char **argv)
{	
	mctp *mctp_inst = get_mctp_init();
	uint8_t interger = 0;
	uint8_t fraction = 0;
	pm8702_get_dimm_temp(mctp_inst, receiver_info[0].ext_params, dimm_info[0].dimm_data, &interger, &fraction);
	printk("dimm temp B: %02d.%02d\n",interger, fraction);
	pm8702_get_dimm_temp(mctp_inst, receiver_info[0].ext_params, dimm_info[1].dimm_data, &interger, &fraction);
	printk("dimm temp D: %02d.%02d\n",interger, fraction);
	return 0;
}

static int read_cxl_temp(const struct shell *shell, size_t argc, char **argv)
{
	mctp *mctp_inst = get_mctp_init();
	int16_t cxl_temp = 0;
	cci_get_chip_temp(mctp_inst, receiver_info[0].ext_params, &cxl_temp);
	printk("device temp : %d\n", cxl_temp);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pm8702_test,
			       SHELL_CMD(mctp_init, NULL, "MCTP init", test_pm8702_mctp_init),
			       SHELL_CMD(set_eid, NULL, "Set endpoing ID", test_pm8702_set_eid),
			       SHELL_CMD(read_cxl_temp, NULL, "read cxl temp", read_cxl_temp),
			       SHELL_CMD(read_dimm_temp, NULL, "read dimm temp", read_dimm_temp),

			       SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(pm8702, &sub_pm8702_test, "Test PM8702 Cmd", NULL);
