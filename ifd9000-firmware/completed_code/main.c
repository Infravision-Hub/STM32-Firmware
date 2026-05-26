#include "stm32c0xx.h"

void SystemInit(void)
{
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_0;
    RCC->CR &= ~RCC_CR_HSIDIV;
    while (!(RCC->CR & RCC_CR_HSIRDY));
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE);
}

void __libc_init_array(void) {}

#define DBG_STATUS  (*(volatile uint32_t *)0x20000200)
#define DBG_EE_OK   (*(volatile uint32_t *)0x20000204)

#define U4_ADDR   0x28   /* Path 1 — J3  powered */
#define U13_ADDR  0x2B   /* Path 2 — J16 powered */

static void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms * 6000UL; i++)
        __asm volatile("nop");
}

static int i2c_write2(uint8_t addr, uint8_t b0, uint8_t b1)
{
    uint32_t t;
    t = 100000;
    while ((I2C1->ISR & I2C_ISR_BUSY) && --t);
    if (!t) return 0;
    I2C1->CR2 = ((uint32_t)(addr & 0x7F) << 1) | (2UL << 16) | I2C_CR2_START;
    t = 100000;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && !(I2C1->ISR & I2C_ISR_NACKF) && --t);
    if (!t || (I2C1->ISR & I2C_ISR_NACKF)) { I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF; return 0; }
    I2C1->TXDR = b0;
    t = 100000;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && !(I2C1->ISR & I2C_ISR_NACKF) && --t);
    if (!t || (I2C1->ISR & I2C_ISR_NACKF)) { I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF; return 0; }
    I2C1->TXDR = b1;
    t = 100000;
    while (!(I2C1->ISR & I2C_ISR_TC) && --t);
    I2C1->CR2 |= I2C_CR2_STOP;
    t = 100000;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && --t);
    I2C1->ICR = I2C_ICR_STOPCF;
    return 1;
}

static void eeprom_wait(uint8_t addr)
{
    for (int i = 0; i < 100; i++) {
        delay_ms(1);
        uint32_t t = 10000;
        while ((I2C1->ISR & I2C_ISR_BUSY) && --t);
        I2C1->CR2 = ((uint32_t)(addr & 0x7F) << 1)
                  | I2C_CR2_AUTOEND | I2C_CR2_START;
        t = 10000;
        while (!((I2C1->ISR & I2C_ISR_STOPF) | (I2C1->ISR & I2C_ISR_NACKF)) && --t);
        int acked = !(I2C1->ISR & I2C_ISR_NACKF);
        I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        if (acked) return;
    }
}

static int AD5144_StoreEEPROM(uint8_t addr, uint8_t rdac)
{
    int ok = i2c_write2(addr, 0x70 | (rdac & 0x03), 0x01);
    eeprom_wait(addr);
    return ok;
}

int main(void)
{
    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;
    (void)RCC->APBENR1;
    RCC->IOPENR  |= RCC_IOPENR_GPIOAEN;
    (void)RCC->IOPENR;

    RCC->APBRSTR1 |=  RCC_APBRSTR1_I2C1RST; (void)RCC->APBRSTR1;
    RCC->APBRSTR1 &= ~RCC_APBRSTR1_I2C1RST; (void)RCC->APBRSTR1;

    /* PA9 = I2C1_SCL (AF6), PA10 = I2C1_SDA (AF6) */
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFU<<4)|(0xFU<<8))) | ((6U<<4)|(6U<<8));
    GPIOA->OTYPER |= (1U<<9)|(1U<<10);
    GPIOA->PUPDR   = (GPIOA->PUPDR & ~((3U<<18)|(3U<<20))) | ((1U<<18)|(1U<<20));
    GPIOA->MODER   = (GPIOA->MODER & ~((3U<<18)|(3U<<20))) | ((2U<<18)|(2U<<20));

    I2C1->CR1     = 0U;
    I2C1->TIMINGR = 0x00B42F13U;
    I2C1->CR1     = I2C_CR1_PE;
    delay_ms(10);

    /* ── Step 1: Write wiper values ─────────────────────────────────
     * U4  (Path 1, 0x28): J5=12V, J7=12V, J8=12V, J4=8.5V
     * U13 (Path 2, 0x2B): J14=12V, J15=12V, J17=12V, J13=8.5V  */
    i2c_write2(U4_ADDR,  0x10, 0x2E);  /* RDAC1 → J5  Vout1_1 = 12V  */
    i2c_write2(U4_ADDR,  0x11, 0x39);  /* RDAC2 → J7  Vout2_1 = 12V  */
    i2c_write2(U4_ADDR,  0x12, 0x2E);  /* RDAC3 → J8  Vout3_1 = 12V  */
    i2c_write2(U4_ADDR,  0x13, 0x50);  /* RDAC4 → J4  8V5_1   = 8.5V */

    i2c_write2(U13_ADDR, 0x10, 0x2E);  /* RDAC1 → J14 Vout1_2 = 12V  */
    i2c_write2(U13_ADDR, 0x11, 0x39);  /* RDAC2 → J15 Vout2_2 = 12V  */
    i2c_write2(U13_ADDR, 0x12, 0x2E);  /* RDAC3 → J17 Vout3_2 = 12V  */
    i2c_write2(U13_ADDR, 0x13, 0x50);  /* RDAC4 → J13 8V5_2   = 8.5V */
    DBG_STATUS = 1;
    delay_ms(100);

    /* ── Step 2: Store both digipots to EEPROM ───────────────────── */
    uint32_t ok = 0;
    ok += AD5144_StoreEEPROM(U4_ADDR,  0);
    ok += AD5144_StoreEEPROM(U4_ADDR,  1);
    ok += AD5144_StoreEEPROM(U4_ADDR,  2);
    ok += AD5144_StoreEEPROM(U4_ADDR,  3);
    ok += AD5144_StoreEEPROM(U13_ADDR, 0);
    ok += AD5144_StoreEEPROM(U13_ADDR, 1);
    ok += AD5144_StoreEEPROM(U13_ADDR, 2);
    ok += AD5144_StoreEEPROM(U13_ADDR, 3);

    DBG_EE_OK  = ok;   /* expect 8 */
    DBG_STATUS = 2;    /* done     */

    while (1) { __asm volatile("nop"); }
}