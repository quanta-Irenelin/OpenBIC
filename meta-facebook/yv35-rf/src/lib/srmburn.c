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
/* Print every 64KB write */
#define SRMBURN_PRINT_PERIOD (64 * 1024)

/*
** indicate the current target
**    TRUE: communicate with pboot
**    FLASE: communicate with SRA device app
*/
bool to_boot;

/* Error counters */
unsigned int error_cnt = 0;
/* i2c device handler */
int i2c_dev_handle = 0;
/* i2c device name  */
char i2c_file_name[MAX_FILE_NAME_LENGTH];
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
*   Write byte to device
* @param[in] file  - file descriptor
* @param[in] value - value to write
* @return
*   Success or failure
* @note
*/
int i2c_write_byte(unsigned char command)
{
    int ret = SUCCESS;
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = 0x27;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = command;

    if (i2c_master_write(&msg, retry)) {
        printf("Failed to 0x%02x write\n",command);
        return -1;
    }
    return ret;
}

/**
* @brief
*   Send Reset command
* @param[in]    - None
* @return
*   SUCCESS or Error code
* @note
*/
int srm_reset(void)
{
    int ret = SUCCESS;
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = 0x27;
	msg.tx_len = 1;
	msg.rx_len = 2;
	msg.data[0] = SRM_CMD_BYTE_RESET;

    /* Request to reset device state machine */
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to reset\n");
        return -1;
    }

    return ret;
}


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
int srm_get_status(unsigned char *status, unsigned char wait_state)
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
	msg.target_addr = 0x27;
	msg.tx_len = 1;
	msg.rx_len = to_boot ? 4: 6;
	msg.data[0] = to_boot ? SRM_PBOOT_ID : SRM_SRA_ID;
    int cnt = to_boot? 4: 6;

    while (1)
    {
        for (i = 0; (i < state_count) && (resync_count < state_count);){
            if(i2c_master_read(&msg, retry)){
                printf("Failed to read CXL status\n");
                return -1;
            }
            data[i] = msg.data[i];
            printf("cxl status: ");
            for(int j=0;j<cnt;j++){
                printf("0x%02x ", data[j]);
            }
            printf("\n");
            printf("i: %d, state_count: %d, resync_count: %d\n",i, state_count,resync_count);
            k_msleep(10000);
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
                if (to_boot && (wait_state == SRM_STATE_IDLE)){
                    if (proto_ver == ROM_PROTO_UNKOWN){
                        for (int i = ROM_PROTO_0; i < ROM_PROTO_CNT; i ++){
                            if (data[1] == rom_idle_states[i]){
                                proto_ver = i;
                            }
                            printf("proto_ver: %d\n",proto_ver);    
                        }
                        if (proto_ver == ROM_PROTO_UNKOWN){
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
        printf("cxl status");
        for(int j=0;j<cnt;j++){
            printf("0x%02x ", data[j]);
        }
        printf("\n");
        k_msleep(10000);

        if (resync_count == 0){
            /* No issue get out of loop */
            break;
        }else{
            if (wait_state == 0){
                break;
            }
#if (DBG_SRM_EN == 1)
            if (to_boot)
            {
                printf("Inavlid State: 0x%x 0x%x 0x%x 0x%x\n",
                       data[0], data[1], data[2], data[3]);
            }
            else
            {
                printf("Target State: 0x%x, Inavlid State: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                       wait_state, data[0], data[1], data[2], data[3], data[4], data[5]);
            }
#endif
            k_msleep(WAIT_TIME_SEC);
            i = 0;
            resync_count = 0;
        }
    }

    return (resync_count == 0) ? SUCCESS : FAIL;
}

int srm_send_block(uint8_t *data)
{
    int ret = SUCCESS;
    int retry = 5;
    I2C_MSG msg = {0};
	msg.bus = 1;
	msg.target_addr = 0x27;
	msg.tx_len = 32;
	msg.rx_len = 2;
    for(int i = 0; i<32;i++){
	    msg.data[i] = *(data+i);
    }
    /* Send Packet Data */
    if (i2c_master_write(&msg, retry)) {
        printf("Failed to write block\n");
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
    printf("SRM reset\n");
    srm_reset();
    printf("Start communication and find/recheck rom version");
    /* Start communication and find/recheck rom version */
    do{
        proto_ver_prev = proto_ver;     /* to check rom version twice */
        proto_ver = ROM_PROTO_UNKOWN;
        srm_get_status(status, SRM_STATE_IDLE);
    } while ((proto_ver == ROM_PROTO_UNKOWN) || (proto_ver_prev != proto_ver));

    printf("BOOT ROM Ver: %C\n", 'A' + proto_ver - ROM_PROTO_0);

    /* Send device SRA image */
    // printf("\nSending Sideband Recovery Application: %s\n", arguments.sra_img);
    // ret = srm_send_image(arguments.sra_img);

    /* Send firmware image */
    to_boot = false;

    // printf("\nSending Firmware Image: %s\n", arguments.fw_img_path);
    // ret = srm_send_image(arguments.fw_img_path);

    return ret;
}

uint8_t cxl_recovery_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end)
{
	static bool is_init = 0;
	static uint8_t *txbuf = NULL;
	static uint32_t buf_offset = 0;
	uint32_t ret = 0;
    set_CXL_update_status(POWER_ON);
    srm_reset();
    i2c_write_byte(SRM_CMD_BYTE_IMGWRITE);
	if (!is_init) {
		SAFE_FREE(txbuf);
		txbuf = (uint8_t *)malloc(128);
		if (txbuf == NULL) { // Retry alloc
			k_msleep(100);
			txbuf = (uint8_t *)malloc(128);
		}
		if (txbuf == NULL) {
			printf("i2c index failed to allocate txbuf");
			return FWUPDATE_OUT_OF_HEAP;
		}
		is_init = 1;
		buf_offset = 0;
		k_msleep(10);
	}

	memcpy(&txbuf[buf_offset], msg_buf, msg_len);
	buf_offset += msg_len;

	// i2c master write while collect 128 bytes data or BMC signal last image package with target | 0x80
	if ((buf_offset == 128) || sector_end) {
		uint8_t sector = 4;
		uint32_t txbuf_offset;
		uint32_t update_offset;

		for (int i = 0; i < sector; i++) {
			txbuf_offset = 32 * i;
			update_offset = (offset / 128) * 128 + txbuf_offset;
            srm_send_block(&txbuf[txbuf_offset]);
            printf("update_offset 0x%02x",update_offset);
		}
		SAFE_FREE(txbuf);
		k_msleep(10);
		is_init = 0;

		return ret;
	}

	return FWUPDATE_SUCCESS;
}