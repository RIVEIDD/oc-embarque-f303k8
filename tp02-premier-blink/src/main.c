/*
 * Gabarit de TP - NUCLEO-F303K8
 *
 * A completer :
 * 1. Copier ce dossier en tpXX-nom-du-tp/ (adapter TARGET dans le Makefile)
 * 2. Noter dans un README.md local les differences F103RB -> F303K8
 *    rencontrees dans ce TP (registres, broches, peripheriques)
 */

#include "stm32f3xx.h"

static void delay(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

int main (void) 
{
    RCC->AHBENR |=  RCC_AHBENR_GPIOBEN ;
    GPIOB->MODER = GPIOB->MODER & ~(0x03 << 2*3); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOB->MODER = GPIOB->MODER | (0x01 << 2*3);
    while(1) 
    { 
        GPIOB->ODR ^= (1 << 3);
        delay(400000);
    } 
    return 0; 
}
