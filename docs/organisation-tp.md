# Decoupage du cours en TP + usage de la breadboard

Sommaire officiel du cours (recupere sur OpenClassrooms) :

- **Partie 1** - Installez et utilisez les outils (Intro, outils, cible/compil/exec,
  outils de dev, projet)
- **Partie 2** - Comprenez l'execution d'un programme (archi ARM, memoire, pile,
  exceptions/IT, lien C/asm) -- **theorique**, pas specifique a une carte
- **Partie 3** - Programmez votre microcontroleur (registres/masques, GPIO,
  timers, interruptions, projet "LED aleatoire")
- **Partie 4** - Peripheriques avances (PWM, ADC, UART, bonnes pratiques,
  projet "detection d'appui bouton")

## 1. Proposition de decoupage en dossiers

Un dossier `tpNN-<sujet>/` par chapitre **avec code**, cree au moment ou vous
l'attaquez (via `template-tp/`, voir README). Les chapitres purement
theoriques (Partie 2 en quasi-totalite) n'ont pas besoin de dossier : une
note dans `fiche-suivi-projet-embarque.md` suffit, sauf si vous voulez
experimenter (ex: observer la pile en RAM sous GDB -> peut justifier un
`tp0x-pile-systeme/` minimal).

| Dossier propose | Chapitre du cours | Notes F303K8 |
|---|---|---|
| `tp01-hello-target` | P1.3 Configurez une cible, compilez, executez | deja couvert par `tp00-smoke-test`, a dupliquer/adapter si vous voulez le refaire vous-meme en suivant le cours pas a pas |
| `tp02-gpio` | P3.3 Configurez les ports d'E/S | **le plus gros ecart** : MODER/OTYPER/PUPDR/AFR au lieu de CRL/CRH, cf. `docs/correspondance-f103-f303.md` §4 |
| `tp03-timers` | P3.4 Gerer le temps avec les timers | TIM4 absent -> utiliser TIM3/TIM15/16/17 (§5) |
| `tp04-interruptions` | P3.5 Gerez vos interruptions | table de vecteurs et noms d'IRQ differents (§8), pas de bouton B1 -> breadboard (voir plus bas) |
| `tp05-led-aleatoire` | P3.6 Projet "LED aleatoire" | bouton externe requis (pas de B1 utilisateur) |
| `tp06-pwm` | P4.1 PWM | LD3/PB3 = SPI1_SCK, prevoir LED externe si vous voulez garder SPI libre en parallele |
| `tp07-adc` | P4.2 ADC | pas d'ADC sur A4/PA5 -> choisir A0/A1/A3/A5/A6/A7 pour le potentiometre |
| `tp08-uart` | P4.3 Communication serie | USART2 sur PA2/PA15 = VCP (comme le cours), transposition directe |
| `tp09-bouton` | P4.5 Projet "detection d'appui" | bouton externe + debounce (pas de B1 GPIO) |

Adaptez librement les numeros/noms ; ce qui compte est qu'un `README.md`
local dans chaque `tpNN/` note les differences F103->F303 rencontrees
(cf. gabarit `template-tp/`), pour nourrir la Skill Claude en fin de
parcours (etape 8 de la demande initiale).

## 2. Ou la breadboard vient combler les limites du Nucleo-32

| Besoin du cours | Absent/limite sur Nucleo-32 | Solution breadboard |
|---|---|---|
| Bouton utilisateur (B1) | B1 du Nucleo-32 = RESET uniquement, pas un GPIO | Bouton poussoir + resistance de tirage (pull-down externe ou `PUPDR` interne en pull-up + bouton vers GND) sur une broche libre, ex. **PA0** ou **PB0** |
| 2e LED (pour PWM, comparaison de canaux, jeux de lumiere) | Une seule LED utilisateur (LD3=PB3, partagee avec SPI1_SCK) | LED + resistance ~220-330 ohm sur une broche libre (ex. **PA1** pour rester loin de SPI/I2C) |
| Entree analogique pour l'ADC | A4/PA5 sans ADC ; peu de broches ADC dispo au total | Potentiometre (diviseur de tension) sur **A0/PA0** ou **A1/PA1** ; en bonus, LDR (photoresistance) + resistance fixe pour un capteur de lumiere |
| Sortie sonore pour la partie PWM | Aucune sortie audio native | Buzzer passif sur une broche a canal timer PWM libre, ex. **PA8 (D9, TIM1_CH1)** ou **PB1 (D6, TIM3_CH4)** |
| Capteurs I2C/SPI (extension au-dela du cours) | Pas de capteur embarque | I2C1 dispo sur **PB6(SCL)/PB7(SDA)** (D5/D4) pour un capteur externe (ex. BME280, MPU6050) en bonus post-cours ; SPI1 sur **PA11/PB3-5** pour un ecran ou capteur SPI, en notant le conflit avec LD3 |

Points d'attention breadboard :
- Toujours une resistance en serie avec une LED externe (le GPIO STM32 sort
  du 3.3V, ~10-15mA max recommande par broche).
- Pour un bouton vers GND, activer le pull-up interne (`PUPDR` = pull-up) et
  lire l'entree active a l'etat bas, ou cabler une resistance de tirage
  externe si vous voulez pratiquer le calcul du pont diviseur.
- Alimentation 3.3V disponible sur le connecteur (broche `3V3`), ne pas
  utiliser le 5V logique cote GPIO.
