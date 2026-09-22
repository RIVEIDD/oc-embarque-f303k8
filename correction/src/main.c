/*
 * correction - "Configurez un modulateur de longueur d'impulsion" (PWM)
 * (Partie 4, cours OpenClassrooms "Developpez en C pour l'embarque")
 *
 * ATTENTION : ce fichier a ete redige par Claude a la demande explicite
 * de l'utilisateur, pour servir de corrige de reference - PAS pour
 * remplacer l'exercice. Remplace l'ancien corrige "LED aleatoire"
 * (toujours recuperable dans l'historique git, commit dfc1ba2 / 5d2a032).
 *
 * Adapte depuis l'exemple complet donne dans le chapitre du cours
 * (PA6/TIM3_CH1, PWM 20 kHz, duty cycle balaye par IT de TIM2 toutes
 * les 100 ms), concu pour une NUCLEO-F103RB. Portage NUCLEO-F303K8 :
 *
 * 1. Broche PWM : PA6 existe aussi sur le connecteur du Nucleo-32 (label
 *    "A5"), et porte TIM3_CH1 sur le F303 comme sur le F103 - MAIS le
 *    mecanisme de selection est totalement different :
 *    - F103 : GPIOA->CRL, CNF=10 (AF push-pull) + MODE=10, valeur `0xA`
 *      sur le nibble de la broche - un seul champ combine direction+config,
 *      pas de choix explicite de quelle fonction alternative (l'AFIO_MAPR
 *      global gere les remaps).
 *    - F303 : GPIOA->MODER=10 (mode AF) sur la broche, PUIS
 *      GPIOA->AFR[0] doit recevoir le numero d'AF explicite. Pour
 *      TIM3_CH1 sur PA6, c'est **AF2** (verifie : le F303 expose TIM3_CH1
 *      sur PA6 OU PB4, selectionnable via ce numero d'AF - a
 *      revérifier dans le tableau d'alternate functions du datasheet
 *      STM32F303K8 si le comportement observe ne correspond pas).
 * 2. Toutes les autres macros de registres (RCC_APB1ENR_TIM3EN,
 *    TIM_CCMR1_OC1M_x, TIM_CCER_CC1E, TIM_CR1_CEN, TIM_DIER_UIE,
 *    TIM_SR_UIF) sont identiques nom pour nom entre F103 et F303 -
 *    verifie dans stm32f303x8.h. Seul le GPIO change de logique.
 * 3. `NVIC_ISER_SETENA_28` (cours) : macro absente de notre CMSIS,
 *    remplacee par `(1 << 28)` (meme piege que sur tp06/correction
 *    precedente - IRQ28 = TIM2, identique sur les deux puces).
 * 4. Frequences recalculees pour l'horloge REELLE actuelle de cette
 *    carte (HSI 8 MHz, pas de PLL configuree) plutot que de supposer
 *    les 72 MHz du F103 :
 *    - PWM 20 kHz : le cours utilise PSC=0, ARR=0xE0F (3599) a 72 MHz
 *      (72 000 000 / 3600 = 20 000 Hz). A 8 MHz, PSC=0 et ARR=399
 *      donnent le meme resultat (8 000 000 / 400 = 20 000 Hz).
 *    - Balayage du rapport cyclique toutes les 100 ms : le cours utilise
 *      ARR=999/PSC=7199 a 72 MHz. Ici, tick de 1 ms (PSC=7999, comme
 *      dans tp05/tp07/correction precedente) + ARR=99 donnent
 *      exactement 100 ms (100 ticks x 1 ms).
 * 5. Valeur initiale du rapport cyclique : le cours appelle
 *    `set_pulse_percentage(TIM3, 0x100)` - `0x100` = 256 en decimal,
 *    largement hors de la plage 0-100 % attendue par cette fonction
 *    (probable coquille du cours, `0x100` au lieu de `100`). Corrige ici
 *    en `50` (50 %) - une valeur valide, sans consequence sur le
 *    fonctionnement puisque l'IT de TIM2 recalcule le rapport cyclique
 *    des le premier debordement (100 ms) de toute facon.
 */

#include "stm32f3xx.h"

/*****************************************************************
Peripheriques utilises :
GPIOA, broche 6 (label "A5" sur le connecteur Nucleo-32) : sortie PWM,
    fonction alternative AF2 = TIM3_CH1
TIM3 : genere le signal PWM 20 kHz sur son canal 1
TIM2 : interruption toutes les 100 ms, balaye le rapport cyclique de
    TIM3 par pas de 5 % (boucle 0 -> 100 -> 0 -> ...)
*****************************************************************/

void configure_gpio_pa6_alternate_push_pull(void);
void configure_pwm_ch1_20khz(TIM_TypeDef *TIMER);
void start_timer(TIM_TypeDef *TIMER);
void set_pulse_percentage(TIM_TypeDef *TIMER, int pulse);
void configure_timer2_with_it(void);

int main(void)
{
    configure_gpio_pa6_alternate_push_pull();
    configure_pwm_ch1_20khz(TIM3);
    set_pulse_percentage(TIM3, 50);
    configure_timer2_with_it();
    start_timer(TIM3);
    start_timer(TIM2);

    while (1) {
    }

    return 0;
}

/*****************************************************************
Corps des fonctions
*****************************************************************/

/**
 * Configure PA6 en fonction alternative push-pull (AF2 = TIM3_CH1).
 */
void configure_gpio_pa6_alternate_push_pull(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    /* Mode fonction alternative (10) sur la broche 6 */
    GPIOA->MODER &= ~(0x03 << 2 * 6);
    GPIOA->MODER |= (0x02 << 2 * 6);

    /* AF2 (TIM3_CH1) dans AFR[0], nibble de la broche 6 */
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFRL6;
    GPIOA->AFR[0] |= (2U << GPIO_AFRL_AFRL6_Pos);
}

/**
 * Configure TIMER, canal 1, en PWM mode 1 a 20 kHz (sans le demarrer).
 */
void configure_pwm_ch1_20khz(TIM_TypeDef *TIMER)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIMER->PSC = 0;
    TIMER->ARR = 399; /* 8 MHz / 400 = 20 kHz */

    /* PWM mode 1 sur OC1 : OC1M = 110 */
    TIMER->CCMR1 &= ~TIM_CCMR1_OC1M_0;
    TIMER->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

    /* Active la sortie du canal 1 */
    TIMER->CCER |= TIM_CCER_CC1E;
}

/**
 * Demarre le timer TIMER.
 */
void start_timer(TIM_TypeDef *TIMER)
{
    TIMER->CR1 |= TIM_CR1_CEN;
}

/**
 * Regle le rapport cyclique de TIMER (canal 1) en pourcentage (0-100).
 */
void set_pulse_percentage(TIM_TypeDef *TIMER, int pulse)
{
    TIMER->CCR1 = TIMER->ARR * pulse / 100;
}

/**
 * Configure TIM2 pour interrompre toutes les 100 ms (sans le demarrer).
 */
void configure_timer2_with_it(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->PSC = 7999; /* tick de 1 ms sur l'horloge actuelle (8 MHz) */
    TIM2->ARR = 99;   /* 100 ticks x 1 ms = 100 ms */
    TIM2->DIER |= TIM_DIER_UIE;

    NVIC->ISER[0] |= (1 << 28); /* IRQ 28 = TIM2, cf. correction precedente */
    NVIC->IP[28] |= (7 << 4);
}

/*****************************************************************
Fonction d'interruption
*****************************************************************/

/* Toutes les 100 ms : avance le rapport cyclique de TIM3 par pas de 5 %,
   boucle de 0 a 100 puis repart a 0 (modulo 101 pour inclure 100) */
void TIM2_IRQHandler(void)
{
    static int pulse = 0;
    TIM2->SR &= ~TIM_SR_UIF;
    pulse = (pulse + 5) % 101;
    set_pulse_percentage(TIM3, pulse);
}
