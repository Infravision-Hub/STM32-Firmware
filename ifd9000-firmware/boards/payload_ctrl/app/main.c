#include "stm32c0xx.h"

void SystemInit(void)
{
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_0;
    RCC->CR &= ~RCC_CR_HSIDIV;
    while (!(RCC->CR & RCC_CR_HSIRDY));
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE);
}

void __libc_init_array(void) {}

/* ── Debug slots at 0x20000200 ──────────────────────────────────────────────
   +0x00  DBG_VER       must show 0x00000013
   +0x04  DBG_STATUS    3=ADC ready, 1=I2C done, 2=EEPROM done
   +0x08  DBG_EE_OK     EEPROM write count
   +0x0C  DBG_MUX_CH    current mux channel
   +0x10  DBG_ADC_RAW   last raw value
   +0x14  DBG_ADC_MV    last millivolts (voltage) or milliamps (current)
   +0x18  DBG_ADC_ISR   ISR before ADSTART
   +0x1C  DBG_ADC_T     EOC countdown
   +0x20  DBG_ADC_CR    ADC->CR after ADEN
   +0x24  DBG_VM[0..5]  ARM voltage ch0-5 raw (6 x 4 bytes)
   +0x3C  DBG_IM[0..3]  ARM current ch0-3 raw (4 x 4 bytes, Im5/6 need A2) */
#define DBG_VER      (*(volatile uint32_t *)0x20000200)
#define DBG_STATUS   (*(volatile uint32_t *)0x20000204)
#define DBG_EE_OK    (*(volatile uint32_t *)0x20000208)
#define DBG_MUX_CH   (*(volatile uint32_t *)0x2000020C)
#define DBG_ADC_RAW  (*(volatile uint32_t *)0x20000210)
#define DBG_ADC_MV   (*(volatile uint32_t *)0x20000214)
#define DBG_ADC_ISR  (*(volatile uint32_t *)0x20000218)
#define DBG_ADC_T    (*(volatile uint32_t *)0x2000021C)
#define DBG_ADC_CR   (*(volatile uint32_t *)0x20000220)
static volatile uint32_t * const DBG_VM = (volatile uint32_t *)0x20000224;
static volatile uint32_t * const DBG_IM = (volatile uint32_t *)0x2000023C;

#define U4_ADDR   0x28
#define U13_ADDR  0x2B

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

static void mux1_select(uint8_t ch)
{
    if ((ch>>0)&1) GPIOA->BSRR = (1U<<4);  else GPIOA->BSRR = (1U<<20);
    if ((ch>>1)&1) GPIOA->BSRR = (1U<<5);  else GPIOA->BSRR = (1U<<21);
    if ((ch>>2)&1) GPIOA->BSRR = (1U<<6);  else GPIOA->BSRR = (1U<<22);
}

static void mux2_select(uint8_t ch)
{
    if ((ch>>0)&1) GPIOF->BSRR = (1U<<0);  else GPIOF->BSRR = (1U<<16);
    if ((ch>>1)&1) GPIOF->BSRR = (1U<<1);  else GPIOF->BSRR = (1U<<17);
}

static void all_enables_on(void)
{
    GPIOB->BSRR = (1U<<13)|(1U<<14)|(1U<<15);
    GPIOA->BSRR = (1U<<8);
    GPIOC->BSRR = (1U<<6)|(1U<<7);
    GPIOB->BSRR = (1U<<3)|(1U<<4)|(1U<<5)|(1U<<6);
}

static void all_enables_off(void)
{
    GPIOB->BSRR = (1U<<29)|(1U<<30)|(1U<<31);
    GPIOA->BSRR = (1U<<24);
    GPIOC->BSRR = (1U<<22)|(1U<<23);
    GPIOB->BSRR = (1U<<19)|(1U<<20)|(1U<<21)|(1U<<22);
}

static uint32_t adc_read_ch(uint8_t ch)
{
    uint32_t t;
    ADC1->CHSELR = (1U << ch);
    t = 100000;
    while (!(ADC1->ISR & ADC_ISR_CCRDY) && --t);
    ADC1->ISR = ADC_ISR_CCRDY;
    t = 100000;
    while ((ADC1->ISR & ADC_ISR_CCRDY) && --t);
    ADC1->ISR = ADC_ISR_EOC | ADC_ISR_OVR | ADC_ISR_EOSMP;
    DBG_ADC_ISR = ADC1->ISR;
    ADC1->CR |= ADC_CR_ADSTART;
    t = 500000;
    while (!(ADC1->ISR & ADC_ISR_EOC) && --t);
    DBG_ADC_T = t;
    if (!t) return 0xFFFFFFFF;
    return ADC1->DR & 0xFFF;
}

int main(void)
{
    DBG_VER = 0x15;

    uint32_t t;

    RCC->APBENR1 |= RCC_APBENR1_I2C1EN;   (void)RCC->APBENR1;
    RCC->APBENR2 |= RCC_APBENR2_ADCEN;    (void)RCC->APBENR2;
    RCC->IOPENR  |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN
                  | RCC_IOPENR_GPIOCEN  | RCC_IOPENR_GPIODEN
                  | RCC_IOPENR_GPIOFEN;
    (void)RCC->IOPENR;

    RCC->APBRSTR1 |=  RCC_APBRSTR1_I2C1RST; (void)RCC->APBRSTR1;
    RCC->APBRSTR1 &= ~RCC_APBRSTR1_I2C1RST; (void)RCC->APBRSTR1;
    GPIOA->AFR[1]  = (GPIOA->AFR[1] & ~((0xFU<<4)|(0xFU<<8))) | ((6U<<4)|(6U<<8));
    GPIOA->OTYPER |= (1U<<9)|(1U<<10);
    GPIOA->PUPDR   = (GPIOA->PUPDR & ~((3U<<18)|(3U<<20))) | ((1U<<18)|(1U<<20));
    GPIOA->MODER   = (GPIOA->MODER & ~((3U<<18)|(3U<<20))) | ((2U<<18)|(2U<<20));
    I2C1->CR1      = 0U;
    I2C1->TIMINGR  = 0x00B42F13U;
    I2C1->CR1      = I2C_CR1_PE;
    delay_ms(10);

    GPIOA->MODER  = (GPIOA->MODER  & ~(3U<<16)) | (1U<<16);
    GPIOA->OTYPER &= ~(1U<<8);
    GPIOA->PUPDR  &= ~(3U<<16);

    GPIOB->MODER  = (GPIOB->MODER  & ~((3U<<6)|(3U<<8)|(3U<<10)|(3U<<12)
                                       |(3U<<26)|(3U<<28)|(3U<<30)))
                                   | ((1U<<6)|(1U<<8)|(1U<<10)|(1U<<12)
                                      |(1U<<26)|(1U<<28)|(1U<<30));
    GPIOB->OTYPER &= ~((1U<<3)|(1U<<4)|(1U<<5)|(1U<<6)
                      |(1U<<13)|(1U<<14)|(1U<<15));
    GPIOB->PUPDR  &= ~((3U<<6)|(3U<<8)|(3U<<10)|(3U<<12)
                      |(3U<<26)|(3U<<28)|(3U<<30));

    GPIOC->MODER  = (GPIOC->MODER  & ~((3U<<12)|(3U<<14))) | ((1U<<12)|(1U<<14));
    GPIOC->OTYPER &= ~((1U<<6)|(1U<<7));
    GPIOC->PUPDR  &= ~((3U<<12)|(3U<<14));

    GPIOA->MODER  = (GPIOA->MODER  & ~((3U<<8)|(3U<<10)|(3U<<12)))
                                    | ((1U<<8)|(1U<<10)|(1U<<12));
    GPIOA->OTYPER &= ~((1U<<4)|(1U<<5)|(1U<<6));
    GPIOA->PUPDR  &= ~((3U<<8)|(3U<<10)|(3U<<12));

    GPIOF->MODER  = (GPIOF->MODER  & ~((3U<<0)|(3U<<2))) | ((1U<<0)|(1U<<2));
    GPIOF->OTYPER &= ~((1U<<0)|(1U<<1));
    GPIOF->PUPDR  &= ~((3U<<0)|(3U<<2));

    GPIOA->MODER |=  (3U<<0) | (3U<<2);
    GPIOA->PUPDR &= ~((3U<<0) | (3U<<2));

    all_enables_off();

    ADC1->CR    = 0;
    ADC1->CFGR2 = (3U << 30);
    ADC1->CFGR1 = 0;
    ADC1->SMPR  = 7;
    ADC1->ISR   = 0x1F;
    ADC1->CR    = ADC_CR_ADEN;
    t = 1000000;
    while (!(ADC1->ISR & ADC_ISR_ADRDY) && --t);
    DBG_ADC_CR = ADC1->CR;
    ADC1->CHSELR = (1U << 0);
    t = 1000000;
    while (!(ADC1->ISR & ADC_ISR_CCRDY) && --t);
    ADC1->ISR = ADC_ISR_CCRDY;
    t = 100000;
    while ((ADC1->ISR & ADC_ISR_CCRDY) && --t);
    DBG_STATUS = 3;

    i2c_write2(U4_ADDR,  0x10, 0x2E);
    i2c_write2(U4_ADDR,  0x11, 0x39);
    i2c_write2(U4_ADDR,  0x12, 0x2E);
    i2c_write2(U4_ADDR,  0x13, 0x50);
    i2c_write2(U13_ADDR, 0x10, 0x2E);
    i2c_write2(U13_ADDR, 0x11, 0x39);
    i2c_write2(U13_ADDR, 0x12, 0x2E);
    i2c_write2(U13_ADDR, 0x13, 0x50);
    DBG_STATUS = 1;
    delay_ms(100);

    uint32_t ok = 0;
    ok += AD5144_StoreEEPROM(U4_ADDR,  0);
    ok += AD5144_StoreEEPROM(U4_ADDR,  1);
    ok += AD5144_StoreEEPROM(U4_ADDR,  2);
    ok += AD5144_StoreEEPROM(U4_ADDR,  3);
    ok += AD5144_StoreEEPROM(U13_ADDR, 0);
    ok += AD5144_StoreEEPROM(U13_ADDR, 1);
    ok += AD5144_StoreEEPROM(U13_ADDR, 2);
    ok += AD5144_StoreEEPROM(U13_ADDR, 3);
    DBG_EE_OK  = ok;
    DBG_STATUS = 2;

    /* ── 7. Main loop — mux1 ch0+1 (voltage), mux2 ch0+1 (current) ────── */
    while (1)
    {
        all_enables_on();

        /* Mux1 ch0 = ARM_Vm1 */
        mux1_select(0);
        DBG_MUX_CH = 0x00;
        delay_ms(10);
        uint32_t raw = adc_read_ch(0);
        DBG_VM[0] = raw;
        delay_ms(2000);

        /* Mux1 ch1 = ARM_Vm2 */
        mux1_select(1);
        DBG_MUX_CH = 0x01;
        delay_ms(10);
        raw = adc_read_ch(0);
        DBG_VM[1] = raw;
        delay_ms(2000);

        /* Mux2 ch0 = ARM_Im1 */
        mux2_select(0);
        DBG_MUX_CH = 0x10;
        delay_ms(10);
        raw = adc_read_ch(1);
        DBG_IM[0] = raw;
        delay_ms(2000);

        /* Mux2 ch1 = ARM_Im2 */
        mux2_select(1);
        DBG_MUX_CH = 0x11;
        delay_ms(10);
        raw = adc_read_ch(1);
        DBG_IM[1] = raw;
        delay_ms(2000);

        all_enables_off();
        delay_ms(2000);
    }
}