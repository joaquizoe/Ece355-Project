//
// This file is part of the GNU ARM Eclipse distribution.
// Copyright (c) 2014 Liviu Ionescu.
//

// ----------------------------------------------------------------------------
// School: University of Victoria, Canada.
// Course: ECE 355 "Microprocessor-Based Systems".
// This is template code for Part 2 of Introductory Lab.
//
// See "system/include/cmsis/stm32f051x8.h" for register/bit definitions.
// See "system/src/cmsis/vectors_stm32f051x8.c" for handler declarations.
// ----------------------------------------------------------------------------

#include <stdio.h>
#include "diag/Trace.h"
#include "cmsis/cmsis_device.h"
#include "stm32f051x8.h"

// ----------------------------------------------------------------------------
//
// STM32F0 empty sample (trace via $(trace)).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the $(trace) output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace_impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"


/* Definitions of registers and their bits are
   given in system/include/cmsis/stm32f051x8.h */


/* Clock prescaler for TIM2 timer: no prescaling */
#define myTIM2_PRESCALER ((uint16_t)0x0000)
/* Maximum possible setting for overflow */
#define myTIM2_PERIOD ((uint32_t)0xFFFFFFFF)

void myGPIOB_Init(void);
void myTIM2_Init(void);
void myEXTI_Init(void);

void myGPIOA_Init(void);


// Declare/initialize your global variables here...
// NOTE: You'll need at least one global variable
// (say, timerTriggered = 0 or 1) to indicate
// whether TIM2 has started counting or not.
volatile uint32_t timerOverflow = 0;

volatile uint8_t selectedSignal = 0;
volatile uint8_t firstEdge = 1;

/*** Call this function to boost the STM32F0xx clock to 48 MHz ***/

void SystemClock48MHz( void )
{
//
// Disable the PLL
//
    RCC->CR &= ~(RCC_CR_PLLON);
//
// Wait for the PLL to unlock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != 0 );
//
// Configure the PLL for 48-MHz system clock
//
    RCC->CFGR = 0x00280000;
//
// Enable the PLL
//
    RCC->CR |= RCC_CR_PLLON;
//
// Wait for the PLL to lock
//
    while (( RCC->CR & RCC_CR_PLLRDY ) != RCC_CR_PLLRDY );
//
// Switch the processor to the PLL clock source
//
    RCC->CFGR = ( RCC->CFGR & (~RCC_CFGR_SW_Msk)) | RCC_CFGR_SW_PLL;
//
// Update the system with the new clock frequency
//
    SystemCoreClockUpdate();

}

/*****************************************************************/


int
main(int argc, char* argv[])
  {

    SystemClock48MHz();

    trace_printf("This is Part 2 of Introductory Lab...\n");
    trace_printf("System clock: %u Hz\n", SystemCoreClock);

        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;  /* Enable SYSCFG clock */

    myGPIOA_Init();     /* Initialize USER button PA0 */
    myGPIOB_Init();     /* Initialize I/O port PB */
    myTIM2_Init();      /* Initialize timer TIM2 */
    myEXTI_Init();      /* Initialize EXTI */

    uint8_t previousSignal = selectedSignal;

    while (1)
    {
        // Nothing is going on here...

    }

    return 0;

}

void myGPIOA_Init()
{
    /* Enable clofk for GPIOA peripheral */
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN_Msk;

    /* Congigure PA0 as input */
    GPIOA->MODER &= ~GPIO_MODER_MODER0;

    /* Ensure no pull-up/pull-down for PA0 */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR0;

}

void myGPIOB_Init()
{
    /* Enable clock for GPIOB peripheral */
    // Relevant register: RCC->AHBENR

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN_Msk;

    /* Configure PB2 and PB3 as inputs */
    // Relevant register: GPIOB->MODER

    GPIOB->MODER &= ~(3U << 4);
    GPIOB->MODER &= ~GPIO_MODER_MODER3;

    /* Ensure no pull-up/pull-down for PB2 and PB3 */
    // Relevant register: GPIOB->PUPDR

    GPIOB->PUPDR &= ~(3U << 4);
    GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR3;
}


void myTIM2_Init()
{
    /* Enable clock for TIM2 peripheral */
    // Relevant register: RCC->APB1ENR

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Configure TIM2: buffer auto-reload, count up, stop on overflow,
     * enable update events, interrupt on overflow only */
    // Relevant register: TIM2->CR1

    TIM2->CR1 = ((uint16_t)0x008C);

    /* Set clock prescaler value */
    TIM2->PSC = myTIM2_PRESCALER;
    /* Set auto-reloaded delay */
    TIM2->ARR = myTIM2_PERIOD;

    /* Update timer registers */
    // Relevant register: TIM2->EGR

    TIM2->EGR = ((uint16_t)0x0001);
    TIM2->SR &= ~TIM_SR_UIF;

    /* Assign TIM2 interrupt priority = 0 in NVIC */
    // Relevant register: NVIC->IP[3], or use NVIC_SetPriority

    NVIC_SetPriority(TIM2_IRQn, 0);

    /* Enable TIM2 interrupts in NVIC */
    // Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ

    NVIC_EnableIRQ(TIM2_IRQn);

    /* Enable update interrupt generation */
    // Relevant register: TIM2->DIER

    TIM2->DIER |= TIM_DIER_UIE;

    /* Keep timer stopped until first rising edge */
    TIM2->CR1 &= ~TIM_CR1_CEN;
}

void myEXTI_Init()
{
    /* Map EXTI2 line to PB2 */
    // Relevant register: SYSCFG->EXTICR[0]

    SYSCFG->EXTICR[0] &= ~(0xFU << 8);
    SYSCFG->EXTICR[0] |= (0x1U << 8);

    /* EXTI2 line interrupts: set rising-edge trigger */
    // Relevant register: EXTI->RTSR

    EXTI->RTSR |= 0x4;
    EXTI->FTSR &= ~(1U << 2);

    /* Unmask interrupts from EXTI2 line */
    // Relevant register: EXTI->IMR

    EXTI->PR = EXTI_PR_PR2;
    EXTI->IMR |= 0x4;

    /* Assign EXTI2 interrupt priority = 0 in NVIC */
    // Relevant register: NVIC->IP[2], or use NVIC_SetPriority

    NVIC_SetPriority(EXTI2_3_IRQn, 0);

    /* Enable EXTI2 interrupts in NVIC */
    // Relevant register: NVIC->ISER[0], or use NVIC_EnableIRQ

    NVIC_EnableIRQ(EXTI2_3_IRQn);

    /* User button */

    /* Map EXTI0 line to PA0 */
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;

    /* EXTI0 line interrupts: set rising-edge trigger */
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->FTSR &= ~EXTI_FTSR_TR0;

    /* Unmask interrupts from EXTI0 line */
    EXTI->PR = EXTI_PR_PR0;
    EXTI->IMR |= EXTI_IMR_MR0;

    /* Assign EXTI0 interrupt priority = 0 in NVIC */
    NVIC_SetPriority(EXTI0_1_IRQn, 0);

    /* Enable EXTI0 interrupts in NVIC */
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    /* PB3 */

    /* Map EXTI3 line to PB3 */
    SYSCFG->EXTICR[0] &= ~(0xFU << 12);
    SYSCFG->EXTICR[0] |= (0x1U << 12);

    /* Configure rising-edge trigger */
    EXTI->RTSR |= EXTI_RTSR_TR3;
    EXTI->FTSR &= ~EXTI_FTSR_TR3;

    /* Clear pending flag */
    EXTI->PR = EXTI_PR_PR3;

    /* Initially disable EXTI3 */
    EXTI->IMR &= ~EXTI_IMR_MR3;
}


/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
void TIM2_IRQHandler()
{
    /* Check if update interrupt flag is indeed set */
    if ((TIM2->SR & TIM_SR_UIF) != 0)
    {
        trace_printf("\n*** Overflow! ***\n");

        /* Clear update interrupt flag */
        // Relevant register: TIM2->SR

        TIM2->SR &= ~TIM_SR_UIF;
        timerOverflow = 1;

        /* Restart stopped timer */
        // Relevant register: TIM2->CR1

        TIM2->CR1 |= TIM_CR1_CEN;
    }
}

/* This handler is declared in system/src/cmsis/vectors_stm32f051x8.c */
void EXTI2_3_IRQHandler()
{
    // Declare/initialize your local variables here...
    uint32_t activeLine;

    if (selectedSignal == 0)
    {
        activeLine = EXTI_PR_PR2;
    }
    else
    {
        activeLine = EXTI_PR_PR3;
    }

    /* Check if EXTI2 interrupt pending flag is indeed set */
    if ((EXTI->PR & activeLine) != 0)
    {
        //
        // 1. If this is the first edge:
        //  - Clear count register (TIM2->CNT).
        //  - Start timer (TIM2->CR1).
        //    Else (this is the second edge):
        //  - Stop timer (TIM2->CR1).
        //  - Read out count register (TIM2->CNT).
        //  - Calculate signal period and frequency.
        //  - Print calculated values to the console.
        //    NOTE: Function trace_printf does not work
        //    with floating-point numbers: you must use
        //    "unsigned int" type to print your signal
        //    period and frequency.
        //
        // 2. Clear EXTI2 interrupt pending flag (EXTI->PR).
        // NOTE: A pending register (PR) bit is cleared
        // by writing 1 to it.
        //

        /* Clear selected interrupt pending flag */
        EXTI->PR = activeLine; // CHANGED

        if (firstEdge == 1)
        {
            // First rising edge

        	// trace_printf("\n first rising edge");

            TIM2->CNT = 0;

            TIM2->CR1 |= TIM_CR1_CEN;

            timerOverflow = 0;
            TIM2->SR &= ~TIM_SR_UIF;

            firstEdge = 0;
        }
        else
        {
            // Second rising edge

            TIM2->CR1 &= ~TIM_CR1_CEN;

            uint32_t count = TIM2->CNT;

            //trace_printf("\n 2nd rising edge");

            if (!timerOverflow && count > 0)
            {
                float period_us = (count / 48.0);

                float freq = (float)48000000 / count;

                trace_printf(
                    "\nFrequency: %f Hz, Period: %f us",
                    freq, period_us);
            } else {
            	trace_printf("\nOverflow / zero count error!");
            }

            firstEdge = 1;
        }
    }
}

void EXTI0_1_IRQHandler()
{
    /* Check if the user button has been pressed */
    if((EXTI->PR & EXTI_PR_PR0) != 0)
    {
        /* Clear pending flag */
        EXTI->PR = EXTI_PR_PR0;

        /* Disable both signal interrupts */
        EXTI->IMR &= ~(EXTI_IMR_MR2 | EXTI_IMR_MR3);

        /* Stop and reset timer */
        TIM2->CR1 &= ~TIM_CR1_CEN;
        TIM2->CNT = 0;
        TIM2->SR &= ~TIM_SR_UIF;

        timerOverflow = 0;
        firstEdge = 1;

        /* Switches selected signal */
        selectedSignal ^= 1;

         /* Clear old pending flags */
        EXTI->PR = EXTI_PR_PR2 | EXTI_PR_PR3;

        if (selectedSignal == 0)
        {
            /* Enable function generator */
            EXTI->IMR |= EXTI_IMR_MR2;
        }
        else
        {
            /* Enable NE555 */
            EXTI->IMR |= EXTI_IMR_MR3;
        }
    }
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------