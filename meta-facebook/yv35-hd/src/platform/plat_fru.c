#include <string.h>
#include "fru.h"
#include "plat_fru.h"
#include "hal_i2c.h"

const EEPROM_CFG plat_fru_config[] = {
	{
		NV_ATMEL_24C128,
		MB_FRU_ID,
		MB_FRU_PORT,
		MB_FRU_ADDR,
		FRU_DEV_ACCESS_BYTE,
		FRU_START,
		FRU_SIZE,
	},
	{
		NV_ATMEL_24C128,
		DPV2_FRU_ID,
		DPV2_FRU_PORT,
		DPV2_FRU_ADDR,
		FRU_DEV_ACCESS_BYTE,
		FRU_START,
		FRU_SIZE,
	},
};

const EEPROM_CFG plat_rb_fru_config[] = {
	{
		NV_ATMEL_24C128,
		RF_FRU_ID,
		RF_FRU_PORT,
		RF_FRU_ADDR,
		FRU_DEV_ACCESS_BYTE,
		FRU_START,
		FRU_SIZE,
	},
};

void pal_load_fru_config(void)
{
	memcpy(fru_config, plat_fru_config, sizeof(plat_fru_config));
	mux_fru_read();
	memcpy(fru_config, plat_rb_fru_config, sizeof(plat_rb_fru_config));
}

bool mux_fru_read(void)
{	
	uint8_t retry = 5;
	I2C_MSG msg;
	memset(&msg, 0, sizeof(I2C_MSG));
	msg.bus = RF_FRU_PORT;
	/* change address to 7-bit */
	msg.target_addr = (0xe2 >> 1);
	msg.tx_len = 1;
	msg.data[0] = 2;
	if (i2c_master_write(&msg, retry))
		return false;

	return true;
}
