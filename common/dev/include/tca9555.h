#ifndef __tca9555__
#define __tca9555__

#include <stdint.h>

// Refer to Table3 Command byte
typedef enum
{   
    /* Read byte Protocol*/ 
    TCA9555_INPUT_PORT0 = 0x00, 
    TCA9555_INPUT_PORT1 = 0x01,

    /* Read-write Protocol   */ 
    /* default val: 11111111 */
    TCA9555_OUTPUT_PORT0 = 0x02, //config gpio I/O status
    TCA9555_OUTPUT_PORT1 = 0x03, 
    TCA9555_CONFIG_PORT0 = 0x06, //config gpio direction
    TCA9555_CONFIG_PORT1 = 0x07,
    TCA9555_REG_ADDR_MAX

} tca9555_reg_addr_t;

/**
 * @brief I2C slave address
 */
typedef enum {
    TCA9555_ADDR_A2L_A1L_A0L = 0x20,
    TCA9555_ADDR_A2L_A1L_A0H,
    TCA9555_ADDR_A2L_A1H_A0L,
    TCA9555_ADDR_A2L_A1H_A0H,
    TCA9555_ADDR_A2H_A1L_A0L,
    TCA9555_ADDR_A2H_A1L_A0H,
    TCA9555_ADDR_A2H_A1H_A0L,
    TCA9555_ADDR_A2H_A1H_A0H,
    TCA9555_ADDR_MAX
} tca9555_addr_t;

/**
 * @brief TCA9555 GPIO mode
 */
typedef enum {
    TCA9555_GPIO_OUTPUT = 0,
    TCA9555_GPIO_INPUT = 1
} tca9555_gpio_mode_t;


typedef enum {
    P1V8_ASIC_PG_R = 0,
    P0V8_ASICA_PWRGD,
    P0V8_ASICD_PWRGD,
    P0V9_ASICA_PWRGD,
    PWRGD_PVDDQ_AB,
    PVPP_AB_PG_R,
    PVTT_AB_PG_R,
    TCA9555_GPIO_P07,
    PWRGD_PVDDQ_CD,
    PVPP_CD_PG_R,
    PVTT_CD_PG_R,
    P0V9_ASICA_FT_R,
    P0V8_ASICD_FT_R,
    P0V8_ASICA_FT_R,
    P5V_STBY_PG,
    TCA9555_GPIO_P17,
    TCA9555_GPIO_MAX
} tca9555_gpio_pin_t2;


#endif
