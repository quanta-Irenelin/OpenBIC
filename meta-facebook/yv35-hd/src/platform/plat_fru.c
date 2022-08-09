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
	{
		NV_ATMEL_24C128,
		RF_FRU_ID,
		RF_FRU_PORT,
		RF_FRU_ADDR,
		FRU_DEV_ACCESS_BYTE,
		FRU_START,
		FRU_SIZE,
		RF_MUX_PRSNT,
		RF_FRU_MUX_ADDR,
		RF_FRU_MUX_CHAN,
	},
};

void pal_load_fru_config(void)
{
	memcpy(fru_config, plat_fru_config, sizeof(plat_fru_config));
}



