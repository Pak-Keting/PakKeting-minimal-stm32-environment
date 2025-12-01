//#include "CMSIS/Device/ST/STM32F4xx/Include/stm32f411xe.h"
 #include "include/stm32f4xx.h"
 #include <stdint.h>

void SystemClock_Config(void)
{
    /* 1. Enable HSE */
    RCC->CR |= RCC_CR_HSEON;
    while(!(RCC->CR & RCC_CR_HSERDY));

    /* 2. Configure Flash wait states (0 WS for 0MHz < HCLK < 30MHz) */
    /* VERY IMPORTANT BIT HERE, IT'S THE REASON WHY IT DOESN'T WORK FOR ME THE FIRST TIME. */
    /* IF IT CONFIGURED WRONG AND OPERATING ON WRONG FREQUENCY RANGE, YOUR CODE WOULDN'T WORK */
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_0WS;

    /* PLL need to be configured before enabling it */
    /* 3. Configure PLL */
    RCC->PLLCFGR =
         (15 << RCC_PLLCFGR_PLLM_Pos)   |  // PLL_M = 15
        (144 << RCC_PLLCFGR_PLLN_Pos)   |  // PLL_N = 144
          (0 << RCC_PLLCFGR_PLLP_Pos)   |  // PLL_P = 2 (00b = /2, 01b = /4)
          (5 << RCC_PLLCFGR_PLLQ_Pos)   |  // PLL_Q = 5
          RCC_PLLCFGR_PLLSRC_HSE;          // HSE selected

    /* 4. Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    while(!(RCC->CR & RCC_CR_PLLRDY));

    /* 5. Set bus prescalers */
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;   // AHB  = 100 MHz
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV1;  // APB1 = 50 MHz
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;  // APB2 = 100 MHz

    /* 6. Switch SYSCLK to PLL */
    /* I DIDN'T USE PLL AS A SYSCLK IN THIS CASE */
    // RCC->CFGR |= RCC_CFGR_SW_PLL;
    // while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* 6. Switch SYSCLK to HSE */
    RCC->CFGR |= RCC_CFGR_SW_HSE;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSE);

    /* 7. Turn of the HSI */
    RCC->CR &= ~(RCC_CR_HSION);
}

void GPIOC_INIT() {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;  /* Enable GPIOC */
    GPIOC->MODER |= GPIO_MODER_MODER13_0; /* Change output MODE of C13 */
}

#define DELAY_S 2500000
int main() {
    SystemClock_Config();

    /* Set NOVBUSSENS because Black Pill doesn't have it */
    /* This is for USB, so if you planned to use it, keep this in mind */
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;

    GPIOC_INIT();

    /* I know is is probaly the most ugly loop you'll ever see, but */
    /* I've already burned out at this stage. I just wanted to see it blink */
    int i;
    while(1) {
        GPIOC->ODR |= GPIO_ODR_OD13; // Turn off LED
        for(i = 0; i<DELAY_S/4/2; i++) {
            __asm__("nop");
        }

        GPIOC->ODR &= ~(GPIO_ODR_OD13); // Turn on LED
        for(i = 0; i<DELAY_S/2; i++) {
            __asm__("nop");
        }
        i = 0;
    }

    return 0;
}