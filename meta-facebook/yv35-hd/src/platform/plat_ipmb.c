#include <stdio.h>
#include <string.h>
#include "plat_ipmb.h"
#include "plat_class.h"

IPMB_config pal_IPMB_config_table[] = {
	// index, interface, channel, bus, channel_target_address, enable_status, self_address,
	// rx_thread_name, tx_thread_name
	{ BMC_IPMB_IDX, I2C_IF, BMC_IPMB, IPMB_I2C_BMC, BMC_I2C_ADDRESS, ENABLE, SELF_I2C_ADDRESS,
	  "RX_BMC_IPMB_TASK", "TX_BMC_IPMB_TASK" },
	{ EXP1_IPMB_IDX, I2C_IF, EXP1_IPMB, IPMB_EXP1_BUS, BIC1_I2C_ADDRESS, DISABLE,
	  SELF_I2C_ADDRESS, "RX_EPX1_IPMB_TASK", "TX_EXP1_IPMB_TASK" },
	{ EXP2_IPMB_IDX, I2C_IF, EXP2_IPMB, IPMB_EXP2_BUS, BIC2_I2C_ADDRESS, DISABLE,
	  SELF_I2C_ADDRESS, "RX_EPX2_IPMB_TASK", "TX_EXP2_IPMB_TASK" },
	{ RESERVED_IDX, RESERVED_IF, RESERVED, RESERVED_BUS, RESERVED_ADDRESS, DISABLE,
	  RESERVED_ADDRESS, "RESERVED_ATTR", "RESERVED_ATTR" },
};

bool pal_load_ipmb_config(void)
{
	CARD_STATUS _1ou_status = get_1ou_status();
	if (_1ou_status.present) {
		pal_IPMB_config_table[EXP1_IPMB_IDX].enable_status = ENABLE;
	}

	CARD_STATUS _2ou_status = get_2ou_status();
	if (_2ou_status.present) {
		// for dpv2 sku, disable ipmb and set i2c freq to 400Khz for slave devices reading
		// for reset of expansion board, enable ipmb and set i2c freq to 1Mhz
		if ((_2ou_status.card_type & TYPE_2OU_DPV2_16) == TYPE_2OU_DPV2_16) {
			i2c_freq_set(pal_IPMB_config_table[EXP2_IPMB_IDX].bus, I2C_SPEED_FAST);
			pal_IPMB_config_table[EXP2_IPMB_IDX].enable_status = DISABLE;
		} else {
			pal_IPMB_config_table[EXP2_IPMB_IDX].enable_status = ENABLE;
		}
	}

	memcpy(IPMB_config_table, pal_IPMB_config_table, sizeof(pal_IPMB_config_table));
	return true;
}
