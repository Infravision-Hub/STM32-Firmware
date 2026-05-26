#include "stm32c0xx.h"

void SystemInit(void)
{
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_1;
    RCC->CR &= ~RCC_CR_HSIDIV;
}

void __libc_init_array(void) {}

#define DBG_COUNTER (*(volatile uint32_t *)0x20000200)
#define DBG_TX_CNT  (*(volatile uint32_t *)0x20000204)
#define DBG_TXBTO   (*(volatile uint32_t *)0x20000208)
#define DBG_PSR     (*(volatile uint32_t *)0x2000020C)
#define DBG_CR      (*(volatile uint32_t *)0x20000210)

#define SRAMCAN  (0x4000B400UL)
#define TXF_BASE (SRAMCAN + 0x278U)
#define NODE_ID  42U

static void spin(volatile uint32_t n) { while (n--); }

int main(void)
{
    DBG_CR      = RCC->CR;
    DBG_COUNTER = 0;
    DBG_TX_CNT  = 0;
    DBG_TXBTO   = 0;

    RCC->APBENR1 |= RCC_APBENR1_FDCAN1EN; (void)RCC->APBENR1;
    RCC->APBRSTR1 |=  RCC_APBRSTR1_FDCAN1RST; (void)RCC->APBRSTR1;
    RCC->APBRSTR1 &= ~RCC_APBRSTR1_FDCAN1RST; (void)RCC->APBRSTR1;
    spin(500);

    RCC->CCIPR &= ~RCC_CCIPR_FDCAN1SEL;
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN; (void)RCC->IOPENR;

    GPIOB->PUPDR  = (GPIOB->PUPDR  & ~(3U<<0)) | (1U<<0);
    GPIOB->MODER  = (GPIOB->MODER  & ~(3U<<0)) | (2U<<0);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFU<<0)) | (3U<<0);
    GPIOB->MODER  = (GPIOB->MODER  & ~(3U<<2)) | (2U<<2);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFU<<4)) | (3U<<4);

    FDCAN1->CCCR &= ~FDCAN_CCCR_CSR; while(FDCAN1->CCCR & FDCAN_CCCR_CSA);
    FDCAN1->CCCR |= FDCAN_CCCR_INIT; while(!(FDCAN1->CCCR & FDCAN_CCCR_INIT));
    FDCAN1->CCCR |= FDCAN_CCCR_CCE;  while(!(FDCAN1->CCCR & FDCAN_CCCR_CCE));
    FDCAN_CONFIG->CKDIV = 0U;
    FDCAN1->CCCR &= ~(FDCAN_CCCR_TEST | FDCAN_CCCR_MON | FDCAN_CCCR_DAR);
    FDCAN1->NBTP  = (11U<<25)|(34U<<16)|(11U<<8)|(0U<<0);
    FDCAN1->TXBC  = 0U;
    FDCAN1->RXGFC = (2U<<4)|(2U<<2);
    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT; while(FDCAN1->CCCR & FDCAN_CCCR_INIT);

    static uint8_t tid = 0;
    volatile uint32_t ms = 0;
    volatile uint32_t last_tx = 0;

    while (1)
    {
        spin(48000);
        ms++;
        DBG_COUNTER = ms;
        DBG_PSR     = FDCAN1->PSR;

        if ((ms - last_tx) >= 100U)
        {
            last_tx = ms;

            if (!(FDCAN1->TXFQS & FDCAN_TXFQS_TFQF))
            {
                uint32_t pi = (FDCAN1->TXFQS & FDCAN_TXFQS_TFQPI_Msk) >> FDCAN_TXFQS_TFQPI_Pos;
                volatile uint32_t *tx = (volatile uint32_t *)(TXF_BASE + pi * 72U);

                /* 29-bit DroneCAN NodeStatus */
                tx[0] = (1U<<30) | ((341UL<<8) | NODE_ID);
                tx[1] = (8U<<16);
                tx[2] = ms;
                tx[3] = (uint32_t)(0xE0U | (tid++ & 0x1FU)) << 24;

                FDCAN1->TXBAR = (1U << pi);
                DBG_TX_CNT++;
                spin(48000);
                DBG_TXBTO = FDCAN1->TXBTO;
            }
        }
    }
}