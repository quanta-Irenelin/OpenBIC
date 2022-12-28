#include "cci.h"
#include "mctp.h"
#include <logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/slist.h>
#include <zephyr.h>
#include "sensor.h"
#include "plat_mctp.h"

LOG_MODULE_REGISTER(cci);
static int cxl_temp = 0;
static mctp_smbus_port smbus_port[] = {
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_CXL},
};
static mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_CXL, I2C_BUS_CXL, I2C_ADDR_CXL0 },
	// { MCTP_EID_CXL, I2C_BUS_CXL, I2C_ADDR_CXL1 },
};



static uint8_t mctp_cci_cmd_resp_process(mctp *mctp_inst, uint8_t *buf, uint32_t len,
					  mctp_ext_params ext_params)
{
	if (!mctp_inst || !buf || !len)
		return MCTP_ERROR;
    cxl_temp = buf[DEV_TEMP_OFFSET];
    printf("cxl temp: %02d\n", buf[DEV_TEMP_OFFSET]);

	return MCTP_SUCCESS;
}

int get_cxl_temp()
{
	return cxl_temp;
}

uint8_t mctp_cci_send_msg(void *mctp_p, mctp_cci_msg *msg)
{
	if (!mctp_p || !msg || !msg->cmd_data)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;

	mctp_reg_endpoint_resolve_func(mctp_inst, get_mctp_route_info);
	msg->hdr.msg_type = MCTP_MSG_TYPE_CCI;
	msg->ext_params.tag_owner = 1;
	
	uint16_t len = sizeof(msg->hdr) + msg->cmd_data_len;
	uint8_t buf[len];
	memcpy(buf, &msg->hdr, sizeof(msg->hdr));
	memcpy(buf + sizeof(msg->hdr), msg->cmd_data, msg->cmd_data_len);
	LOG_HEXDUMP_DBG(buf, len, __func__);
	uint8_t rc = mctp_send_msg(mctp_inst, buf, len, msg->ext_params);
	if (rc == MCTP_ERROR) {
		LOG_WRN("mctp_send_msg error!!");
		return MCTP_ERROR;
	}

	return MCTP_SUCCESS;
}

void send_cci(uint32_t cci_opcode)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mctp_route_tbl); i++) {
		mctp_route_entry *p = mctp_route_tbl + i;

		if (p->bus != smbus_port[0].conf.smbus_conf.bus){
			continue;
		}
		printk("Send CCI CMD for bus 0x%x, addr 0x%x\n", p->bus, p->addr);
		struct _set_cci_req req = { 0 };
		req.op = cci_opcode;

		mctp_cci_msg msg;
		memset(&msg, 0, sizeof(msg));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
		msg.ext_params.smbus_ext_params.addr = p->addr;

		msg.cmd_data = (uint8_t *)&req;
		msg.cmd_data_len = sizeof(req);

		mctp_cci_send_msg(find_mctp_by_smbus(p->bus), &msg);
	}
	
}

uint8_t mctp_cci_cmd_handler(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params)
{
	if (!mctp_p || !buf || !len)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;
    uint8_t mctp_msg_type = buf[0];
   	uint8_t pl_msg_resp = buf[1];

    if (mctp_msg_type!=MCTP_MSG_TYPE_CCI || (!pl_msg_resp)){
		return CCI_INVALID_RESP;
    }
	mctp_cci_cmd_resp_process(mctp_inst, buf, len, ext_params);
	
	return CCI_CC_SUCCESS;
}
