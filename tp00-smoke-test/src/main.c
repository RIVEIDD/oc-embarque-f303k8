/*
 * tp00-smoke-test : verification de la chaine de build/flash (pas un TP du cours)
 *
 * Fait clignoter LD3 (LED verte utilisateur de la NUCLEO-F303K8), sur PB3.
 * Sur la NUCLEO-F103RB du cours, LD2 est sur PA5 (bus APB2) : ici GPIOB
 * est sur le bus AHB (RCC->AHBENR), pas APB2 comme sur le F103.
 */

#include "stm32f3xx.h"

static void delay(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

int main(void)
{
    /* 1. Horloge du port B */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    /* 2. PB3 en sortie push-pull (MODER = 01) */
    GPIOB->MODER &= ~GPIO_MODER_MODER3;
    GPIOB->MODER |=  GPIO_MODER_MODER3_0;

    while (1) {
        GPIOB->ODR ^= GPIO_ODR_3;
        delay(400000);
    }
}
