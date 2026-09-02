# Correspondance NUCLEO-F103RB (cours) -> NUCLEO-F303K8 (carte reelle)

Sources : RM0008 (F103, medium density), RM0316 (F303x6/x8), datasheets ST
STM32F103RB / STM32F303K8, UM1956 (Nucleo-32).

## 1. Coeur et horloge

| | F103RB (cours) | F303K8 (cette carte) |
|---|---|---|
| Coeur | Cortex-M3 | Cortex-M4F (FPU simple precision) |
| Frequence max | 72 MHz | 72 MHz (identique) |
| Jeu d'instructions | Thumb-2, pas de FPU, pas de DSP | Thumb-2 + FPU (VFPv4-SP) + instructions DSP (SIMD) |
| Flash | 128 KB | **64 KB** (2x moins) |
| RAM | 20 KB | **12 KB** (moins de 2x) |
| Boitier | LQFP64 | LQFP32 (format "Nucleo-32", Arduino Nano) |

Impact pratique : les binaires doivent rester compacts (attention aux `printf`
avec floats qui gonflent vite avec newlib), et le flag de compilation doit
cibler `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` au lieu de
`-mcpu=cortex-m3` (pas de `-mfpu`/`-mfloat-abi` sur F103).

## 2. GPIO disponibles

Le Nucleo-64 (F103RB) expose la quasi-totalite des broches du LQFP64 sur les
connecteurs ST Morpho + Arduino Uno (~50 GPIO utilisables). Le Nucleo-32
(F303K8) n'expose que le connecteur Arduino **Nano** : 22 broches utilisables.

| Fonction Arduino | Broche STM32 | Remarque |
|---|---|---|
| D0 | PA10 | USART1_RX |
| D1 | PA9  | USART1_TX |
| D2 | PA12 | |
| D3 | PB0  | TIM3_CH3 / TIM8_CH2N |
| D4 | PB7  | partage avec I2C1_SDA |
| D5 | PB6  | I2C1_SCL, TIM16_CH1N (canal complementaire, PWM "inverse") |
| D6 | PB1  | TIM3_CH4 |
| D7 | PF0  | **OSC_IN** si resonateur externe monte (sinon libre) |
| D8 | PF1  | **OSC_OUT** si resonateur externe monte (sinon libre) |
| D9 | PA8  | TIM1_CH1 |
| D10| PA11 | SPI1_NSS / USB |
| D11| PB5  | SPI1_MOSI |
| D12| PB4  | SPI1_MISO |
| D13| PB3  | SPI1_SCK **= LD3 (LED verte utilisateur)** |
| A0 | PA0  | |
| A1 | PA1  | |
| A2 | PA3  | attention : RX possible mais signale instable en reception UART sur certains montages (cf. erratum communaute ST) |
| A3 | PA4  | |
| A4 | PA5  | I2C-like (pas de vrai I2C dessus), pas d'ADC dispo sur A4 |
| A5 | PA6  | |
| A6 | PA7  | |
| A7 | PA2  | **partage avec VCP_TX (USART2 vers ST-LINK)** : a eviter si vous utilisez le port serie virtuel |

Consequence directe : le TP F103RB qui utilise PA5 pour LD2 et PC13 pour B1
(bouton utilisateur) **n'a pas d'equivalent direct** : PC13 n'existe pas sur
le connecteur du Nucleo-32.

- **LED utilisateur** : LD3 (verte) est sur **PB3**, pas PA5. Elle partage la
  broche D13/SPI1_SCK : si un TP plus tard utilise SPI1, LD3 clignotera en
  meme temps que le bus (a savoir en cas de comportement bizarre).
- **Bouton utilisateur** : il **n'y en a pas** sur le Nucleo-32 (seul B1 =
  RESET, cable sur NRST, inutilisable en GPIO). -> voir section 6 (breadboard)
  pour le remplacer par un bouton externe sur breadboard.

## 3. RCC : activation d'horloge des peripheriques

C'est le piege le plus frequent en portant le code du cours.

| | F103RB | F303K8 |
|---|---|---|
| Horloge GPIOx | **APB2ENR**, bits `IOPAEN`, `IOPBEN`, ... | **AHBENR**, bits `GPIOAEN`, `GPIOBEN`, ... |
| Horloge AFIO (remap) | `RCC_APB2ENR_AFIOEN` (registre AFIO dedie) | N'existe pas : le "remap" est integre dans `GPIOx_AFR[]` (voir section 4) |
| Horloge USART1 | `RCC_APB2ENR_USART1EN` | `RCC_APB2ENR_USART1EN` (identique, USART1 reste sur APB2) |
| Horloge USART2/3 | `RCC_APB1ENR_USART2EN` / `USART3EN` | Identique (APB1) |
| Horloge TIM2/3/4 | `RCC_APB1ENR_TIMxEN` | Identique pour TIM2/TIM3 (TIM4 absent, voir section 5) |

Exemple concret (activer GPIOB) :
```c
/* F103RB (cours) */
RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

/* F303K8 (cette carte) */
RCC->AHBENR  |= RCC_AHBENR_GPIOBEN;
```

## 4. Configuration GPIO : registre completement different

Le F103 utilise l'ancien schema **CRL/CRH** (legacy, herite du F1) :
2 registres 32 bits de 4 bits par broche (2 bits MODE + 2 bits CNF), pas de
registre AF separe -> le remapping des fonctions alternatives se fait via
`AFIO->MAPR` (un registre global avec un nombre limite de combinaisons
predefinies : remap total ou partiel par peripherique).

Le F303 utilise le schema "moderne" commun a F0/F2/F3/F4/L1/L4... :

| Registre | Role | Existe sur F103 ? |
|---|---|---|
| `MODER` | Mode (IN/OUT/AF/Analog), 2 bits/broche | Non (equivalent : CRL/CRH bits CNF+MODE) |
| `OTYPER` | Push-pull / open-drain, 1 bit/broche | Non (equivalent : CRL/CRH bit CNF0) |
| `OSPEEDR` | Vitesse de sortie, 2 bits/broche | Non (equivalent : CRL/CRH bits MODE) |
| `PUPDR` | Pull-up/pull-down, 2 bits/broche | Non (pull géré différemment via ODR en mode input pull) |
| `AFR[0]`/`AFR[1]` | Fonction alternative AF0-AF15, 4 bits/broche | Non (AFIO_MAPR global, options limitees) |

C'est la partie du cours a **re-ecrire integralement**, pas juste adapter des
constantes : la logique de configuration (nombre de registres, largeur des
champs) change, contrairement a RCC ou seul le nom du registre bouge.

## 5. Timers

| Timer | F103RB (medium density) | F303K8 |
|---|---|---|
| TIM1 | Avance, 4 canaux + complementaires | Avance, 4 canaux + complementaires (identique dans l'esprit) |
| TIM2 | 16 bits, 4 canaux | **32 bits**, 4 canaux |
| TIM3 | 16 bits, 4 canaux | 16 bits, 4 canaux |
| TIM4 | 16 bits, 4 canaux | **absent** |
| TIM6/TIM7 | absents (medium density) | presents (basiques, TIM6/7 lies aussi au DAC) |
| TIM15/16/17 | absents | presents (petits timers avance, 1-2 canaux + complementaire) |

A retenir : si un TP du cours utilise TIM4, il faut le remplacer par TIM3,
TIM15, TIM16 ou TIM17 selon le besoin (nombre de canaux, PWM simple vs
complementaire).

## 6. USART

Meme nombre nominal (USART1/2/3) sur les deux puces. Difference pratique :
sur le Nucleo-32, **USART2 (PA2/PA15) est cablee au ST-LINK** pour faire
office de port serie virtuel (VCP) — comme sur le Nucleo-64. Donc le TP UART
du cours (souvent sur USART2 justement, pour profiter du VCP) reste
directement transposable : seul le nom du bit RCC change (`APB1ENR` identique
sur les deux, `USART2EN` — ici pas de piege RCC contrairement au GPIO).

## 7. Peripheriques presents uniquement sur le F303K8 (bonus vs cours)

- **FPU** materielle simple precision (le cours F103 n'en parle jamais)
- **Comparateurs analogiques** (COMP2, COMP4-6)
- **DAC1/DAC2** (lies a TIM6/TIM7)
- ADC plus rapide et plus flexible (SAR 12 bits, injection/regular, plus de
  modes que l'ADC1 "simple" du F103)
- Instructions DSP/SIMD (non utilisees en cours, mentionne pour info)

## 8. NVIC / table des vecteurs

Le nombre et l'ordre des IRQ sont differents (peripheriques differents =
IRQ differentes). La table des vecteurs vendee dans
`vendor/startup/startup_stm32f303x8.s` (issue du CMSIS ST officiel) est la
reference a utiliser -- ne pas recopier la table du cours (basee sur
`startup_stm32f10x_md.s`).

## 9. Debug / flash

Identique dans l'esprit (ST-LINK + OpenOCD + GDB), mais :
- Le F303K8 embarque un **ST-LINK/V2-1** (comme les Nucleo-64 recents).
- Le fichier de cible OpenOCD change : `target/stm32f1x.cfg` (cours) ->
  `target/stm32f3x.cfg` (ici), voir `tools/openocd_f303k8.cfg`.
