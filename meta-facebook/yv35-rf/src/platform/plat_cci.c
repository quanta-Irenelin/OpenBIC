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

LOG_MODULE_REGISTER(plat_cci);

static int cxl_temp = 0;
static uint16_t cci_tag = 0;
static struct _cci_handler_query_entry cci_query_tbl[] = {
	{ CCI_GET_HEALTH_INFO, health_info_handler },
};

uint16_t cci_platform_read(uint32_t cci_opcode, uint32_t pl_len, mctp_ext_params ext_params, uint8_t receiver_bus)
{   
        
    cci_msg_body req = { 0 };

    req.msg_tag = cci_tag++;
    req.op = cci_opcode;

    mctp_cci_msg msg;
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg.ext_params, &ext_params, sizeof(mctp_ext_params));

    msg.msg_body = req;
    msg.pl_data_len = 0;

    int resp_len = sizeof(cci_msg_body) + pl_len;

    uint8_t rbuf[resp_len];

    mctp_cci_read(receiver_bus, &msg, rbuf, resp_len);

    void (*handler_query)(uint8_t *, uint16_t) = NULL;
    for (int i = 0; i < ARRAY_SIZE(cci_query_tbl); i++) {
        if (cci_opcode == cci_query_tbl[i].type) {
            handler_query = cci_query_tbl[i].handler_query;
            break;
        }
    }

    handler_query(rbuf, resp_len);
    
    return 0;
}

void health_info_handler(uint8_t *buf, uint16_t len)
{
	if (!buf || !len)
		return;
	LOG_HEXDUMP_INF(buf, len, __func__);
    cxl_temp = buf[DEV_TEMP_OFFSET];
    printf("cxl temp: %02d\n", buf[DEV_TEMP_OFFSET]);
}

int get_cxl_temp()
{
	return cxl_temp;
}