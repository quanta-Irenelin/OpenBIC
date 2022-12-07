/*
** Include Files
*/
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "srmburn.h"
#include "plat_i2c.h"
#include <drivers/disk.h>
#include "util_spi.h"
#include "libutil.h"


#include <plat_power_seq.h>
#include "hal_gpio.h"
#include "util_spi.h"
#include "util_sys.h"
#include "plat_gpio.h"
/*
** indicate the current target
**    TRUE: communicate with pboot
**    FLASE: communicate with SRA device app
*/
bool to_boot;
int final_flag = false;
int srm_send_init = true;
unsigned int written = 0;
unsigned int device_index = 0;

/* Rom Idle states */
static const unsigned char rom_idle_states[ROM_PROTO_CNT] =
{
    [ROM_PROTO_0] = SRM_STATE_IDLE_PROTO_0,
    [ROM_PROTO_1] = SRM_STATE_IDLE_PROTO_1,
    [ROM_PROTO_2] = SRM_STATE_IDLE_PROTO_2,
};
static srm_protocol_ver proto_ver = ROM_PROTO_UNKOWN;


/**
* @brief
*   Read state from device
*
* @param[out] status - buffer to hold state information
* @param[in] wait_state - 0: Don't wait state
*                         value: target state to wait
*
* @return
*   SUCCESS
*
* @note
*   Pboot returns 4B state and SRA returns 6B state.
*   When read error happens, read sync can be broken because it's reading state byte by byte
*   We know the first byte should be 0x3D or 0x3E, so if we can't receive it on the first byte,
*   Checking more bytes upto maxium length.
*/
int srm_get_status(unsigned char *status, unsigned char twi_cmd, unsigned char wait_state)
{
    unsigned char *data = status;
    unsigned int state_count = to_boot ? 4: 6;
    unsigned char target_app = to_boot ? SRM_PBOOT_ID : SRM_SRA_ID;
    unsigned int resync_count = 0;
    unsigned char error_state = 0;
    unsigned int error_state_count = 0;
    int i;
    uint8_t retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = CXL_RECOVER_ADDR;
	msg.tx_len = 1;
	msg.rx_len = to_boot ? 4: 6;
    int cnt = to_boot? 4: 6;
    msg.data[0] = twi_cmd;
    if(i2c_master_read(&msg, retry)){
        printf("Failed to read CXL status\n");
        return -1;
    }
    while (1)
    {
        for (i = 0; (i < state_count) && (resync_count < state_count);){

            data[i] = msg.data[i];
            printf("cxl status: ");
            for(int j=0;j<cnt;j++){
                printf("0x%02x ", data[j]);
            }
            printf("\n");
            printf("i: %d, state_count: %d, resync_count: %d\n",i, state_count,resync_count);
            k_msleep(100);
            if (i == 0){
                /* Check app ID: 3D - pboot, 3E - SRA */
                if (data[0] != target_app){
                    /* Wrong app index: device is not able to respond now or sync is broken. stay in index 0 */
                    resync_count++;
                    continue;
                }else{
                    /* Found right start byte, read full state */
                    resync_count = 0;
                }
            }else if (i == 1){
                bool state_failure = false;
                /*
                *  handles boot rom idle status separately because
                *  the status value is indicating boot rom version
                */
                if (to_boot && (wait_state == SRM_STATE_IDLE))
                {
                    if (proto_ver == ROM_PROTO_UNKOWN){
                        for (int i = ROM_PROTO_0; i < ROM_PROTO_CNT; i ++){
                            if (data[1] == rom_idle_states[i]){
                                proto_ver = i;
                            }
                        }if (proto_ver == ROM_PROTO_UNKOWN){
                            state_failure = true;
                        }
                    }else{
                        if (data[1] != rom_idle_states[proto_ver]){
                            state_failure = true;
                        }
                    }
                }else if ((0 != wait_state) && (data[1] != wait_state)){
                    state_failure = true;
                    if (error_state != data[1]){
                        error_state = data[1];
                        error_state_count = 0;
                    }else{
                        error_state_count++;
                        if (error_state_count % 100 == 0){
                            printf("Wrong State: 0x%x\n", error_state);
                        }
                    }
                }

                /* Check target state */
                if (state_failure){
                    i = 0;          /* Find again from start */
                    resync_count++;
                    continue;
                }else{
                    /* Found right state, read full state */
                    resync_count = 0;
                }
            }
            i++; 
        }
        printf("cxl status ");
        for(int j=0;j<cnt;j++){
            printf("0x%02x ", data[j]);
        }
        printf("\n");
        k_msleep(100);

        if (resync_count == 0){
            /* No issue get out of loop */
            break;
        }else{
            if (wait_state == 0){
                break;
            }
            k_msleep(WAIT_TIME_SEC);
            i = 0;
            resync_count = 0;
        }
    }

    return (resync_count == 0) ? SUCCESS : FAIL;
}

int srm_send_block(unsigned char *status, uint8_t *data)
{   
    unsigned char *return_status = status;
    int ret = SUCCESS;
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = CXL_RECOVER_ADDR;
	msg.tx_len = 1;
	msg.rx_len = 0;

    /* Send Packet Data Init Flag */
    msg.data[0] = SRM_PKT_CMD_FLAG;
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to write SRM_PKT_CMD_FLAG\n");
        return FAIL;
    }

    /* Send Packet Data Flag (keep Write flag/Final write flag) */
    msg.data[0] = final_flag ? SRM_PKT_FINAL_WRITE : SRM_PKT_KEEP_WRITE;
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to write flag\n");
        return FAIL;
    }

    for(int i = 0; i< BLOCK_TX_LENGTH;i++){
        msg.data[0] = *(data+i);
        /* Send Packet Data by one byte */
        if(i < BLOCK_TX_LENGTH-1){
            if (i2c_master_write(&msg, retry)) {
                printf("Failed to write block\n");
                return FAIL;
            }
        }
        if(i == BLOCK_TX_LENGTH-1){
            msg.rx_len = to_boot ? 4: 6;
            if (i2c_master_read(&msg, retry)) {
                printf("Failed to write block\n");
                return FAIL;
            }
            for(int i=0;i< msg.rx_len;i++){
                return_status[i] = msg.data[i];
            } 
        }
    }      
    return ret;
}

int srm_send_size()
{
    int ret = SUCCESS;
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = CXL_RECOVER_ADDR;
	msg.tx_len = 1;
	msg.rx_len = 1;
	msg.data[0] = 0x58;
    /* Send file size */
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to write size\n");
        return -1;
    } 
	msg.tx_len = 1;
	msg.rx_len = to_boot ? 4: 6;   
    msg.data[0] = 0x33;
    if (i2c_master_read(&msg, retry)) {
        printf("Failed to write size\n");
        return -1;
    }         
    return ret;
}


/**
* @brief
*   Processing Side Band Recovery
* @param[in]    - None
* @return
*   SUCCESS or Error code
* @note
*/
int srm_run(void)
{   
    int ret = 0;
    unsigned char status[6] = {0};
    to_boot = true;
    srm_protocol_ver proto_ver_prev;
    printf("Start communication and find/recheck rom version\n\n");

    /* Start communication and find/recheck rom version */
    do{
        proto_ver_prev = proto_ver;     /* to check rom version twice */
        proto_ver = ROM_PROTO_UNKOWN;
        srm_get_status(status, SRM_CMD_BYTE_INIT, SRM_STATE_IDLE);
    } while ((proto_ver == ROM_PROTO_UNKOWN) || (proto_ver_prev != proto_ver));

    printf("BOOT ROM Ver: %c\n", 'A' + proto_ver - ROM_PROTO_0);

    return ret;
}



uint8_t cxl_do_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end){
	static bool is_init = 0;
	static uint8_t *txbuf = NULL;
	static uint32_t buf_offset = 0;
	uint32_t ret = 0;
    unsigned char status[6] = {0};
    unsigned int size = 52572;

    set_CXL_update_status(POWER_ON);
    if(srm_send_init){
        srm_run();
        /* Initialize State and Check if device is ready */
        srm_get_status(status,SRM_CMD_BYTE_RESET, SRM_STATE_IDLE);
        /* pboot uses word length whereas SRA uses byte length */
        if (to_boot){
            size = size >> 2;
            /* Account for dword unaligned sizes */
            if ((52572 % 4) > 0){
                size++;
            }
        }
        srm_get_status(status,SRM_CMD_BYTE_IMGWRITE, SRM_STATE_WRITE);
        printf("WRITE STATE\n");
        srm_send_size();
        printf("SEND SIZE\n");
        srm_send_init = false;
    }
    printf("run status: %d\n",srm_send_init);

	if (!is_init) {
		SAFE_FREE(txbuf);
		txbuf = (uint8_t *)malloc(SECTOR_SZ_BYTES);
		if (txbuf == NULL) { // Retry alloc
			k_msleep(100);
			txbuf = (uint8_t *)malloc(SECTOR_SZ_BYTES);
		}
		if (txbuf == NULL) {
			printf("i2c index failed to allocate txbuf");
			return FWUPDATE_OUT_OF_HEAP;
		}
		is_init = 1;
		buf_offset = 0;
		k_msleep(10);
	}

    printf("device index: 0x%02x\n", device_index);
	memcpy(&txbuf[buf_offset], msg_buf, msg_len);
	buf_offset += msg_len;

	// i2c master write while collect 128 bytes data or BMC signal last image package with target | 0x80
	if ((buf_offset == SECTOR_SZ_BYTES) || sector_end) {
		uint8_t sector = SECTOR_SZ_BYTES/BLOCK_TX_LENGTH;
		uint32_t txbuf_offset;
		uint32_t update_offset;

		for (int i = 0; i < sector; i++) {
            unsigned int len = BLOCK_TX_LENGTH;
            if (to_boot){
                if (len > ((size << 2) - written)){
                    len = ((size << 2) - written);
                }
            }else{
                if (len > (size - written)){
                    len = (size - written);
                }
            }
            
            unsigned char retry = 0;
            while (device_index != written){
                printf("device_index (0x%08x) is not matching with sending offset (0x%08x)\n", device_index, written);
                if (retry++ >= 5){
                    printf("device is in wrong state\n");
                    return EFAIL;
                }
                k_msleep(WAIT_TIME_SEC);
                device_index = to_boot ?
                                (((unsigned int)status[3] << 8) + status[2]) << 2 :
                                (((unsigned int)status[5] << 24) + ((unsigned int)status[4] << 16) +
                                ((unsigned int)status[3] << 8) + status[2]);
            }
			txbuf_offset = BLOCK_TX_LENGTH * i;

			update_offset = (offset / SECTOR_SZ_BYTES) * SECTOR_SZ_BYTES + txbuf_offset;
            ret = srm_send_block(status, &txbuf[txbuf_offset]);
            device_index = to_boot ?
                (((unsigned int)status[3] << 8) + status[2]) << 2 :
                (((unsigned int)status[5] << 24) + ((unsigned int)status[4] << 16) +
                ((unsigned int)status[3] << 8) + status[2]);
            printf("i: %d, update_offset 0x%02x, ret:%d, written 0x%02x\n",i ,update_offset ,ret,written);
            written += len;

            if(update_offset == (size-BLOCK_TX_LENGTH)){
                final_flag = 1;
                k_msleep(WAIT_TIME_SEC);
                SAFE_FREE(txbuf);
                k_msleep(10);
                return SRM_CMD_BYTE_VERIFY;
            }
		}
        if(ret){
            return FWUPDATE_UPDATE_FAIL;
        }
        SAFE_FREE(txbuf);
        k_msleep(10);
		is_init = 0;
		return ret;
	}
    return ret;
}

int srm_verify(unsigned char twi_command, unsigned char status)
{
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = CXL_RECOVER_ADDR;
	msg.tx_len = 1;
	msg.rx_len = to_boot ? 4: 6;
	msg.data[0] = twi_command;

    if (i2c_master_read(&msg, retry)) {
        printf("Failed to 0x%02x read\n",twi_command);
        return FAIL;
    }

    return msg.data[1] == status ? SUCCESS : FAIL;
}


int srm_write_byte(char twi_command)
{
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = CXL_RECOVER_ADDR;
	msg.tx_len = 1;
	msg.rx_len = 0;
	msg.data[0] = twi_command;
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to 0x%02x read\n",twi_command);
        return FAIL;
    }
    return SUCCESS;
}


uint8_t cxl_recovery_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end)
{   
    uint8_t ret = FWUPDATE_UPDATE_FAIL;
	set_CXL_update_status(POWER_ON);
    uint8_t is_cxl_DC_ON = gpio_get(FM_POWER_EN);
    unsigned char status[6] = {0};
    for(int i=0; i<5;i++){
        if(is_cxl_DC_ON == POWER_ON){
            ret = cxl_do_update(offset, msg_len, msg_buf, sector_end);
            if(ret == SRM_CMD_BYTE_VERIFY){
                ret = srm_verify(SRM_CMD_BYTE_VERIFY, SRM_STATE_VERIFY);
                printf("ret: %d\n", ret);
                printf("\nSend cmd to go into EXEC state\n");
                srm_write_byte(SRM_CMD_BYTE_EXEC);
                k_msleep(1000);
                to_boot = false;
                printf("WRITE STATE\n");
                srm_write_byte(SRM_CMD_BYTE_RESET);
                printf("\nSend cmd to go into Reset state\n");
               
            }
            return ret;
        }else{
            k_msleep(300);
            is_cxl_DC_ON = gpio_get(FM_POWER_EN);
        }
    }    

	return ret;
}