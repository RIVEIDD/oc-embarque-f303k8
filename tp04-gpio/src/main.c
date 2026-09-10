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
    RCC->AHBENR = RCC->AHBENR | RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    // config led en sortie
    GPIOB->MODER = GPIOB->MODER & ~(0x03 << 2*3); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOB->MODER = GPIOB->MODER | (0x01 << 2*3);

    // config pa0 (bonton joystick) en entrée
    GPIOA->MODER = GPIOA->MODER & ~(0x03 << 2*0); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOA->PUPDR = GPIOA->PUPDR | (0x01 << 2*0);

    //variable de stockage d'état de la led
    int state = 0;
    while (1) {
        while (1) {
            if (state != GPIOA->IDR & (1 << 0)){ // erreur réalise : reprendre le code du cours sans changer ni le gpio (ici a au lieu de c) ni le registre (ici 0 au lieu de 13)
                GPIOB->ODR = GPIOB->ODR ^ (1 << 3);
            }
            state =GPIOA->IDR & (1 << 0); 
        }
    }
}
