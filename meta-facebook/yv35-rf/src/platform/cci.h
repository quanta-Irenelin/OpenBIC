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

#ifndef _CCI_H
#define _CCI_H
#include "mctp.h"
 


typedef enum {
	CCI_GET_HEALTH_INFO = 0x4200,
    CCI_GET_FW_INFO = 0x0200
} CCI_CMD;

struct _cci_handler_query_entry {
	CCI_CMD type;
	void (*handler_query)(void *, uint8_t *, uint16_t);
};


typedef struct __attribute__((packed)) {
	uint8_t msg_type : 7;
	uint8_t ic : 1;
} mctp_cci_hdr;

typedef struct __attribute__((packed)){
	uint8_t msg_type;
	uint8_t msg_tag;
	uint8_t cci_rsv;
	uint16_t op;
	uint16_t pl_len;
	uint8_t rsv;
	uint16_t ret;
	uint16_t stat;
}cci_msg_body;

typedef struct {
	mctp_cci_hdr hdr;
	cci_msg_body msg_body;
}cci_msg_payload;

typedef struct {
	mctp_cci_hdr hdr;
	cci_msg_body msg_body;
	uint16_t cci_body_len;
	mctp_ext_params ext_params;
	void (*recv_resp_cb_fn)(void *, uint8_t *, uint16_t);
	void *recv_resp_cb_args;
	uint16_t timeout_ms;
	void (*timeout_cb_fn)(void *);
	void *timeout_cb_fn_args;
}mctp_cci_msg;

typedef uint8_t (*mctp_cci_cmd_fn)(void *, uint8_t *, uint16_t, uint8_t *, uint16_t *, void *);

typedef struct _mctp_cci_cmd_handler {
	uint8_t cmd_code;
	mctp_cci_cmd_fn fn;
} mctp_cci_cmd_handler_t;

#define DEV_TEMP_OFFSET 16
#define CCI_CC_INVALID_INPUT 0x0002

/*
 * CCI Control Completion Codes
 */
#define CCI_CC_SUCCESS 0x0000
#define CCI_CC_INVALID_INPUT 0x0002
#define CCI_CC_PAYLOAD_INVALID_LEN 0x0016

/*
 * CCI Return Codes
 */
#define CCI_SUCCESS 0x0000
#define CCI_INVALID_RESP 0x0002

int get_cxl_temp();

/*CCI command handler */
uint8_t mctp_cci_cmd_handler(void *mctp_p, uint8_t *buf, uint32_t len, mctp_ext_params ext_params);
bool post_cxl_temp_read(uint8_t sensor_num, void *args, int *reading);
void send_cci(uint32_t cci_opcode);

/* send CCI command message through mctp */
uint8_t mctp_cci_send_msg(void *mctp_p, mctp_cci_msg *msg);

#endif /* _CCI_H */