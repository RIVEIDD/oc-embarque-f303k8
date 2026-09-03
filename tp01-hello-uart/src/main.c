/*
 * tp01-hello-uart - NUCLEO-F303K8
 *
 * Envoie "Hello, world!" en boucle sur USART2 (PA2 = TX, AF7), cablee
 * en interne au ST-LINK -> port serie virtuel cote PC. Sert a valider
 * que la chaine complete (build/flash/GPIO/RCC/USART) fonctionne sur
 * la carte reelle.
 *
 * Ecrit exceptionnellement par Claude a la demande explicite de
 * l'utilisateur, pour valider l'environnement de travail avant de
 * reprendre les TP en autonomie.
 *
 * Horloge : au reset, SystemInit() ne configure aucune PLL -> HSI 8 MHz,
 * donc APB1 = 8 MHz (pas encore le 72 MHz max du F303K8).
 */

#include "stm32f3xx.h"

#define USART2_BAUDRATE 115200UL
#define APB1_CLK_HZ     8000000UL

static void usart2_init(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 en fonction alternative (MODER = 10), AF7 = USART2_TX */
    GPIOA->MODER  &= ~GPIO_MODER_MODER2;
    GPIOA->MODER  |=  GPIO_MODER_MODER2_1;
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL2;
    GPIOA->AFR[0] |=  (7U << GPIO_AFRL_AFRL2_Pos);

    /* BRR = clock APB1 / baudrate (mode oversampling x16, valeur par defaut) */
    USART2->BRR = APB1_CLK_HZ / USART2_BAUDRATE;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void usart2_send_char(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE)) {
    }
    USART2->TDR = (uint8_t)c;
}

static void usart2_send_string(const char *s)
{
    while (*s != '\0') {
        usart2_send_char(*s++);
    }
}

static void delay(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

int main(void)
{
    usart2_init();

    while (1) {
        usart2_send_string("Hello, world!\r\n");
        delay(1000000);
    }
}
