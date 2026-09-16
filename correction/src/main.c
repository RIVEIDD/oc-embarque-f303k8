/*
 * correction - "Entrainez-vous en allumant une LED de maniere aleatoire"
 * (Partie 3, cours OpenClassrooms "Developpez en C pour l'embarque")
 *
 * ATTENTION : ce fichier a ete redige par Claude a la demande explicite
 * de l'utilisateur, pour servir de corrige de reference une fois qu'il
 * aura ecrit sa propre version - PAS pour remplacer l'exercice.
 *
 * Adapte depuis le corrige officiel du cours
 * (main_v1_correction.c, telecharge depuis static.oc-static.com),
 * concu pour une NUCLEO-F103RB. Portage NUCLEO-F303K8 :
 *
 * 1. LED : PA5 (LD2, F103) -> PB3 (LD3, F303K8). Horloge sur
 *    RCC->AHBENR/GPIOBEN (pas APB2ENR/IOPAEN), config via MODER 2 bits
 *    (pas CRL 4 bits).
 * 2. Bouton PC13 (config presente mais JAMAIS utilisee dans cette
 *    version "v1" du corrige officiel - vestige pour une "v2" avec jeu
 *    de reactivite) : retiree entierement ici, PC13 n'existe meme pas
 *    sur le connecteur du Nucleo-32 (cf. docs/correspondance-f103-f303.md).
 * 3. TIM4 (active dans l'original mais jamais utilise en v1, meme
 *    remarque que PC13) : retire. De toute facon absent du F303K8
 *    (cf. docs/correspondance-f103-f303.md §5).
 * 4. NVIC_ISER_SETENA_28/29 : macros absentes de notre CMSIS -
 *    remplacees par les decalages de bit bruts (1 << 28)/(1 << 29).
 *    IRQ28=TIM2 et IRQ29=TIM3 verifies identiques au F103 dans
 *    stm32f303x8.h.
 * 5. Horloge/PSC recalcules pour notre horloge REELLE actuelle
 *    (HSI 8 MHz, pas de PLL configuree - cf. notes de cours Partie 2/3) :
 *    le cours suppose 72 MHz (PSC=7199 -> tick 100us, d'ou le "10*rand()"
 *    pour convertir des ms en dixiemes de ms). Ici, PSC=7999 avec une
 *    horloge a 8 MHz donne un tick de EXACTEMENT 1 ms
 *    (8 000 000 / 8000 = 1000 Hz), ce qui supprime le besoin du
 *    multiplicateur "10*" : rand() renvoie deja directement des
 *    millisecondes, ARR = rand() (ou 300 pour le delai fixe) suffit.
 *    Resultat : le comportement reel (300 ms allume, 800-1800 ms
 *    eteint) est correct sur cette carte telle quelle, sans devoir
 *    configurer le PLL a 72 MHz au prealable.
 */

#include "stm32f3xx.h"

/*****************************************************************
Peripheriques utilises :
GPIOB, broche 3 : pilote LD3 (LED verte utilisateur du F303K8)
TIM2 : chronometre les 300 ms (LED allumee)
TIM3 : chronometre l'intervalle aleatoire avant d'allumer la LED
*****************************************************************/

int rand(void);
void configure_gpio_pb3(void);
void set_gpio(GPIO_TypeDef *GPIO, int n);
void reset_gpio(GPIO_TypeDef *GPIO, int n);
void configure_timer(TIM_TypeDef *TIM, int psc, int arr);
void configure_it(void);
void start_timer(TIM_TypeDef *TIM);
void stop_timer(TIM_TypeDef *TIM);

int main(void)
{
    /* Configuration du port de sortie (LED) */
    configure_gpio_pb3();

    /* Configuration des timers (horloge + periode, sans les demarrer) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;
    configure_timer(TIM2, 7999, 300);
    configure_timer(TIM3, 7999, rand());

    /* Configuration des interruptions */
    configure_it();

    /* Demarrage du premier timer (attente aleatoire avant 1er allumage) */
    start_timer(TIM3);

    /* Boucle d'attente du processeur - tout se passe dans les IT */
    while (1) {
    }

    return 0;
}

/*****************************************************************
Corps des fonctions
*****************************************************************/

/**
 * Configure la broche 3 du port B (LD3, LED verte du F303K8)
 * en sortie push-pull.
 */
void configure_gpio_pb3(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(0x03 << 2 * 3);
    GPIOB->MODER |= (0x01 << 2 * 3);
}

/**
 * Met a 1 la sortie de la broche n du port GPIO.
 */
void set_gpio(GPIO_TypeDef *GPIO, int n)
{
    GPIO->ODR |= (0x01 << n);
}

/**
 * Met a 0 la sortie de la broche n du port GPIO.
 */
void reset_gpio(GPIO_TypeDef *GPIO, int n)
{
    GPIO->ODR &= ~(0x01 << n);
}

/**
 * Configure la periode du timer TIM (psc/arr) sans le demarrer.
 */
void configure_timer(TIM_TypeDef *TIM, int psc, int arr)
{
    TIM->ARR = arr;
    TIM->PSC = psc;
}

/**
 * Demarre le timer TIM.
 */
void start_timer(TIM_TypeDef *TIM)
{
    TIM->CR1 |= TIM_CR1_CEN;
}

/**
 * Arrete le timer TIM.
 */
void stop_timer(TIM_TypeDef *TIM)
{
    TIM->CR1 &= ~TIM_CR1_CEN;
}

/**
 * Configure toutes les interruptions du systeme.
 */
void configure_it(void)
{
    /* Interruption de TIM2 (IRQ 28, identique au F103) */
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1 << 28);

    /* Interruption de TIM3 (IRQ 29, identique au F103) */
    TIM3->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1 << 29);
}

/*****************************************************************
Fonctions d'interruption
*****************************************************************/

/* Fin des 300 ms LED allumee -> on l'eteint et on relance une attente
   aleatoire */
void TIM2_IRQHandler(void)
{
    reset_gpio(GPIOB, 3);
    stop_timer(TIM2);
    configure_timer(TIM3, 7999, rand());
    start_timer(TIM3);
    TIM2->SR &= ~TIM_SR_UIF;
}

/* Fin de l'attente aleatoire -> on allume la LED pour 300 ms */
void TIM3_IRQHandler(void)
{
    set_gpio(GPIOB, 3);
    stop_timer(TIM3);
    start_timer(TIM2);
    TIM3->SR &= ~TIM_SR_UIF;
}

/*****************************************************************
Fonction pre-definie (identique au cours - generateur pseudo-aleatoire
logiciel, aucune dependance materielle donc aucune adaptation F103/F303)
*****************************************************************/

/**
 * Retourne une valeur entiere pseudo-aleatoire comprise entre 800 et 1800.
 */
int rand(void)
{
    static int randomseed = 0;
    randomseed = (randomseed * 9301 + 49297) % 233280;
    return 800 + (randomseed % 1000);
}
