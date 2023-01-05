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
#include "pm8702.h"

LOG_MODULE_REGISTER(plat_cci);

static uint16_t cci_tag = 0;

static struct _cci_handler_query_entry cci_query_tbl[] = {
	{ CCI_GET_HEALTH_INFO, health_info_handler },
    { CCI_I2C_OFFSET_READ, dimm_temp_handler },
};

static opcode_route_entry cci_opcode_tbl[2] = {
	{ CCI_GET_HEALTH_INFO, HEALTH_INFO_REQ_PL_LEN, HEALTH_INFO_PL_LEN },
    { CCI_I2C_OFFSET_READ, I2C_OFFSET_READ_REQ_PL_LEN, I2C_OFFSET_READ_PL_LEN},
};

static uint8_t pl_data[20] =
{
    0x00, 0x00, 0x19, 0x00, 
    0x01, 0x00, 0x05, 0x00, 
    0x01, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 
};

uint16_t cci_platform_read(uint32_t cci_opcode, mctp_ext_params ext_params)
{   
        
    cci_msg_body req = { 0 };
    opcode_route_entry *p;

    for (uint8_t i = 0; i < ARRAY_SIZE(cci_opcode_tbl); i++) {
        if(cci_opcode == cci_opcode_tbl[i].op_code){
		    p = cci_opcode_tbl + i;
        }
    }
    mctp *mctp_init = get_mctp_init();

    req.msg_tag = cci_tag++;
    req.op = cci_opcode;
    req.pl_len = p->CCI_REQ_PL_LEN;
    printk("req pl_len : %d, resp pl_len: %d\n", p->CCI_REQ_PL_LEN, p->CCI_RESP_PL_LEN);

    mctp_cci_msg msg;
    memset(&msg, 0, sizeof(msg));
    memcpy(&msg.ext_params, &ext_params, sizeof(mctp_ext_params));

    msg.msg_body = req;
    msg.pl_data_len = p->CCI_REQ_PL_LEN;
    memcpy(msg.pl_data, pl_data, p->CCI_REQ_PL_LEN);

    int resp_len = sizeof(cci_msg_body) + p->CCI_RESP_PL_LEN;
    uint8_t rbuf[resp_len];

    mctp_cci_read(mctp_init, &msg, rbuf, resp_len);

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


