/*
 * correction - "Entrainez-vous en detectant l'appui sur un bouton"
 * (Partie 4 ch.5, cours OpenClassrooms "Developpez en C pour l'embarque")
 *
 * ATTENTION : ce fichier a ete redige par Claude a la demande explicite
 * de l'utilisateur, pour servir de corrige de reference - PAS pour
 * remplacer l'exercice. Remplace le corrige PWM+ADC precedent
 * (recuperable via `git show c23db1e:correction/src/main.c`).
 *
 * C'est la suite directe du jeu "LED aleatoire" (v1, Partie 3 ch.6,
 * recuperable via `git show 5d2a032:correction/src/main.c`) : le
 * joueur doit appuyer sur le bouton PENDANT que la LED est allumee
 * (fenetre de 300 ms). En cas de reussite, la LED clignote a un rythme
 * regulier (periode 250 ms) au lieu de reprendre le cycle aleatoire.
 *
 * Adapte depuis le corrige officiel du cours (main_v2_correction.c,
 * telecharge depuis static.oc-static.com), concu pour une NUCLEO-F103RB.
 * Portage NUCLEO-F303K8 :
 *
 * 1. LED : PA5 (LD2, F103) -> PB3 (LD3, F303K8), comme sur toutes les
 *    versions precedentes de ce corrige.
 * 2. Bouton : le cours utilise PC13 (bouton USER integre du Nucleo-64,
 *    avec pull-up deja cable sur la carte). **PC13 n'existe pas sur le
 *    connecteur du Nucleo-32.** Remplace ici par le SW du joystick
 *    externe sur breadboard, cable sur **PA0** (meme montage que sur
 *    `tp07_RandLEd`) : pas de pull-up integree sur le module, donc
 *    `PUPDR` interne en pull-up plutot que l'entree "floating" du cours
 *    (qui ne fonctionnerait pas sans resistance externe sur ce module).
 * 3. **TIM4** (clignotement de victoire) : **n'existe pas sur le
 *    F303K8** (cf. `docs/correspondance-f103-f303.md` §5). Remplace par
 *    **TIM6** (timer basique, suffisant : on n'a besoin que d'un
 *    debordement periodique, pas de canaux de sortie).
 * 4. Interruption externe du bouton : mecanisme totalement different,
 *    comme deja rencontre sur `tp07_RandLEd` :
 *    - F103 : `AFIO->EXTICR[3]` (PC13 = ligne EXTI13, registre
 *      EXTICR n°3 = ligne 12-15), `RCC_APB2ENR_AFIOEN`, gestionnaire
 *      partage `EXTI15_10_IRQHandler` (les lignes 10 a 15 partagent une
 *      seule IRQ sur F103).
 *    - F303 : `SYSCFG->EXTICR[0]` (PA0 = ligne EXTI0, registre n°0),
 *      `RCC_APB2ENR_SYSCFGEN`, gestionnaire **dedie** `EXTI0_IRQHandler`
 *      (les lignes 0 a 4 ont chacune leur propre IRQ sur F303 - pas de
 *      partage a gerer ici, plus simple que le cas PC13 du cours).
 * 5. **Piege d'indexation NVIC** : TIM6 a l'IRQ numero **54**
 *    (`TIM6_DAC1_IRQn`, verifie dans stm32f303x8.h) - le premier
 *    registre utilise dans ce projet ou l'IRQ depasse 31 ! `ISER[0]`
 *    couvre les IRQ 0-31, `ISER[1]` couvre les IRQ 32-63. Il faut donc
 *    `NVIC->ISER[1] |= (1 << (54-32))`, PAS `ISER[0]` comme pour
 *    TIM2/TIM3/EXTI0 (tous < 32) - erreur facile a faire en copiant le
 *    reflexe habituel sans recalculer l'index de tableau.
 * 6. `NVIC_ISER_SETENA_28/29/30/8` (cours) : macros absentes de notre
 *    CMSIS (meme piege que sur les corriges precedents), remplacees par
 *    des decalages de bit bruts.
 * 7. Frequences recalculees pour l'horloge REELLE actuelle (HSI 8 MHz,
 *    pas de PLL) plutot que les 72 MHz supposes par le cours - meme
 *    convention "tick de 1 ms" (`PSC=7999`) que sur les corriges
 *    precedents : `ARR=300` pour 300 ms, `ARR=rand()` directement en ms
 *    pour l'attente aleatoire, `ARR=249` pour les 250 ms de clignotement
 *    de victoire (au lieu de `PSC=7199`/`ARR=2499` a 72 MHz).
 * 8. `led_on`/`victoire` marquees `volatile` (bonne pratique deja vue
 *    sur `tp07_RandLEd` pour une variable partagee entre plusieurs
 *    gestionnaires d'interruption).
 */

#include "stm32f3xx.h"

/*****************************************************************
Peripheriques utilises :
GPIOB, broche 3 : pilote LD3 (LED verte utilisateur)
GPIOA, broche 0 : detecte l'appui du bouton (SW du joystick externe)
TIM2 : chronometre les 300 ms d'allumage de la LED
TIM3 : chronometre l'intervalle aleatoire avant d'allumer la LED
TIM6 : assure le clignotement de la LED en cas de victoire
EXTI0 : declenche l'interruption sur l'appui du bouton (front descendant)
*****************************************************************/

int rand(void);
void configure_gpio_pb3(void);
void configure_gpio_pa0(void);
void configure_syscfg_exti_pa0(void);
void set_gpio(GPIO_TypeDef *GPIO, int n);
void reset_gpio(GPIO_TypeDef *GPIO, int n);
void configure_timer(TIM_TypeDef *TIM, int psc, int arr);
void configure_it(void);
void start_timer(TIM_TypeDef *TIM);
void stop_timer(TIM_TypeDef *TIM);

/*****************************************************************
Variables globales
*****************************************************************/

volatile int led_on = 0;   /* indique si la LED est allumee ou non */
volatile int victoire = 0; /* indique si le joueur a gagne (informatif) */

/*****************************************************************
MAIN
*****************************************************************/

int main(void)
{
    /* Configuration des ports d'entree/sortie */
    configure_gpio_pb3();
    configure_gpio_pa0();
    configure_syscfg_exti_pa0();

    /* Configuration des timers */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM6EN;
    configure_timer(TIM2, 7999, 300);
    configure_timer(TIM3, 7999, rand());
    configure_timer(TIM6, 7999, 249);

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
 * Configure la broche 3 du port B (LD3) en sortie push-pull.
 */
void configure_gpio_pb3(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(0x03 << 2 * 3);
    GPIOB->MODER |= (0x01 << 2 * 3);
}

/**
 * Configure la broche 0 du port A (bouton SW du joystick) en entree
 * avec pull-up interne (le module n'a pas de pull-up integree).
 */
void configure_gpio_pa0(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~(0x03 << 2 * 0);
    GPIOA->PUPDR &= ~(0x03 << 2 * 0);
    GPIOA->PUPDR |= (0x01 << 2 * 0);
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
 * Configure PA0 comme source d'interruption externe (ligne EXTI0).
 */
void configure_syscfg_exti_pa0(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA;
}

/**
 * Configure toutes les interruptions du systeme.
 */
void configure_it(void)
{
    /* Interruption de TIM2 (IRQ 28) */
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1 << 28);

    /* Interruption de TIM3 (IRQ 29) */
    TIM3->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1 << 29);

    /* Interruption de TIM6 (IRQ 54 - depasse 31, va dans ISER[1] a
       l'index 54-32=22, pas dans ISER[0] !) */
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[1] |= (1 << (54 - 32));

    /* Interruption externe EXTI0 (bouton, front descendant) */
    EXTI->IMR |= EXTI_IMR_MR0;
    EXTI->FTSR |= EXTI_FTSR_FT0;
    NVIC->ISER[0] |= (1 << 6); /* IRQ 6 = EXTI0 */
}

/*****************************************************************
Fonctions d'interruption
*****************************************************************/

/* Fin des 300 ms LED allumee -> on l'eteint et on relance une attente
   aleatoire */
void TIM2_IRQHandler(void)
{
    reset_gpio(GPIOB, 3);
    led_on = 0;
    stop_timer(TIM2);
    configure_timer(TIM3, 7999, rand());
    start_timer(TIM3);
    TIM2->SR &= ~TIM_SR_UIF;
}

/* Fin de l'attente aleatoire -> on allume la LED pour 300 ms */
void TIM3_IRQHandler(void)
{
    set_gpio(GPIOB, 3);
    led_on = 1;
    stop_timer(TIM3);
    start_timer(TIM2);
    TIM3->SR &= ~TIM_SR_UIF;
}

/* Clignotement de victoire (toggle toutes les 250 ms) */
void TIM6_IRQHandler(void)
{
    if (GPIOB->ODR & (0x01 << 3)) {
        reset_gpio(GPIOB, 3);
    } else {
        set_gpio(GPIOB, 3);
    }
    TIM6->SR &= ~TIM_SR_UIF;
}

/* Appui du bouton : victoire seulement si la LED etait allumee au
   moment de l'appui */
void EXTI0_IRQHandler(void)
{
    if (led_on) {
        victoire = 1;
        stop_timer(TIM2);
        stop_timer(TIM3);
        start_timer(TIM6);
    }
    EXTI->PR |= EXTI_PR_PR0; /* s'efface en ecrivant 1, comme deja vu */
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
