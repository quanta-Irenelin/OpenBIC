#include "hal_gpio.h"
#include "plat_gpio.h"
#include "expansion_board.h"
#include "plat_isr.h"
#include "plat_power_seq.h"
#include "power_status.h"
#include <stdio.h>

SCU_CFG scu_cfg[] = {
	//register    value
	{ 0x7e6e2610, 0x0000D7BF },
	{ 0x7e6e2614, 0x00044300 },
};

void pal_pre_init()
{
	init_platform_config();
	scu_init(scu_cfg, sizeof(scu_cfg) / sizeof(SCU_CFG));
}
void pal_set_sys_status()
{
	set_MB_DC_status(FM_POWER_EN);
	set_DC_status(PWRGD_CARD_PWROK);
	if(gpio_get(PWRGD_CARD_PWROK) == GPIO_HIGH){
		set_DC_on_delayed_status();
	}
	control_power_sequence();
	while(1){
		int gpio_stat = gpio_get(PWRGD_CARD_PWROK);
		printf("Gpio 33 is %d\n", gpio_stat);
		k_msleep(1000);
	}
}

void pal_post_init()
{
	k_usleep(100);

	gpio_set(ASIC_DEV_RST_N, GPIO_HIGH);
	printf("post init ASIC_DEV_RST_N set high\n");
}

#define DEF_PROJ_GPIO_PRIORITY 61

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME", &gpio_init, NULL, NULL, NULL,
	      POST_KERNEL, DEF_PROJ_GPIO_PRIORITY, NULL);
