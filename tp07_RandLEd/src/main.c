/*
 * Gabarit de TP - NUCLEO-F303K8
 *
 * A completer :
 * 1. Copier ce dossier en tpXX-nom-du-tp/ (adapter TARGET dans le Makefile)
 * 2. Noter dans un README.md local les differences F103RB -> F303K8
 *    rencontrees dans ce TP (registres, broches, peripheriques)
 */

#include "stm32f3xx.h"
#include <stdbool.h>

/*****************************************************************
Preambule : indiquez ici les periopheriques que vous avez utilisez
*****************************************************************/

// GPIOB  : broche 3 pour controler la LED verte
// GPIOA : broche 0 pour detecter l'appui du bouton


/*****************************************************************
Declaration des fonctions��
*****************************************************************/
int rand(void);
void configure_gpio_pb3(void) ;
void configure_gpio_pa0(void) ;
void set_gpio(GPIO_TypeDef *GPIO, int n) ;
void reset_gpio(GPIO_TypeDef *GPIO, int n) ;
void configure_timer(TIM_TypeDef *TIM, int psc, int arr) ;
void configure_it(void) ;
void start_timer() ;
void stop_timer() ;

/*****************************************************************
Varibales globales
 *****************************************************************/
volatile bool Led_State = false;
/*****************************************************************
MAIN
*****************************************************************/

int main(void){
    RCC->AHBENR = RCC->AHBENR | RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	
    // Configuration des ports d'entree/sortie
	configure_gpio_pb3();
	configure_gpio_pa0();
    // Ecrire la suite du code
    configure_timer(TIM2, 7999, rand());
    configure_it();
    start_timer();
    
    // Boucle d'attente du processeur
	while (1){
        if (Led_State == true) {
            set_gpio(GPIOB, 3);
            
        }
        else {
            reset_gpio(GPIOB,3);
        }
    }
    
	return 0;
}

/*****************************************************************
Corps des fonctions
*****************************************************************/

/**
Configure la broche 3 du port B (led verte)
*/
void configure_gpio_pb3(void){
    GPIOB->MODER = GPIOB->MODER & ~(0x03 << 2*3); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOB->MODER = GPIOB->MODER | (0x01 << 2*3);

}

/**
Configure la broche 0 du port A (bouton USER) 
*/
void configure_gpio_pa0(void) {
    GPIOA->MODER = GPIOA->MODER & ~(0x03 << 2*0); // 0x03 et non 0xf car sur 2 bits et non 4 comme sur f103
    GPIOA->PUPDR = GPIOA->PUPDR | (0x01 << 2*0);

}

/**
Met a 1 la broche n du port GPIO
*/
void set_gpio(GPIO_TypeDef *GPIO, int n) {
    GPIO->ODR |= (0x01 << n);

}

/**
Met a 0 la broche n du port GPIO
*/
void reset_gpio(GPIO_TypeDef *GPIO, int n) {
    GPIO->ODR &= ~(0x01 << n);

}

/**
Configure la periode du timer TIM en fonction des parametres
psc (prescaler) et arr (autoreload) sans lancer le timer
*/
void configure_timer(TIM_TypeDef *TIM, int psc, int arr) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->ARR = arr;
    TIM2->PSC = psc;
    TIM2->DIER = TIM2->DIER | (1 << 0);

}

/**
Demarre le timer TIM
*/
void start_timer() {
    TIM2->CR1 |= TIM_CR1_CEN;

}

/**
Arrete le timer TIM
*/
void stop_timer() {
    TIM2->CR1 &= ~TIM_CR1_CEN;

}

/**
Configure toutes les interruptions du systeme
*/
void configure_it(void) {
    //activier l'intérruption 28 (interruption liée au tim 2 sur le cortex m3)
    NVIC->ISER[0] = NVIC->ISER[0] | (1 << 28);
    NVIC->IP[28] |= (7 << 4); //prioritée à 7
    //config interrupt sur pa0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;   // clear (0000 = PA, valeur par defaut)
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA; // = 0x0, donc redondant mais explicite
    EXTI->IMR |= EXTI_IMR_MR0; //Démasquer la ligne 0 côté EXTI (l'autoriser à générer une interruption)
    EXTI->FTSR |= EXTI_FTSR_FT0;   // interruption sur front descendant (appui)
    // EXTI->RTSR |= EXTI_RTSR_RT0; // decommenter aussi si tu veux detecter le relachement
    NVIC->ISER[0] |= (1 << 6);

}

/*****************************************************************
Fonctions d'interruption
*****************************************************************/
void TIM2_IRQHandler(void) { 
    TIM2->SR &= ~TIM_SR_UIF;
    Led_State = !Led_State;
    if (Led_State == true) {
        TIM2->ARR = 300;
    }
    else {
        TIM2->ARR = rand();
    }
    
    
    
}
void EXTI0_IRQHandler(void) {
    EXTI->PR |= EXTI_PR_PR0;   // ATTENTION : ici on ecrit un 1 pour EFFACER, pas un 0 !
    // ... ta logique ...
}

/*****************************************************************
Fonctions pre-definies
*****************************************************************/

/**
Retourne une valeur entiere aleatoire comprise entre 800 et 1800
*/
int rand(){
	static int randomseed = 0;
	randomseed = (randomseed * 9301 + 49297) % 233280;
	return 800 + (randomseed % 1000);
}

