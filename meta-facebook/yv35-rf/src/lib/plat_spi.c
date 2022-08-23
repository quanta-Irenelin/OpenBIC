#include <stdio.h>
#include <plat_power_seq.h>

#include "hal_gpio.h"
#include "util_spi.h"
#include "plat_gpio.h"

#define CXL_FLASH_TO_BIC 1
#define CXL_FLASH_TO_CXL 0

#define CXL_UPDATE_MAX_OFFSET 0x2000000

static bool pal_switch_cxl_spi_mux(int gpio_status)
{
	if (gpio_status != CXL_FLASH_TO_BIC && gpio_status != CXL_FLASH_TO_CXL)
	{
		printf("[%s] Invalid argument\n", __func__);
	}

	return gpio_set(SPI_MASTER_SEL, gpio_status);
}

static bool control_flash_power(int power_state)
{
	int control_mode = 0;

	switch (power_state)
	{
	case POWER_OFF:
		control_mode = DISABLE_POWER_MODE;
		break;
	case POWER_ON:
		control_mode = ENABLE_POWER_MODE;
		break;
	default:
		return false;
	}

	for (int retry = 3; retry > 0; retry--)
	{
		if (gpio_get(P1V8_ASIC_PG_R) == power_state) {
			return true;
		}

		control_power_stage(control_mode, P1V8_ASIC_EN_R);
		k_msleep(CHKPWR_DELAY_MSEC);
	}

	return false;
}

uint8_t fw_update_cxl(uint32_t offset, uint16_t msg_len, uint8_t *msg_buf, bool sector_end)
{
	uint8_t ret = FWUPDATE_UPDATE_FAIL;

	if (offset > CXL_UPDATE_MAX_OFFSET) {
		return FWUPDATE_OVER_LENGTH;
	}

	// Enable the P1V8_ASCI to power the flash
	if (!control_flash_power(POWER_ON)){
		return FWUPDATE_UPDATE_FAIL;
	}

	// Set high to choose the BIC as the host
	if (pal_switch_cxl_spi_mux(CXL_FLASH_TO_BIC)) {
		return FWUPDATE_UPDATE_FAIL;
	}

	ret = fw_update(offset, msg_len, msg_buf, sector_end, DEVSPI_SPI1_CS0);

	if ((offset + msg_len) == CXL_UPDATE_MAX_OFFSET || ret != FWUPDATE_SUCCESS)
	{
		if (!control_flash_power(POWER_OFF)){
			return FWUPDATE_UPDATE_FAIL;
		}

		if (gpio_get(CONTROL_POWER_SEQ_04) != POWER_OFF) {
			printf("Fail to disable the ASIC_1V8\n");
		}

		// Release the control of the flash SPI bus
		if (pal_switch_cxl_spi_mux(CXL_FLASH_TO_CXL)) {
			printf("Switch to PIONEER failed\n");
		}
	}

	return ret;
}
