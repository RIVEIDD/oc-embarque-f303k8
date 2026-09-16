/*
 * Gabarit de TP - NUCLEO-F303K8
 *
 * A completer :
 * 1. Copier ce dossier en tpXX-nom-du-tp/ (adapter TARGET dans le Makefile)
 * 2. Noter dans un README.md local les differences F103RB -> F303K8
 *    rencontrees dans ce TP (registres, broches, peripheriques)
 */

#include "stm32f3xx.h"

void TIM2_IRQHandler(void) { 
    TIM2->SR &= ~TIM_SR_UIF;
    GPIOB->ODR = GPIOB->ODR ^ (1 << 3);
}

int main(void)
{
    // Configuration de la broche en sortie
    //RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->AHBENR = RCC->AHBENR | RCC_AHBENR_GPIOBEN;
    // config led en sortie
    GPIOB->MODER = GPIOB->MODER & ~(0x03 << 2*3); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOB->MODER = GPIOB->MODER | (0x01 << 2*3);
    
    // Configuration du timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->ARR = 9999;
    TIM2->PSC = 7199;
    TIM2->DIER = TIM2->DIER | (1 << 0);

    //activier l'intérruption 28 (interruption liée au tim 2 sur le cortex m3)
    NVIC->ISER[0] = NVIC->ISER[0] | (1 << 28);
    NVIC->IP[28] |= (7 << 4); //prioritée à 7

    // Lancement du timer
    TIM2->CR1 |= TIM_CR1_CEN;
    while (1) {
        /* TODO */
    }
}
