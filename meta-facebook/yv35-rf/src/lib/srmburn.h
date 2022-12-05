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
/* Uncomment to enable debug */
#define DBG_SRM_EN      0

/*
* Macros
*/

/* SRMBurn version */

#define SRMBURN_VERSION    01.00.01.00
#define CXL_RECOVER_ADDR               0x27
 
/* SMBus command set */
#define I2C_SMBUS_QUICK                0
#define I2C_SMBUS_BYTE                 1
#define I2C_SMBUS_BYTE_DATA            2
#define I2C_SMBUS_WORD_DATA            3
#define I2C_SMBUS_PROC_CALL            4
#define I2C_SMBUS_BLOCK_DATA           5
#define I2C_SMBUS_I2C_BLOCK_BROKEN     6
#define I2C_SMBUS_BLOCK_PROC_CALL      7
#define I2C_SMBUS_I2C_BLOCK_DATA       8

#define I2C_SMBUS_BLOCK_MAX     32         /**< Linux limits for block size */
#define I2C_SMBUS_READ          1
#define I2C_SMBUS_WRITE         0

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
#define SRM_CMD_BYTE_INIT   0x00       /**< INIT command */
#define SRM_CMD_BYTE_IMGWRITE   0xA1       /**< IMGWRITE command */
#define SRM_CMD_BYTE_VERIFY     0xA2       /**< VERIFY command */
#define SRM_CMD_BYTE_EXEC       0xA3       /**< EXEC command */
#define SRM_CMD_BYTE_RESET      0xFF       /**< EXEC command */

#define BLOCK_TX_LENGTH         32
#define MAX_FILE_NAME_LENGTH    32
#define MAX_I2C_DEV_FILES       16

#define WAIT_TIME_SEC   1
#define MAX_RETRY       3

#define FAIL   -1
#define SUCCESS 0

#define SMS_REQ_IDX_ROM_REV_A   3
#define SMS_REQ_BIT_MASK_REV_A  0x01
#define SMS_REQ_IDX_ROM_REV_B   2
#define SMS_REQ_BIT_MASK_REV_B  0x80
#define DBG_TOKEN_VAL_IDX       3
#define DBG_TOKEN_IDX_IDX       2
#define DBG_TOKEN_IDX_MASK      0x7F


/* length two debug tokens: static followed by ephemeral */
#define SRM_DEBUG_TOKEN_LEN (64 * 2)

/*
Data Structs
*/

typedef enum
{
    EFAIL = 1,
    EINVSLV,             /* Invalid id */
    EINVSTATE,           /* Invalid state */
    EFAILVERIFY,         /* Failed verification */
    EFAILSTATETRANS,     /* Fail state transition */
    EDEVNF,              /* Device not found */
    ENOFILE,             /* Invalid file path */
    ETOTAL

} SRM_ERROR;

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

union i2c_smbus_data {
    unsigned char byte;
    unsigned short word;
    unsigned char block[I2C_SMBUS_BLOCK_MAX + 2]; /* block[1] is used for length */
                                                  /* and one more for PEC */
};



/*
 * Funtion prototypes
*/

/* i2c/smbus write and read commands */
int i2c_smbus_access(int file, char read_write, unsigned char command,
                     int size, union i2c_smbus_data *data);
int i2c_smbus_read_byte(int file);
int i2c_smbus_write_byte(unsigned char value);

/* srm helper functions */
int srm_smbus_init(char filename[40], int addr);
int srm_get_status(unsigned char *status, unsigned char twi_cmd, unsigned char wait_state);
int srm_discover_busses(void);
int srm_send_image(char *image_path);
int srm_send_block(unsigned char *status, uint8_t *data);
int srm_run(void);
int srm_reset(void);
int srm_read_byte(void);
int srm_write_byte(char data);
uint8_t cxl_recovery_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end);
uint8_t cxl_do_update(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end);

#endif /* _SRM_H */