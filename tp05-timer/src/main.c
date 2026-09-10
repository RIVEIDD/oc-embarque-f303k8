/*
 * Gabarit de TP - NUCLEO-F303K8
 *
 * A completer :
 * 1. Copier ce dossier en tpXX-nom-du-tp/ (adapter TARGET dans le Makefile)
 * 2. Noter dans un README.md local les differences F103RB -> F303K8
 *    rencontrees dans ce TP (registres, broches, peripheriques)
 */

#include "stm32f3xx.h"

int main(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->ARR = 9999;
    TIM2->PSC = 7199;
    TIM2->CR1 = TIM2->CR1 | ( 1 << 0);
    while (1) {
        /* TODO */
    }
}
