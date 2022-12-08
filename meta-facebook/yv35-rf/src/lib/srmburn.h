/******************************************************************************
*  Copyright 2022 Microchip Technology Inc. and its subsidiaries.
*  Subject to your compliance with these terms, you may use Microchip
*  software and any derivatives exclusively with Microchip products. It is
*  your responsibility to comply with third party license terms applicable to
*  your use of third party software (including open source software) that may
*  accompany Microchip software.
*  THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
*  EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
*  IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
*  PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT,
*  SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR
*  EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED,
*  EVEN IF MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
*  FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL
*  LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT
*  EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO
*  MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#ifndef _SRM_H
#define _SRM_H

#include <stdbool.h>
#include <stdio.h>
#define CXL_RECOVER_ADDR               0x27
/*
*  Device side HW Rx buffer size is 128B so we need to check device status.
*  if we keeps sending data while the device is busy, then HW fifo overflows and lost data sync
*/
#define SECTOR_SZ_BYTES                128 
#define SRM_SIZE                       52572 
#define BLOCK_TX_LENGTH         32
#define WAIT_TIME_SEC   1
#define MAX_RETRY       3
#define FAIL   -1
#define SUCCESS 0

/* Recovery mode slave identifier */
#define SRM_PBOOT_ID            0x3D       /**< pboot identifier */
#define SRM_SRA_ID              0x3E       /**< sra identifier */

#define SRM_STATE_IDLE          0xB1       /**< IDLE state */
#define SRM_STATE_IDLE_PROTO_0  0xB1       /**< IDLE state Rom Protocol Ver 0 */
#define SRM_STATE_IDLE_PROTO_1  0xB8       /**< IDLE state Rom Protocol Ver 1 */
#define SRM_STATE_IDLE_PROTO_2  0xB9       /**< IDLE state Rom Protocol Ver 2 */
#define SRM_STATE_WRITE         0xB2       /**< WRITE state */
#define SRM_STATE_VERIFY        0xB3       /**< VERIFY state */

/* TWI command bytes from TWI master */
#define SRM_CMD_BYTE_IMGWRITE   0xA1       /**< IMGWRITE command */
#define SRM_CMD_BYTE_VERIFY     0xA2       /**< VERIFY command */
#define SRM_CMD_BYTE_EXEC       0xA3       /**< EXEC command */
#define SRM_CMD_BYTE_RESET      0xFF       /**< RESET command */

#define SRM_CMD_BYTE_INIT       0x00
#define SRM_PKT_CMD_FLAG        0x00       /**< INIT command */
#define SRM_PKT_BLOCK_LENGTH      0x20       /**< SENDFLAG command */

/*
** Rom Protocol enum
** ROM_PROTO_0: Initial version
** ROM_PROTO_1: Added Version info with IDLE status.
**              Protol added for Token transfer and SMS req bit location change
*/
typedef enum
{
    ROM_PROTO_UNKOWN,
    ROM_PROTO_0,
    ROM_PROTO_1,
    ROM_PROTO_2,
    ROM_PROTO_CNT,
} srm_protocol_ver;


/* srm functions */
int srm_get_status(unsigned char *status, unsigned char twi_cmd, unsigned char wait_state);
int srm_send_block(unsigned char *status, uint8_t *data, uint32_t final_bytes);
int srm_run(void);
int srm_reset(void);
int srm_write_byte(char twi_command);
uint8_t cxl_recovery_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end);
uint8_t cxl_do_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end);
int srm_verify(unsigned char twi_command, unsigned char status);
#endif /* _SRM_H */