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
#include "plat_cci.h"

LOG_MODULE_REGISTER(cci);

#define DEFAULT_WAIT_TO_MS 3000
#define RESP_MSG_PROC_MUTEX_WAIT_TO_MS 1000
#define TO_CHK_INTERVAL_MS 1000
#define CCI_MSG_MAX_RETRY 3
#define CCI_MSG_TIMEOUT_MS 3000
#define CCI_READ_EVENT_SUCCESS BIT(0)
#define CCI_READ_EVENT_TIMEOUT BIT(1)

static K_MUTEX_DEFINE(wait_recv_resp_mutex);

static struct _cci_handler_query_entry cci_query_tbl[] = {
	{ CCI_GET_HEALTH_INFO, health_info_handler },
};

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
	mctp_cci_pkt_payload *body = (mctp_cci_pkt_payload *)buf;
	sys_snode_t *node;
	sys_snode_t *s_node;
	sys_snode_t *pre_node = NULL;
	sys_snode_t *found_node = NULL;

	SYS_SLIST_FOR_EACH_NODE_SAFE (&wait_recv_resp_list, node, s_node) {
		wait_msg *p = (wait_msg *)node;
		/* found the proper handler */
		printk("msg tag: 0x%02x, 0x%02x\n", p->msg.msg_body.msg_tag, body->msg_body.msg_tag);
		printk("op code: 0x%02x, 0x%02x\n", p->msg.msg_body.op, body->msg_body.op);
		if ((p->mctp_inst == mctp_inst) && (p->msg.msg_body.msg_tag == body->msg_body.msg_tag) && (p->msg.msg_body.op == body->msg_body.op)) {
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

void cci_read_resp_handler(void *args, uint8_t *rbuf, uint16_t rlen)
{
	if (!args || !rbuf || !rlen)
		return;

	cci_recv_resp_arg *recv_arg = (cci_recv_resp_arg *)args;

	if (rlen > recv_arg->rbuf_len) {
		LOG_WRN("[%s] response length(%d) is greater than buffer length(%d)!", __func__,
			rlen, recv_arg->rbuf_len);
		recv_arg->return_len = recv_arg->rbuf_len;
	} else {
		recv_arg->return_len = rlen;
	}
	memcpy(recv_arg->rbuf, rbuf, recv_arg->return_len);
	uint8_t status = CCI_READ_EVENT_SUCCESS;
	k_msgq_put(recv_arg->msgq, &status, K_NO_WAIT);
}

uint16_t mctp_cci_read(uint32_t cci_opcode, uint8_t receiver_bus, mctp_cci_msg *msg,uint8_t *rbuf, uint16_t rbuf_len)
{
	if (!receiver_bus || !msg )
		return 0;

	uint8_t event_msgq_buffer[1];
	struct k_msgq event_msgq;

	k_msgq_init(&event_msgq, event_msgq_buffer, sizeof(uint8_t), 1);

	void (*handler_query)(void *, uint8_t *, uint16_t) = NULL;
	for (int i = 0; i < ARRAY_SIZE(cci_query_tbl); i++) {
		if (cci_opcode == cci_query_tbl[i].type) {
			handler_query = cci_query_tbl[i].handler_query;
			break;
		}
	}
	cci_recv_resp_arg recv_arg;
	recv_arg.msgq = &event_msgq;
	recv_arg.rbuf = rbuf;
	recv_arg.rbuf_len = rbuf_len;

	msg->recv_resp_cb_fn = handler_query;
	msg->recv_resp_cb_args = (void *)&recv_arg;
	msg->timeout_cb_fn = mctp_cci_resp_timeout;
	msg->timeout_cb_fn_args = (void *)&event_msgq;
	msg->timeout_ms = CCI_MSG_TIMEOUT_MS;

	for (uint8_t retry_count = 0; retry_count < CCI_MSG_MAX_RETRY; retry_count++) {
		uint8_t event = 0;
		if (mctp_cci_send_msg(find_mctp_by_smbus(receiver_bus), msg) == CCI_ERROR) {
			LOG_WRN("[%s] send msg failed!", __func__);
			continue;
		}
		cci_read_resp_handler(msg->recv_resp_cb_args, rbuf, rbuf_len);
		if (k_msgq_get(&event_msgq, &event, K_MSEC(CCI_MSG_TIMEOUT_MS + 1000))) {
			LOG_WRN("[%s] Failed to get status from msgq!", __func__);
			continue;
		}
		if (event == CCI_READ_EVENT_SUCCESS) {
			return recv_arg.return_len;
		}
	}
	LOG_WRN("[%s] retry reach max!", __func__);
	return 0;
}

uint8_t mctp_cci_cmd_handler(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params)
{
	if (!mctp_p || !buf || !len)
		return MCTP_ERROR;

	mctp *mctp_inst = (mctp *)mctp_p;
	mctp_cci_pkt_payload *pkt_pl = (mctp_cci_pkt_payload *)buf;

    uint8_t mctp_msg_type = pkt_pl->hdr.msg_type;
   	uint8_t cci_msg_resp = pkt_pl->msg_body.cci_msg_type;

    if (mctp_msg_type!=MCTP_MSG_TYPE_CCI || (!cci_msg_resp)){
		return CCI_INVALID_RESP;
    }
	mctp_cci_cmd_resp_process(mctp_inst, buf, len, ext_params);
	
	return CCI_CC_SUCCESS;
}

K_THREAD_DEFINE(monitor_cci_msg, 1024, mctp_cci_msg_timeout_monitor, NULL, NULL, NULL, 7, 0, 0);