/*
 * correction - PWM (Partie 4 ch.1) + ADC (Partie 4 ch.2, "Domptez votre
 * convertisseur analogique-numerique") combines : la position d'un
 * potentiometre pilote en direct le rapport cyclique d'une PWM.
 * (Cours OpenClassrooms "Developpez en C pour l'embarque")
 *
 * ATTENTION : ce fichier a ete redige par Claude a la demande explicite
 * de l'utilisateur (l'utilisateur n'a pas de potentiometre sous la main
 * pour faire ce TP lui-meme), pour servir de corrige de reference -
 * PAS pour remplacer l'exercice. Version PWM seule (sans ADC)
 * recuperable via `git show 1fe78ac:correction/src/main.c`.
 *
 * L'exemple officiel du cours combine directement PWM + ADC dans le
 * main() final : le balayage automatique du rapport cyclique par IT de
 * TIM2 (chapitre PWM seul) est remplace ici par une lecture continue du
 * potentiometre dans la boucle principale - TIM2 n'est donc plus utilise
 * du tout dans cette version.
 *
 * PORTAGE NUCLEO-F303K8 - PWM (identique a la version precedente,
 * cf. `git show 1fe78ac` pour le detail) : PA6/TIM3_CH1 via AF2,
 * frequence PWM recalculee pour l'horloge reelle a 8 MHz.
 *
 * PORTAGE NUCLEO-F303K8 - ADC (**le plus gros ecart de tout ce projet**) :
 * le peripherique ADC a ete entierement redessine entre le F103 et le
 * F303, ce n'est pas un simple renommage de registres.
 *
 * 1. Broche : le cours utilise PB0 (ADC1_IN8 sur F103). Ici, **PA0**
 *    (label "A0" du connecteur Nucleo-32) est utilise a la place =
 *    **ADC1_IN1** sur le F303 (verifie par recherche externe) - PB0
 *    aurait aussi fonctionne (broche presente sur le connecteur, label
 *    "D3"), mais son numero de canal ADC sur F303 n'a pas ete verifie ;
 *    PA0 est le choix le plus documente/standard.
 * 2. GPIO en mode analogique : F103 = CRL nibble entierement a `0000`
 *    (MODE=00 + CNF=00). F303 = **`MODER` = `11`** (pas `00`, qui est un
 *    simple mode entree numerique sur cette puce !) - erreur facile a
 *    faire en copiant le reflexe "mettre les bits a 0" du F103.
 * 3. Peripherique ADC completement redessine : le F103 utilise
 *    `CR1`/`CR2`/`SR`/`SQR1`/`SQR3` (legacy). Le F303 utilise
 *    `CR`/`CFGR`/`ISR`/`SQR1` (architecture "moderne", comme pour
 *    USART/GPIO) - noms de registres differents, PAS de simple
 *    correspondance 1-pour-1.
 * 4. Horloge : F103 = `RCC->APB2ENR`/`ADC1EN` + prescaler dans
 *    `RCC->CFGR` (`ADCPRE_DIV6`, ADC limite a 14 MHz). F303 =
 *    `RCC->AHBENR`/`ADC12EN` (bit 28, partage entre ADC1 et ADC2) +
 *    mode d'horloge dans `ADC12_COMMON->CCR` (`CKMODE`) - choisi ici en
 *    synchrone HCLK/1 (le plus simple, pas besoin d'une horloge ADC
 *    dediee separee).
 * 5. **Etape totalement absente du F103** : le F303 a un regulateur de
 *    tension interne dedie a l'ADC (`ADVREGEN`) qu'il faut activer
 *    explicitement et laisser stabiliser quelques microsecondes avant
 *    de calibrer/activer l'ADC - sans ca, l'ADC ne fonctionne pas du
 *    tout. Absent de la procedure F103, qui n'a pas ce regulateur.
 * 6. Calibration : F103 = `CR2.CAL`, attendre qu'il retombe a 0. F303 =
 *    meme principe mais avec `CR.ADCAL` (nom different, mecanisme
 *    identique).
 * 7. Activation : F103 n'a qu'un `ADON` (un bit fait tout : active PUIS
 *    declenche une conversion en le reecrivant). F303 separe clairement
 *    les etapes : `ADEN` (active l'ADC, attendre le flag `ADRDY`) PUIS
 *    `ADSTART` (declenche une conversion) - deux bits distincts pour
 *    deux actions distinctes.
 * 8. Sequence de conversion (`SQR1`) : le F103 repartit les canaux
 *    entre plusieurs registres (`SQR1`/`SQR2`/`SQR3`, le premier canal
 *    est dans `SQR3`). Le F303 regroupe la longueur de sequence ET le
 *    1er canal dans le **meme registre** `SQR1` (champs `L` et `SQ1`
 *    distincts mais co-localises) - a ne pas chercher dans `SQR3` sur
 *    F303, la disposition est differente.
 * 9. Acquittement des flags de fin de conversion : `ADC->ISR` s'efface
 *    en ecrivant un **1** (`|= ADC_ISR_EOC`), comme `EXTI->PR` deja vu -
 *    PAS comme `TIM->SR` (ecriture a 0). Le F103 utilisait deja ce
 *    style figure (`SR &= ~EOC`) pour son ADC - encore un exemple de
 *    convention qui differe non seulement entre F103 et F303, mais
 *    entre peripheriques d'une meme puce.
 */

#include "stm32f3xx.h"

/*****************************************************************
Peripheriques utilises :
GPIOA, broche 6 (label "A5") : sortie PWM, AF2 = TIM3_CH1
GPIOA, broche 0 (label "A0") : entree analogique, ADC1_IN1
TIM3 : genere le signal PWM 20 kHz sur son canal 1
ADC1 : lit la position du potentiometre en continu (polling)
*****************************************************************/

void configure_gpio_pa6_alternate_push_pull(void);
void configure_pwm_ch1_20khz(TIM_TypeDef *TIMER);
void start_timer(TIM_TypeDef *TIMER);
void set_pulse_percentage(TIM_TypeDef *TIMER, int pulse);
void configure_gpio_pa0_analog_input(void);
void configure_adc_in1(void);
int convert_single(void);

int main(void)
{
    configure_gpio_pa6_alternate_push_pull();
    configure_pwm_ch1_20khz(TIM3);
    set_pulse_percentage(TIM3, 0);

    configure_gpio_pa0_analog_input();
    configure_adc_in1();

    start_timer(TIM3);

    while (1) {
        int res = convert_single();
        set_pulse_percentage(TIM3, 100 * res / 0xFFF);
    }

    return 0;
}

/*****************************************************************
Corps des fonctions - PWM (inchange par rapport a la version precedente)
*****************************************************************/

/**
 * Configure PA6 en fonction alternative push-pull (AF2 = TIM3_CH1).
 */
void configure_gpio_pa6_alternate_push_pull(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER &= ~(0x03 << 2 * 6);
    GPIOA->MODER |= (0x02 << 2 * 6);

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

    TIMER->CCMR1 &= ~TIM_CCMR1_OC1M_0;
    TIMER->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

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

/*****************************************************************
Corps des fonctions - ADC
*****************************************************************/

/**
 * Configure PA0 en entree analogique (MODER = 11, PAS 00).
 */
void configure_gpio_pa0_analog_input(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    GPIOA->MODER |= (0x03 << 2 * 0); /* 11 = analogique sur F303 */
}

/**
 * Active et calibre ADC1 sur son canal 1 (PA0), sans lancer de
 * conversion.
 */
void configure_adc_in1(void)
{
    RCC->AHBENR |= RCC_AHBENR_ADC12EN;

    /* Horloge ADC synchrone, HCLK/1 (le plus simple : pas d'horloge
       ADC dediee separee a configurer) */
    ADC1_2_COMMON->CCR &= ~ADC12_CCR_CKMODE;
    ADC1_2_COMMON->CCR |= ADC12_CCR_CKMODE_0;

    /* Etape absente du F103 : activer le regulateur de tension interne
       de l'ADC et laisser le temps de stabiliser (~10-20us minimum
       d'apres le datasheet) avant toute calibration/activation */
    ADC1->CR &= ~ADC_CR_ADVREGEN;
    ADC1->CR |= ADC_CR_ADVREGEN_0;
    for (volatile int i = 0; i < 400; i++) {
        __NOP();
    }

    /* Calibration (equivalent du CR2.CAL du F103, nom different) */
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) {
    }

    /* Activation de l'ADC, puis attente du flag "pret" (etape separee
       du demarrage d'une conversion, contrairement au F103) */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) {
    }

    /* Sequence de 1 seule conversion (L=0), canal 1 en 1ere position */
    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |= (1U << ADC_SQR1_SQ1_Pos);
}

/**
 * Declenche une conversion sur ADC1 et retourne le resultat (0-4095).
 */
int convert_single(void)
{
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) {
    }
    ADC1->ISR |= ADC_ISR_EOC; /* s'efface en ecrivant 1, comme EXTI->PR */
    return ADC1->DR;
}
