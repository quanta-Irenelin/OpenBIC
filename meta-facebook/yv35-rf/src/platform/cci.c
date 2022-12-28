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

#define DEFAULT_WAIT_TO_MS 3000
#define RESP_MSG_PROC_MUTEX_WAIT_TO_MS 1000
#define TO_CHK_INTERVAL_MS 1000

static int cxl_temp = 0;
static mctp_smbus_port smbus_port[] = {
	{ .conf.smbus_conf.addr = I2C_ADDR_BIC, .conf.smbus_conf.bus = I2C_BUS_CXL},
};
static mctp_route_entry mctp_route_tbl[] = {
	{ MCTP_EID_CXL, I2C_BUS_CXL, I2C_ADDR_CXL0 },
	// { MCTP_EID_CXL, I2C_BUS_CXL, I2C_ADDR_CXL1 },
};

typedef struct _wait_msg {
	sys_snode_t node;
	mctp *mctp_inst;
	int64_t exp_to_ms;
	mctp_cci_msg msg;
} wait_msg;

static K_MUTEX_DEFINE(wait_recv_resp_mutex);
static sys_slist_t wait_recv_resp_list = SYS_SLIST_STATIC_INIT(&wait_recv_resp_list);

static uint8_t mctp_cci_msg_timeout_check(sys_slist_t *list, struct k_mutex *mutex)
{
	if (!list || !mutex)
		return MCTP_ERROR;

	if (k_mutex_lock(mutex, K_MSEC(RESP_MSG_PROC_MUTEX_WAIT_TO_MS))) {
		LOG_WRN("cci mutex is locked over %d ms!!", RESP_MSG_PROC_MUTEX_WAIT_TO_MS);
		return MCTP_ERROR;
	}

	sys_snode_t *node;
	sys_snode_t *s_node;
	sys_snode_t *pre_node = NULL;
	int64_t cur_uptime = k_uptime_get();

	SYS_SLIST_FOR_EACH_NODE_SAFE (list, node, s_node) {
		wait_msg *p = (wait_msg *)node;

		if ((p->exp_to_ms <= cur_uptime)) {
			printk("mctp cci msg timeout!!\n");
			sys_slist_remove(list, pre_node, node);

			if (p->msg.timeout_cb_fn)
				p->msg.timeout_cb_fn(p->msg.timeout_cb_fn_args);

			free(p);
		} else {
			pre_node = node;
		}
	}

	k_mutex_unlock(mutex);
	return MCTP_SUCCESS;
}

static void mctp_cci_msg_timeout_monitor(void *dummy0, void *dummy1, void *dummy2)
{
	ARG_UNUSED(dummy0);
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);

	while (1) {
		k_msleep(TO_CHK_INTERVAL_MS);

		mctp_cci_msg_timeout_check(&wait_recv_resp_list, &wait_recv_resp_mutex);
	}
}

static void mctp_cci_resp_handler(void *args, uint8_t *buf, uint16_t len)
{
	if (!buf || !len)
		return;
	cxl_temp = buf[DEV_TEMP_OFFSET];
    printf("cxl temp: %02d\n", buf[DEV_TEMP_OFFSET]);

	LOG_HEXDUMP_INF(buf, len, __func__);
}

static struct _cci_handler_query_entry cci_query_tbl[] = {
	{ CCI_GET_HEALTH_INFO, mctp_cci_resp_handler },
};


static void mctp_cci_resp_timeout(void *args)
{
	mctp_route_entry *p = (mctp_route_entry *)args;
	printk("[%s]  send cci failed on endpoint 0x%x, bus %d \n", __func__, p->endpoint,
	       p->bus);
}

static uint8_t mctp_cci_cmd_resp_process(mctp *mctp_inst, uint8_t *buf, uint32_t len,
					  mctp_ext_params ext_params)
{
	if (!mctp_inst || !buf || !len)
		return MCTP_ERROR;

	if (k_mutex_lock(&wait_recv_resp_mutex, K_MSEC(RESP_MSG_PROC_MUTEX_WAIT_TO_MS))) {
		LOG_WRN("mutex is locked over %d ms!", RESP_MSG_PROC_MUTEX_WAIT_TO_MS);
		return MCTP_ERROR;
	}
	// _set_cci_req *hdr = (_set_cci_req *)buf;
	sys_snode_t *node;
	sys_snode_t *s_node;
	sys_snode_t *pre_node = NULL;
	sys_snode_t *found_node = NULL;

	SYS_SLIST_FOR_EACH_NODE_SAFE (&wait_recv_resp_list, node, s_node) {
		wait_msg *p = (wait_msg *)node;
		/* found the proper handler */
		if (p->mctp_inst == mctp_inst) {
			found_node = node;
			sys_slist_remove(&wait_recv_resp_list, pre_node, node);
			break;
		} else {
			pre_node = node;
		}
	}
	k_mutex_unlock(&wait_recv_resp_mutex);

	if (found_node) {
		/* invoke resp handler */
		wait_msg *p = (wait_msg *)found_node;
		if (p->msg.recv_resp_cb_fn)
			p->msg.recv_resp_cb_fn(
				p->msg.recv_resp_cb_args, buf + sizeof(p->msg.hdr),
				len - sizeof(p->msg.hdr)); /* remove mctp ctrl header for handler */
		free(p);
	}

	return MCTP_SUCCESS;
}

int get_cxl_temp()
{
	return cxl_temp;
}

uint8_t mctp_cci_send_msg(void *mctp_p, mctp_cci_msg *msg)
{
	if (!mctp_p || !msg )
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;

	mctp_reg_endpoint_resolve_func(mctp_inst, get_mctp_route_info);
	msg->hdr.msg_type = MCTP_MSG_TYPE_CCI;
	msg->ext_params.tag_owner = 1;
	
	uint16_t len = sizeof(msg->hdr) + msg->cci_body_len;
	uint8_t buf[len];
	memcpy(buf, &msg->hdr, sizeof(msg->hdr));
	memcpy(buf + sizeof(msg->hdr), &msg->msg_body, msg->cci_body_len);
	LOG_HEXDUMP_DBG(buf, len, __func__);
	uint8_t rc = mctp_send_msg(mctp_inst, buf, len, msg->ext_params);
	if (rc == MCTP_ERROR) {
		LOG_WRN("mctp_send_msg error!!");
		return MCTP_ERROR;
	}

	wait_msg *p = (wait_msg *)malloc(sizeof(*p));
	if (!p) {
		LOG_WRN("wait_msg alloc failed!");
		return MCTP_ERROR;
	}

	memset(p, 0, sizeof(*p));
	p->mctp_inst = mctp_inst;
	p->msg = *msg;
	p->exp_to_ms =
		k_uptime_get() + (msg->timeout_ms ? msg->timeout_ms : DEFAULT_WAIT_TO_MS);

	k_mutex_lock(&wait_recv_resp_mutex, K_FOREVER);
	sys_slist_append(&wait_recv_resp_list, &p->node);
	k_mutex_unlock(&wait_recv_resp_mutex);
	

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
		cci_msg_body req = { 0 };
		void (*handler_query)(void *, uint8_t *, uint16_t) = NULL;
		for (i = 0; i < ARRAY_SIZE(cci_query_tbl); i++) {
			if (cci_opcode == cci_query_tbl[i].type) {
				handler_query = cci_query_tbl[i].handler_query;
				break;
			}
		}
		req.op = cci_opcode;

		mctp_cci_msg msg;
		memset(&msg, 0, sizeof(msg));
		msg.ext_params.type = MCTP_MEDIUM_TYPE_SMBUS;
		msg.ext_params.smbus_ext_params.addr = p->addr;
		msg.msg_body = req;
		msg.cci_body_len = sizeof(req);

		msg.recv_resp_cb_fn = handler_query;
		msg.timeout_cb_fn = mctp_cci_resp_timeout;
		msg.timeout_cb_fn_args = p;

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

K_THREAD_DEFINE(monitor_cci_msg, 1024, mctp_cci_msg_timeout_monitor, NULL, NULL, NULL, 7, 0, 0);