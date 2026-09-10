# Fiche de suivi - Cours C embarque (NUCLEO-F303K8)

## Contexte

Suivi du cours OpenClassrooms "Developpez en C pour l'embarque"
(https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque),
concu pour une NUCLEO-F103RB (STM32F103RB, Cortex-M3), adapte pour etre
suivi sur une **NUCLEO-F303K8** (STM32F303K8, Cortex-M4F, format Nucleo-32,
LQFP32, moins de broches, GPIO/registres differents) + breadboard pour
compenser les broches/peripheriques absents.

Objectif : reactiver des bases embarque (registres, GPIO, timers, UART, IT)
en bare-metal/CMSIS (pas de framework Arduino, pas de HAL complete ST -
acces registre direct comme le cours), sous Linux (WSL2 Ubuntu 24.04) avec
Git pour versionner chaque TP.

Depot GitHub (public) : https://github.com/RIVEIDD/oc-embarque-f303k8

Voir aussi :
- [`docs/correspondance-f103-f303.md`](docs/correspondance-f103-f303.md) -
  tableau de correspondance registres/GPIO/timers F103 -> F303
- [`docs/organisation-tp.md`](docs/organisation-tp.md) - decoupage des TP et
  usage de la breadboard

## Historique / Etat d'avancement

| Date | Fait |
|---|---|
| 2026-09-02 | Setup initial : verification toolchain (arm-none-eabi-gcc 13.2.1, gdb-multiarch 15.1, openocd 0.12.0, make 4.3 - tous deja presents) ; mise en place environnement bare-metal (CMSIS ST vendorise dans `vendor/`, linker script `common/linker/STM32F303K8Tx_FLASH.ld` pour Flash 64K/RAM 12K, startup officiel `startup_stm32f303x8.s`, Makefile partage `common/mk/common.mk`) ; build de `tp00-smoke-test` (blink LD3/PB3) valide avec succes en local (compile+link, non flashe : ST-LINK pas encore visible depuis WSL) ; init du repo Git avec `.gitignore` ; tableau de correspondance F103RB->F303K8 redige (`docs/correspondance-f103-f303.md`) ; proposition de decoupage des TP + usage breadboard (`docs/organisation-tp.md`) ; gabarit de TP reutilisable (`template-tp/`). Skill Claude reutilisable proposee mais reportee (utilisateur prefere attendre 1-2 TP reels avant de la figer). |
| 2026-09-02 | Publication GitHub : installation de `gh` + `gh auth login` (compte RIVEIDD) par l'utilisateur, cle d'hote GitHub ajoutee a `~/.ssh/known_hosts` (SSH bloque par defaut sous WSL2 sans `ssh-askpass`), creation du depot public `RIVEIDD/oc-embarque-f303k8` et push des 2 commits existants (`origin/master` tracke). |
| 2026-09-03 | `tp01-hello-uart` cree (USART2/PA2 -> "Hello, world!" en boucle, VCP ST-LINK) - exceptionnellement ecrit entierement par Claude a la demande explicite de l'utilisateur pour valider l'environnement, build+link valides en local (920 B Flash). Tentative de passage ST-LINK -> WSL2 via usbipd : blocage sur `usbipd bind` (etat reste `Not shared`), piste principale = PowerShell non lance en administrateur ; pas encore flashe/teste sur la carte reelle. Procedures `make flash` et `picocom` detaillees ci-dessous. |
| 2026-09-03 | ST-LINK debloque cote WSL2 (`usbipd bind` en admin) - `make flash` operationnel. `tp02-premier-blink` cree et **ecrit par l'utilisateur en autonomie** (guidage Claude uniquement) : transposition du tout premier exemple du cours (toggle PA5/CRL sur F103) vers PB3/MODER sur F303K8. Deux allers-retours de debug guides : (1) LED figee car boucle sans delai (toggle a une frequence bien superieure a la persistance retinienne) -> ajout d'un `delay()` a base de `__NOP()` ; (2) masque `0xF` (4 bits, style CRL) au lieu de `0x3` (2 bits, style MODER) sur la config de `GPIOB->MODER`, sans consequence ici mais corrige par l'utilisateur (note dans "Pieges rencontres" ci-dessous). Premiere session GDB complete (`make debug`, breakpoints, `next`/`continue`, inspection registre via `print/x`) - confirmee fonctionnelle sur la carte reelle, cf. procedures ci-dessous. Environnement valide de bout en bout : build + flash + execution autonome + debug GDB sur le vrai NUCLEO-F303K8. |
| 2026-09-04 | Fonctionnement detaille des Makefiles explique (mecanisme `MAKEFILE_LIST`/`vpath`/regles generiques de `common/mk/common.mk`). Creation de `CLAUDE.md` a la racine du repo (contexte, regle "guider pas coder a la place de l'utilisateur", structure, commandes de build, pieges F103->F303 recurrents, protocole fiche de suivi). Demarrage de `tp03_premier-projet` (chapitre "Entrainez-vous en creant un projet", Partie 1) : les ressources telechargees du cours (`librairie.lib` + `functions.h` avec prototypes vides) ont ete diagnostiquees comme **incompatibles avec notre toolchain** - `librairie.lib` est une archive `ar` valide mais ses objets internes sont compiles avec ARM Compiler 5 (`armcc`, Keil MDK-ARM/µVision), un format non linkable par `arm-none-eabi-gcc`/GNU ld (confirme via `file`/`ar t`/`xxd`, chaine `Component: ARM Compiler 5.06` visible dans le binaire). Decision utilisateur : reproduire l'esprit du TP (multi-fichiers) avec un `functions.c` maison plutot que sauter le chapitre ou tenter de decompiler le `.lib`. TARGET du Makefile corrige, `functions.c` pas encore ecrit (`make flash` echoue actuellement avec `No rule to make target 'build/functions.o'` - normal, fichier source manquant). |
| 2026-09-07 | Reprise de session : `functions.c` de `tp03_premier-projet` toujours pas ecrit (bloquant identique a la pause precedente, pas retraite cette session). Lecture de la Partie 2 du cours ("Comprenez l'execution d'un programme") - 3 chapitres theoriques resumes dans la nouvelle section "Notes de cours" ci-dessous : Introduction (composants processeur, familles microprocesseur/DSP/microcontroleur/FPGA), Architecture programmable ARM (Harvard, 17 registres, RISC, load/store LDR/STR, flags xPSR, ARMv7 vs v8), Memoire dans les architectures ARM (alignement 32 bits, little-endian, 5 modes d'adressage, pool litteral). Aucun code de TP modifie cette session - uniquement de la prise de notes de cours, Partie 2 etant theorique/generique (pas de specificite F103 vs F303). |
| 2026-09-08 | Suite et fin de la lecture de la Partie 2 : 3 derniers chapitres resumes dans "Notes de cours" - Procedures et pile systeme (BL/LR/BX LR, PUSH/POP, convention R0-R3, lien fait avec `_Min_Stack_Size = 0x400` deja present dans notre linker script), Exceptions et interruptions (IVT, empilage automatique, NVIC ISER/IP, lien fait avec le mecanisme `.weak`/`Default_Handler` deja utilise dans `vendor/startup/startup_stm32f303x8.s`), Compilation C/assembleur (chaine Keil mise en correspondance avec notre toolchain GNU - `.map`/`.elf`/`.ld` deja produits par `common.mk` - et piege note sur `__asm{}` Keil vs `asm volatile()` GCC). Partie 2 quasi terminee (reste le quiz recapitulatif). `functions.c` de `tp03_premier-projet` toujours pas ecrit - reste le blocage principal cote pratique. |
| 2026-09-09 | Debut de la Partie 3 ("Programmez votre microcontroleur") : 3 chapitres lus et resumes dans "Notes de cours" - Specificites d'une architecture microcontroleur (peripheriques types : GPIO/timers/watchdog/capture-compare/ADC/PWM/bus, chiffres F103 a comparer au F303K8), Manipulez les registres et les masques (technique bit-a-bit generique, deja appliquee dans tous nos TP), Configurez les ports d'entree/sortie (**pas encore code, prevu demain**). Pour ce dernier, analyse d'adaptation F103->F303 complete preparee : `IDR`/`ODR`/`BSRR`/`BRR` transposables tels quels (verifie dans le CMSIS), `CRL`/`CRH` -> `MODER`/`OTYPER`/`PUPDR` (deja fait sur `tp02`), et surtout le bouton USER (PC13 sur F103) n'a pas d'equivalent sur le Nucleo-32 - deux options presentees (floating + pull externe comme le cours, ou `PUPDR` interne, registre qui n'existe pas sur F103). `tp04-gpio` sera cree demain par l'utilisateur via `template-tp`. `functions.c` de `tp03` toujours pas ecrit. |
| 2026-09-10 | Nouvelle regle adoptee : chaque commande shell d'un TP est desormais consignee dans la section "Journal des commandes (par TP)" au fil de l'eau (pas seulement resumee en fin de session) - codifie aussi dans `CLAUDE.md`. `tp04-gpio` avance : LED PB3 en sortie (reprise de la logique `tp02`), remplacement du bouton USER (absent sur Nucleo-32) par le bouton SW d'un joystick externe sur breadboard (cablage 3V3 plutot que 5V pour rester simple sur la tolerance des GPIO, SW sur une broche libre avec `PUPDR` interne en pull-up puisque le module n'a pas de pull-up integre) ; plusieurs erreurs de code corrigees en autonomie (instruction hors fonction, mauvais port RCC active - GPIOA/GPIOC au lieu de GPIOB pour la LED -, typo `PIOA` au lieu de `GPIOA`). Demarrage de `tp05-timer` (Partie 3 ch.4, timers) : bug "TIM2->CNT reste a 0x0" investigue et resolu - fausse alerte, `PSC=7199`/`ARR=9999` sont les valeurs de l'exemple du cours calculees pour 72 MHz, alors que la carte tourne encore a 8 MHz (~9x plus lent que prevu, tick toutes les 900µs) ; le compteur avance bien, juste lu trop tot / a verifier avec `continue` + `Ctrl-C` plutot qu'un `print` immediat. Chapitre "Gerer le temps avec les timers" lu et resume dans "Notes de cours" (formule de periode, registres CR1/PSC/ARR/CNT/SR, detection UIF par polling). Aucun commit de code TP effectue cette session (uniquement discussions/corrections en cours d'ecriture). |

## Journal des commandes (par TP)

Chaque commande shell utilisee pendant un TP, dans l'ordre, pour pouvoir
reproduire ou deboguer une session sans avoir a la reconstituer de
memoire. Mise a jour au fil de l'eau (pas seulement en fin de session).

### tp04-gpio
```sh
cp -r template-tp tp04-gpio
```
TARGET du Makefile pas encore renomme (toujours `tpXX-nom-du-tp`).

### tp05-timer
```sh
cp -r template-tp tp05-timer   # inferee depuis la structure du dossier (build/, inc/, src/,
                                 # Makefile identique au gabarit) - pas rapportee explicitement
                                 # au moment de l'execution, a confirmer si besoin
make                             # build reussi (voir build/*.elf/*.bin/*.hex, TARGET pas renomme)
make debug                      # session GDB pour investiguer TIM2->CNT
```
Commandes GDB utilisees pendant la session de debug de `TIM2->CNT` (reste
a 0x0 apparent, cause : PSC/ARR calcules pour 72 MHz alors que la carte
tourne a 8 MHz - cf. "Notes de cours" > Partie 3 > Gerer le temps avec
les timers) :
```
(gdb) print/x TIM2->CNT
(gdb) display/x TIM2->CNT
(gdb) continue
```
Puis, apres deconnexion physique de la carte (USB debranche) pendant que
GDB/OpenOCD tournaient encore, sortie propre de la session :
```
(gdb) quit
```
(ou, si GDB ne repondait plus : `Ctrl-C` puis `quit`, et en dernier
recours depuis un autre terminal : `pkill openocd`).

### Reference generale (hors TP specifique)
Commandes Git discutees cette session, reutilisables sur n'importe quel TP :
```sh
git add nom-du-dossier/                       # stage recursivement tout un dossier
git log origin/master..HEAD --oneline         # liste les commits locaux pas encore pushes
```

## Procedures d'installation / reprise

### Toolchain (deja installee sur cette machine)
```sh
sudo apt install gcc-arm-none-eabi gdb-multiarch openocd make
```

### ST-LINK depuis WSL2
Le ST-LINK n'est **pas visible par defaut** depuis WSL2 (USB non partage par
Hyper-V/WSLg). Cote Windows (PowerShell **en administrateur**, obligatoire
pour `bind`) :
```powershell
winget install --interactive --exact dorssel.usbipd-win
usbipd list                              # reperer le BUSID du ST-LINK (ex. 2-5)
usbipd bind --busid <BUSID>
```
Verifier avec `usbipd list` que l'etat passe de `Not shared` a `Shared`
avant de continuer (si ca reste sur `Not shared`, la cause la plus
frequente est un PowerShell lance sans droits admin). Ensuite (PowerShell
normal suffit) :
```powershell
usbipd attach --wsl --busid <BUSID>
```
Puis verifier cote WSL avec `lsusb` (vendor `0483`) et
`openocd -f tools/openocd_f303k8.cfg`. A refaire a chaque redemarrage
Windows (ou `usbipd attach --wsl --busid <BUSID> --auto-attach` pour
automatiser le reattachement).

### Reprendre un TP
```sh
cd tpNN-nom-du-tp
make            # build
make flash      # flash (necessite ST-LINK visible, voir ci-dessus)
make debug      # openocd + gdb-multiarch
```

#### `make flash`, en detail
Le target (`common/mk/common.mk`) :
```makefile
flash: $(BUILD_DIR)/$(TARGET).elf
	openocd -f $(OPENOCD_CFG) -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"
```
1. Rebuild l'ELF si besoin (dependance Makefile).
2. Lance `openocd` avec `tools/openocd_f303k8.cfg` (interface ST-LINK SWD +
   cible `stm32f3x`).
3. La commande OpenOCD `program ... verify reset exit` : connexion au
   ST-LINK, arret du CPU cible, effacement des pages Flash necessaires,
   ecriture de l'ELF a partir de 0x08000000, **verify** (relecture +
   comparaison), **reset** (relache le CPU qui redemarre sur le nouveau
   firmware), **exit** (ferme OpenOCD - contrairement a `make debug` qui le
   laisse tourne pour GDB).

#### Terminal serie (picocom) pour lire l'USART2/VCP
```sh
ls /dev/ttyACM*                    # trouver le port (ttyACM0, ttyACM1...)
picocom -b 115200 /dev/ttyACM0
# Ctrl-A puis Ctrl-X pour quitter
```
- `-b 115200` doit correspondre **exactement** au baudrate configure dans
  le firmware (`USART2_BAUDRATE` dans le code) - sinon caracteres
  illisibles.
- `/dev/ttyACM0` (pas `ttyUSB0`) : le ST-LINK expose son VCP en USB
  CDC-ACM. N'apparait que si le passage usbipd a reussi.
- Format implicite 8N1 (8 bits donnees, pas de parite, 1 stop) = config par
  defaut de `USART2->CR1`/`CR2` si non modifiee dans le code.
- Fermer picocom (`Ctrl-A` `Ctrl-X`) n'arrete **pas** le firmware sur la
  carte, juste l'affichage cote PC : la carte continue de tourner en
  arriere-plan.

#### Arreter un firmware qui tourne
Pas de "Ctrl-C" embarque : une fois flashe, le programme tourne en boucle
tant que la carte est alimentee. Options :
1. **Debrancher l'USB/alimentation** - simple, mais le meme firmware
   repart automatiquement au rebranchement (il reste en Flash).
2. **Halter le CPU sans effacer le firmware** (utile pour inspecter l'etat
   a un instant donne) :
   ```sh
   make debug
   ```
   puis dans GDB : `Ctrl-C` fige l'execution, `continue`/`c` relance,
   `monitor reset halt` remet a zero puis fige immediatement.
3. **Reflasher un autre programme** (`make flash` depuis un autre dossier
   de TP) - remplace definitivement le code en cours.

Le bouton **B1** de la carte fait uniquement **RESET** (redemarre le meme
firmware depuis le debut), il ne l'arrete pas.

#### Points d'arret (breakpoints) avec GDB

Depuis le dossier du TP :
```sh
make debug
```
Lance `openocd` en arriere-plan (serveur GDB port 3333) puis
`gdb-multiarch` attache dessus (`target extended-remote localhost:3333`).
A l'attache, le CPU est halte la ou il se trouve (pas de reset
automatique) : si le firmware tournait deja, GDB l'arrete en plein milieu
de son execution.

**Poser un breakpoint**
```
(gdb) break main            # au debut de main()
(gdb) break main.c:26       # sur une ligne precise (ex. le toggle GPIO)
```

**Executer**
```
(gdb) continue    # (ou c) : tourne a pleine vitesse jusqu'au prochain breakpoint
(gdb) next        # (ou n) : execute la ligne courante SANS entrer dans les fonctions
(gdb) step        # (ou s) : idem mais entre dans les fonctions appelees
```
Piege : pour une ligne dans une boucle avec un `delay()` a des centaines
de milliers d'iterations, `next`/`step` sont impraticables (des dizaines
de milliers de pressions necessaires). Utiliser `continue` pour sauter
directement au prochain passage sur le breakpoint (un `continue` = un tour
de boucle complet), pas `next`.

**Inspecter un registre/variable**
```
(gdb) print/x GPIOB->ODR    # valeur hexadecimale actuelle - plus fiable
                              # que de regarder la LED a l'oeil nu
```

**Gerer les breakpoints**
```
(gdb) info breakpoints
(gdb) delete 1
```

**Quitter**
```
(gdb) quit    # ou Ctrl-D - tue aussi openocd en arriere-plan (trap du Makefile)
```

**Reflasher sans quitter la session** : `load` (envoie l'ELF courant en
Flash via OpenOCD) suivi de `monitor reset halt` (redemarre proprement
depuis le vecteur reset et re-halte).

**Messages a ne pas confondre avec des erreurs**, vus lors de la premiere
session :
- `Warn : keep_alive() was not invoked...` (OpenOCD) : avertissement
  cosmetique, arrive quand on reste un moment sur le prompt `(gdb)` entre
  deux commandes. Sans impact.
- `Info : rejected 'gdb' connection, no more connections allowed` :
  OpenOCD n'autorise qu'un seul client GDB a la fois - a surveiller si un
  autre outil (extension de debug IDE type Cortex-Debug) tente de se
  connecter en parallele sur le port 3333.
- Les logs `Info :`/`Warn :` d'OpenOCD s'affichent dans le meme terminal
  que la session GDB interactive (le Makefile ne redirige pas la sortie
  d'OpenOCD vers un fichier) - piste d'amelioration outillage encore en
  suspens, pas encore appliquee.

Point cle a retenir : le CPU halte **ne remet pas a zero les peripheriques**
- un registre comme `GPIOx->ODR` garde sa valeur (et la broche reste
  physiquement pilotee dans cet etat) meme quand l'execution est figee en
  plein milieu du programme.

## Pieges rencontres pendant les TP

- **`tp02-premier-blink` (config MODER)** : utiliser un masque `0xF` (4
  bits), herite du style CRL du F103. Sur `MODER`, chaque broche n'occupe
  que 2 bits, donc ce masque efface aussi les 2 bits de la broche suivante
  (PB4 ici). Ca ne casse rien aujourd'hui car PB4 est deja a `00` par
  defaut au reset, mais si un jour plusieurs broches d'affilee sont
  configurees sur le meme port, ca ecrasera une config deja faite. Le
  masque correct pour 2 bits serait `0x3` au lieu de `0xF`.

- **Ressources telechargeables du cours (`.lib`)** : certains chapitres
  (ex. "Entrainez-vous en creant un projet") fournissent un `.lib`
  precompile avec Keil ARM Compiler 5 (`armcc`), pense pour etre linke
  dans µVision - incompatible avec `arm-none-eabi-gcc`/GNU ld. Verifier
  avec `file mon.lib` (doit dire "current ar archive" - format conteneur
  OK) puis extraire un membre (`ar x mon.lib`) et regarder ses premiers
  octets (`xxd`) : la chaine `Component: ARM Compiler` confirme
  l'incompatibilite. Pas une erreur de manipulation, juste un ecart
  d'outillage a anticiper sur les prochains chapitres avec ressources
  telechargeables.

## Notes de cours (parties theoriques)

Chapitres sans TP materiel associe (concepts generiques, valables aussi
bien sur F103RB que F303K8) - resumes ici plutot que dans un dossier
`tpNN-.../`, cf. `docs/organisation-tp.md` §1.

### Partie 2 - Introduction (architecture processeur)
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4604781-introduction)

Objectif du chapitre : poser le vocabulaire des composants d'architecture
processeur qui permettent l'execution du code sur une cible embarquee.

**Composants d'une architecture processeur :**
- **ALU** (Arithmetic Logic Unit) : effectue les calculs entiers (8/16/32
  bits) - addition, soustraction, multiplication, operations logiques,
  manipulations de bits.
- **Banc de registres** : memoire d'acces rapide directement connectee a
  l'ALU. Inclut des registres speciaux : **PC** (Program Counter, adresse
  de la prochaine instruction), registre d'etat (flags), **SP** (Stack
  Pointer, pointeur de pile).
- **ROM/Flash** : memoire non volatile, contient le code executable.
- **RAM** : memoire volatile lecture/ecriture, contient les variables du
  programme et les valeurs initialisees copiees depuis la ROM au demarrage
  (mecanisme deja rencontre concretement : c'est exactement ce que fait
  `Reset_Handler` dans `vendor/startup/startup_stm32f303x8.s` en copiant
  `.data` de la Flash vers la RAM avant `main()`).
- **Pile systeme (stack)** : structure LIFO en RAM, stocke les infos
  critiques lors des interruptions et des appels de fonction (adresse de
  retour, registres sauvegardes).

**Familles de processeurs :**
- **Microprocesseur** : multi-coeurs, haute puissance, necessite un OS -
  pas adapte a l'embarque pur.
- **DSP** (Digital Signal Processing) : optimise pour le traitement rapide
  du signal.
- **Microcontroleur** : puissance moindre, peripheriques integres - le
  choix standard pour l'embarque (c'est la categorie du STM32F103RB comme
  du STM32F303K8).
- **FPGA** : circuit logique reconfigurable, tres flexible mais necessite
  un langage de programmation different (VHDL/Verilog), hors perimetre de
  ce cours.

### Partie 2 - Decouvrez les grandes lignes de l'architecture programmable ARM
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4604956-decouvrez-les-grandes-lignes-de-l-architecture-programmable-arm)

**ARM (l'entreprise)** concoit des architectures de coeur processeur mais
ne fabrique rien elle-meme : elle licencie ses designs a des fondeurs qui
y ajoutent memoire et peripheriques pour creer des familles de produits
(ex. la famille STM32F10x... et par extension STM32F3xx pour notre carte).

**Architecture Harvard** : separe physiquement les acces a la "memoire de
code" (programme) et a la "memoire de donnees" (variables) - contrairement
a une architecture Von Neumann qui partage un bus unique pour les deux.

**Jeu de registres (ARMv7, 32 bits) - 17 registres :**
- **R0-R12** : 13 registres generaux
- **SP** (R13, Stack Pointer), **LR** (R14, Link Register - adresse de
  retour d'un appel de fonction), **PC** (R15, Program Counter)
- **xPSR** (registre d'etat, variantes APSR/EPSR/IPSR)
- Registres de controle additionnels : PRIMASK, FAULTMASK, BASEPRI,
  CONTROL (gestion des priorites d'interruption, mode privilegie/non
  privilegie)

**RISC** (Reduced Instruction Set Computer) : jeu d'instructions reduit et
simple plutot que complexe (CISC). L'ALU fait de l'arithmetique entiere
(add/sub/mul/div) et de la logique (comparaison, AND, OR).

**Architecture load/store** : la memoire n'est accessible que par 2
instructions dediees, **LDR** (Load Register - charge un registre depuis
la memoire) et **STR** (STore Register - ecrit un registre en memoire).
Aucune operation ALU ne travaille directement sur la memoire : tout doit
d'abord passer par un registre. C'est le mecanisme derriere l'acces aux
peripheriques memory-mapped (ex. `GPIOB->ODR = ...` se traduit en un
`STR` vers l'adresse du registre).

**Flags de statut (registre xPSR)**, mis a jour optionnellement par une
instruction via le suffixe **S** :
- **C** (Carry) : retenue/depassement non signe
- **Z** (Zero) : resultat nul
- **N** (Negative) : bit de signe du resultat
- **V** (oVerflow) : depassement signe
- **Q** (saturation) : utilise avec les instructions USAT/SSAT

**Versions d'architecture** : ARMv7 (standard 32 bits actuel, celle du
cours et de nos deux cartes - Cortex-M3 pour le F103RB, Cortex-M4F pour
le F303K8, tous deux ARMv7-M) vs ARMv8 (2014, permet le 64 bits - hors
perimetre ici).

### Partie 2 - Explorez la memoire dans les architectures ARM
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4605171-explorez-la-memoire-dans-les-architectures-arm)

**Alignement "doublement pair"** : une valeur 32 bits doit commencer a une
adresse dont les 2 derniers bits sont a 0 (adresse multiple de 4) pour
fonctionner correctement. Exemple du cours : une variable `int Locale`
placee au tout debut de la RAM du STM32 occupe `0x20000000`-`0x20000003`
(4 octets), un tableau de `char Carac` juste apres. `0x20000000` est
justement l'adresse de debut de la RAM sur nos deux cartes (F103RB comme
F303K8) - meme si la taille totale de RAM differe (20K vs 12K, cf.
`docs/correspondance-f103-f303.md`).

**Little-endian** : ARM range l'octet de poids faible a l'adresse la plus
basse ("a l'envers" par rapport a une lecture naturelle). Une valeur 32
bits est donc stockee en 4 octets dans l'ordre inverse de sa valeur
mathematique. Un tableau de `char` (1 octet/element) n'est lui pas
concerne, chaque element tenant sur une seule adresse.

**Modes d'adressage memoire** (utilises par `LDR`/`STR`, cf. page
precedente sur l'architecture load/store) :
1. **Indirection directe** : `LDR Rt,[Rn]` - lit a l'adresse contenue dans Rn
2. **Avec offset** : `LDR Rt,[Rn,#±imm8]` - ajoute un decalage sans modifier Rn
3. **Indexe par registre** : `LDR Rt,[Rn,Rm]` - ideal pour parcourir un tableau
4. **Post-incrementation** : `LDR Rt,[Rn],#±imm8` - Rn modifie APRES le transfert
5. **Pre-incrementation** : `LDR Rt,[Rn,#±imm8]!` - Rn modifie AVANT le transfert

**Pool litteral** : une valeur immediate trop grande pour tenir dans
l'encodage d'une instruction est stockee juste apres le code, et chargee
via un acces indirect relatif au PC (Program Counter).

### Partie 2 - Utilisez les procedures et la pile systeme
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4610331-utilisez-les-procedures-et-la-pile-systeme)

**Appel de procedure** : l'instruction **BL** (Branch and Link) saute vers
une fonction en memorisant l'adresse de retour dans **LR** (Link
Register). Le retour se fait via **BX LR** (saut a l'adresse contenue
dans LR).

**Convention d'appel (registres)** :
- **R0-R3** : jusqu'a 4 arguments d'entree, dans l'ordre
- **R0** (ou R0-R1 pour un resultat 64 bits) : valeur de retour
- **LR** : adresse de retour, ecrite automatiquement par `BL`

**Pile systeme (SP, LIFO)** :
- **PUSH** : decremente SP de 4 (pre-decrement), PUIS ecrit la donnee
- **POP** : lit a l'adresse pointee par SP, PUIS incremente SP de 4
- Taille par defaut : **1024 octets** definis dans le fichier de demarrage
  - correspond exactement a `_Min_Stack_Size = 0x400;` (0x400 = 1024) dans
    notre `common/linker/STM32F303K8Tx_FLASH.ld` : meme convention que le
    cours, deja en place dans notre projet sans qu'on l'ait nomme ainsi.

**A quoi sert la pile, concretement :**
1. **Sauvegarder LR avant un appel imbrique** : si une fonction A appelle
   une fonction B, LR est ecrase par l'appel a B. A doit donc faire
   `PUSH {LR}` en entree et `POP {PC}` en sortie (restaure LR ET fait le
   retour en une seule instruction, puisqu'on pop directement dans PC).
2. **Arguments au-dela de 4** : les arguments supplementaires (au-dela de
   R0-R3) sont places sur la pile, lus par la fonction appelee via un
   adressage relatif a SP.
3. **Variables locales volumineuses** (tableaux, etc.) : le compilateur
   decremente SP au prologue de la fonction (reserve de l'espace) et le
   restaure a l'epilogue ; acces via offset relatif a SP.

Point cle : "la pile systeme est une zone memoire commune a l'ensemble de
l'application embarquee" - une seule pile partagee par tout le programme
(main + toutes les fonctions + gestionnaires d'interruption), pas une pile
par fonction.

### Partie 2 - Maitrisez les exceptions et les interruptions
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4636746-maitrisez-les-exceptions-et-les-interruptions)

**Exceptions vs interruptions** : meme mecanisme de traitement pour 3 cas
distincts - defaillance materielle (exception), erreur d'execution
(exception), demande d'un peripherique (interruption).

**Table des vecteurs d'interruption (IVT)** : reside a l'adresse
`0x00000000`, jusqu'a 255 entrees sur Cortex-M3. 2 entrees obligatoires :
position 0 = SP initial, position 1 = adresse de reset (PC). Construite a
la compilation/edition de liens, avec des gestionnaires par defaut
(symboles **weak**) redefinissables par l'utilisateur - **exactement le
mecanisme deja utilise dans `vendor/startup/startup_stm32f303x8.s`** :
chaque `..._IRQHandler` y est declare `.weak` et alias vers
`Default_Handler`, donc definir une fonction `void TIM2_IRQHandler(void)`
dans notre propre code l'ecrase automatiquement sans erreur de linker.

**Deroulement lors d'une interruption :**
1. L'instruction en cours se termine
2. Le CPU sauvegarde automatiquement R0-R3, R12, l'adresse de retour,
   xPSR et LR sur la pile systeme (empilage automatique - pas besoin de
   `PUSH` explicite comme pour un appel de fonction classique)
3. Un code special (`0xFFFFFFFx`) est place dans LR, signalant un retour
   d'interruption
4. Le CPU lit le numero d'interruption et recupere l'adresse du
   gestionnaire correspondant dans l'IVT
5. Cette adresse est chargee dans PC

**Retour d'interruption** : `BX LR` avec la valeur speciale dans LR est
reconnu par le Cortex-M comme une sortie d'interruption (restaure
automatiquement les registres empiles a l'etape 2).

**NVIC (Nested Vectored Interrupt Controller)** - gere priorites et
autorisations :
- Priorites : 0-255 en theorie, mais **0-15 sur STM32** (valeur plus
  basse = priorite plus haute)
- `ISER`/`ICER` : active/desactive une interruption donnee
- `ISPR`/`ICPR` : force/efface l'etat "en attente" d'une interruption
- `IABR` : lecture seule, indique les interruptions actives
- `IP` : registres de priorite (8 bits par entree)

**Implementer un gestionnaire en C** : signature obligatoire
`void NomDuHandler(void)`, nom impose par la table des vecteurs (ex.
`TIM2_IRQHandler`) - pas de parametres possibles, communication avec le
reste du programme via variables globales (souvent `volatile`).

**Etapes de configuration d'une interruption :**
1. Definir la fonction handler avec le nom exact attendu
2. Regler sa priorite via `NVIC->IP[n]`
3. L'activer via `NVIC->ISER[x]`
4. Configurer le peripherique lui-meme pour qu'il genere l'interruption

Ce chapitre est generique Cortex-M (M3 comme M4F) - seuls les **noms et
numeros d'IRQ** different entre F103 et F303 (deja documente dans
`docs/correspondance-f103-f303.md` §8), le mecanisme NVIC/pile/vecteurs
lui-meme est identique.

### Partie 2 - Faites le lien entre la compilation C et l'assembleur
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4636751-faites-le-lien-entre-la-compilation-c-et-l-assembleur)

**Chaine de compilation (vocabulaire Keil/µVision du cours) :**
1. **Editeur** : saisie du code
2. **Compilateur** : C -> assembleur compatible processeur
3. **Assembleur** : assembleur -> fichiers objets incomplets
4. **Linker (editeur de liens)** : combine les objets, attribue les
   adresses physiques finales
5. **Loader** : transfere l'image du programme en memoire cible
6. **Debugger** : observation/controle a l'execution

**Fichiers produits, avec leur equivalent GNU deja utilise dans ce
projet :**
| Cours (Keil) | Role | Notre equivalent GNU |
|---|---|---|
| `*.lst` | listing, erreurs compil/assemblage | sortie stdout de `arm-none-eabi-gcc` |
| `*.map` | organisation memoire, adresses variables/fonctions | `build/*.map` (deja genere via `-Wl,-Map=...` dans `common.mk`) |
| `*.axf`/`*.elf`/`*.hex` | image executable | `build/*.elf` et `build/*.hex` (deja produits) |
| `*.lib` | bibliotheque d'objets reutilisables | equivalent GNU : `*.a` (via `arm-none-eabi-ar`) - **pas le meme format**, cf. piege `librairie.lib`/Keil deja rencontre sur `tp03` |
| `*.sct`/`*.ld` (scatter file) | specification memoire pour le linker | notre `common/linker/STM32F303K8Tx_FLASH.ld` (syntaxe GNU `MEMORY`/`SECTIONS`, differente du format `.sct` Keil mais meme role) |

**Convention d'appel C/asm** (deja vue en detail dans le chapitre
precedent) : jusqu'a 4 arguments en R0-R3, retour en R0 (ou R0-R1 pour un
resultat 64 bits).

**Assembleur inline** : le cours utilise la syntaxe Keil `__asm { ... }`.
**Avec GCC, la syntaxe est differente** : `asm volatile ("...")` (GNU
extended asm) ou `__asm__(...)`. A garder en tete si on veut un jour
inserer de l'assembleur inline dans un TP - ne pas copier `__asm { }` du
cours tel quel, ca ne compilera pas avec `arm-none-eabi-gcc`.

### Partie 3 - Comprenez les specificites d'une architecture microcontroleur
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4633171-comprenez-les-specificites-d-une-architecture-microcontroleur)

Debut de la Partie 3 (theorie + TP materiel, contrairement a la Partie 2
purement theorique).

**Ce qui distingue un microcontroleur d'un microprocesseur** : systeme
embarque autonome, consommation minimisee, peripheriques integres pour
interagir directement avec le monde physique (pas besoin d'OS).

**Composants integres :**
- **GPIO** : "la partie la plus passive et la moins intelligente" mais le
  point de connexion essentiel avec l'exterieur (tout/rien : eclairage,
  vanne, bouton). Le nombre de broches et le boitier (LQFP, VFQFPN,
  LFBGA...) determinent les capacites d'interfacage - a verifier avant de
  commencer un projet (nous l'avons fait des le debut :
  `docs/correspondance-f103-f303.md` §2, LQFP64 vs LQFP32).
- **Timers/compteurs** : comptent des changements d'etat ; relies a une
  horloge fixe, ils mesurent le temps. Debordement -> interruption,
  registre de rechargement programmable.
- **Watchdog** : surveille la reactivite du logiciel, declenche une
  interruption si le programme ne le "nourrit" pas periodiquement.
- **Capture/Compare** : capture la valeur d'un timer a un evenement
  (mesurer la duree d'appui d'un bouton) ou compare a un seuil.
- **ADC** : tension -> valeur numerique, via multiplexeur (plusieurs
  canaux), caracterise par sa resolution.
- **PWM** : signal carre a rapport cyclique variable, alternative
  economique a un vrai DAC.
- **Bus de communication** : UART/USART, I2C, SPI, CAN, Ethernet, USB.

**Chiffres du cours pour le STM32F103** (a comparer avec notre F303K8,
cf. `docs/correspondance-f103-f303.md`) : jusqu'a 80 GPIO (LQFP100 - le
RB du cours en LQFP64 en a moins), 4 timers generaux, 2 watchdogs, 2
canaux ADC, plusieurs interfaces de communication.

**3 documents de reference a toujours avoir sous la main** (le cours
insiste dessus, on applique deja ce principe) :
- **Datasheet** : correspondance broches <-> peripheriques
- **Reference manual** : description detaillee des peripheriques et
  registres (RM0008 pour F103, **RM0316 pour notre F303**)
- **Programming manual** : architecture du coeur, mecanisme d'interruption

### Partie 3 - Manipulez les registres et les masques
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4633446-manipulez-les-registres-et-les-masques)

Chapitre 100% generique (independant de la carte) - technique de
manipulation de bits en C, deja appliquee dans tous nos TP.

**Operateurs bit-a-bit vs logiques** : `&`/`|`/`^`/`~` (bit-a-bit)
different de `&&`/`||`/`!` (logique, traite l'operande comme un booleen).

**Mettre un bit a 1** : OR avec un masque n'ayant que ce bit a 1 :
```c
init |= (1 << 4);
```
**Mettre un bit a 0** : AND avec le masque INVERSE :
```c
init &= ~(1 << 4);
```
**Tester un bit** : AND puis comparaison :
```c
if (value & (1 << 5)) { /* bit 5 a 1 */ }
```
**Limite importante soulignee par le cours** : impossible de mettre des
bits a 0 ET a 1 en une seule operation - toujours 2 operations separees
(un `&= ~(...)` puis un `|= (...)`, jamais fusionnables).

**Bonne pratique** : toujours construire le masque avec `(1 << position)`
plutot qu'une valeur hexadecimale calculee a la main - moins d'erreurs,
plus lisible. C'est le style deja utilise partout dans nos TP.

### Partie 3 - Configurez les ports d'entree/sortie (GPIO)
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4633881-configurez-les-ports-d-entree-sortie)

**Pas encore code par l'utilisateur - a faire prochainement.** Voici
l'analyse d'adaptation F103RB -> F303K8 pour preparer ce TP.

**Ce que fait l'exemple du cours** : LED sur PA5 (sortie push-pull) +
bouton USER sur PC13 (entree floating), avec la structure `GPIO_TypeDef`
du F103 : `CRL, CRH, IDR, ODR, BSRR, BRR, LCKR` (7 registres). Boucle de
detection de changement d'etat du bouton (comparaison avec l'etat
precedent) qui toggle la LED a chaque appui/relachement.

**Registres GPIO F303K8 (verifies dans `vendor/cmsis/device/stm32f303x8.h`)** :
```c
MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2], BRR
```
10 registres au lieu de 7 - MODER/OTYPER/OSPEEDR/PUPDR remplacent
CRL/CRH (deja rencontre sur `tp02-premier-blink`), et **AFR[2]** est
nouveau (fonctions alternatives, absent du F103 qui utilise `AFIO_MAPR`
a la place - cf. `docs/correspondance-f103-f303.md` §4).

**Bonne nouvelle deja verifiee** : `IDR`, `ODR`, `BSRR` et `BRR` existent
**avec le meme nom et le meme role** sur le F303 - le code de lecture
(`GPIOC->IDR & (1 << 13)`) et d'ecriture (`GPIOA->ODR ^= (1 << 5)`, ou
via `BSRR`/`BRR`) du cours se transpose donc **sans changement de
logique**, seuls le port/la broche changent.

**Ce qui doit vraiment changer :**
1. **Horloge** : `RCC->APB2ENR` (`IOPAEN`/`IOPCEN`) -> `RCC->AHBENR`
   (`GPIOxEN`) - piege deja rencontre sur `tp02`.
2. **Mode de la LED (sortie push-pull)** : remplacer la config CRL/CRH
   4 bits par `MODER` (2 bits, valeur `01` = sortie) - deja fait sur
   `tp02-premier-blink` pour PB3 (LD3), reutilisable telle quelle.
3. **Le bouton USER n'a pas d'equivalent sur le Nucleo-32.** PC13
   n'existe meme pas sur le connecteur du F303K8 (cf.
   `docs/correspondance-f103-f303.md` §2). Il faudra :
   - Choisir une broche libre du connecteur Arduino Nano (ex. **PA0** ou
     **PB0**, cf. `docs/organisation-tp.md` §2) et y cabler un bouton
     externe sur la breadboard.
   - Configurer cette broche en entree (`MODER = 00`, valeur par defaut).
   - **Difference d'approche pour le pull-up/pull-down** : le F103 n'a
     pas de registre dedie - le mode "pull-up/pull-down" de CRL/CRH
     reutilise le bit `ODR` correspondant pour choisir la direction (une
     astuce specifique au F103). Le F303 a un **vrai registre `PUPDR`**
     (2 bits/broche : `00`=aucun, `01`=pull-up, `10`=pull-down) -
     plus propre, pas besoin du detour par `ODR`. Deux options
     equivalentes au resultat du cours :
     - Reproduire l'entree "floating" du cours (`PUPDR = 00`) + une
       resistance de tirage **externe** sur la breadboard (comme le
       bouton B1 du Nucleo-64 original, qui a son pull-up cable sur la
       carte).
     - Ou, plus idiomatique F303 : `PUPDR = 01` (pull-up interne) et
       cabler juste le bouton vers la masse, sans resistance externe.
4. **Vitesse de sortie (`OSPEEDR`)** : le cours mentionne 3 vitesses
   possibles sur le F103 (dans les bits CRL/CRH), recommandant la plus
   basse pour la plupart des cas. Sur F303, c'est le registre separe
   `OSPEEDR` (2 bits/broche) - non configure explicitement dans nos TP
   jusqu'ici (valeur par defaut = vitesse basse au reset, ce qui
   correspond deja a la recommandation du cours).

**Non concerne par ce TP mais a garder en tete** : `AFR[2]` ne sera
utile que lorsque `MODER` = `10` (fonction alternative) - pas necessaire
ici puisque LED et bouton restent en GPIO pur (`MODER` = `01`/`00`).

### Partie 3 - Gerer le temps avec les timers
[Page du cours](https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque/4634846-gerer-le-temps-avec-les-timers)

**Registres cles** : `CR1` (bit 0 = **CEN**, demarre/arrete le compteur),
`PSC` (prescaler, divise l'horloge par `PSC+1`), `ARR` (auto-reload,
seuil de debordement), `CNT` (valeur courante), `SR` (bit 0 = **UIF**,
flag de debordement/update).

**Formule de periode** :
```
T_timer = T_horloge * (PSC+1) * (ARR+1)
```
Exemple du cours (a 72 MHz, pour 1 seconde) : `PSC=7199`, `ARR=9999` ->
`(1/72e6) * 7200 * 10000 = 1.0 s`. **C'est exactement les valeurs deja
utilisees dans `tp05-timer`** - d'ou le debug de cette session : notre
carte tourne encore a **8 MHz** (HSI, pas de PLL configuree, cf. notes
Partie 2), donc avec ces memes PSC/ARR la periode reelle actuelle est
`(1/8e6) * 7200 * 10000 ≈ 9 s`, pas 1 s. `CNT` incremente bien, mais tous
les `7200/8MHz = 900 µs` seulement - d'ou l'impression de "reste a 0" si
on le lit trop tot apres le demarrage (cf. session de debug precedente).
Pour un vrai "1 seconde" sur cette carte en l'etat, il faudrait soit
adapter `PSC`/`ARR` a 8 MHz (`PSC=799, ARR=9999` par exemple, a verifier),
soit configurer le PLL a 72 MHz (futur chapitre horloge).

**Detection de debordement (polling), code du cours (F103, transposable
tel quel a l'exception du nom du bit RCC deja vu pour l'activation
horloge)** :
```c
if (TIM2->SR & TIM_SR_UIF) {
    TIM2->SR = TIM2->SR & ~TIM_SR_UIF;   // toujours effacer le flag
    GPIOA->ODR = GPIOA->ODR ^ (1 << 5);
}
```
Le flag **doit** etre efface manuellement (ecriture a 0), sinon il reste
positionne et signale un debordement en continu meme si un seul a eu
lieu. `TIM_SR_UIF` (bit 0 de `SR`) est confirme identique sur le F303
(verifie dans `stm32f303x8.h`) - transposable tel quel.

**Horloge** : SYSCLK par defaut a 72 MHz sur le F103 du cours (PLL deja
configuree par defaut sur cette carte - **pas le cas sur notre F303K8**,
cf. remarque ci-dessus), qui alimente ensuite les prescalers AHB/APB1/APB2
distribues aux peripheriques. Bonne pratique soulignee par le cours :
desactiver les horloges des peripheriques non utilises pour economiser
l'energie (deja applique implicitement : on n'active que les `RCC_..EN`
strictement necessaires dans chaque TP).

## Prochaines etapes

- [ ] (optionnel, priorite basse) Confirmer `tp01-hello-uart` sur la carte
      via picocom - plus urgent maintenant que le GPIO+debug sont valides
      de bout en bout via `tp02-premier-blink`
- [ ] (optionnel) Nettoyer les logs OpenOCD/GDB entremeles en redirigeant
      la sortie d'OpenOCD vers un fichier dans `common/mk/common.mk`
      (propose, pas encore fait)
- [ ] Ecrire `tp03_premier-projet/src/functions.c` (implementer
      `function1(int)`/`function2(void)` conformement a `functions.h`),
      l'ajouter au build (`SRCS` deja mis a jour dans le Makefile), puis
      `make flash`. Decider si `librairie.lib` (inutilisable) est
      supprime ou garde de cote.
- [ ] Terminer la Partie 2 : quiz de fin de partie ("Les grands principes
      de l'execution") - pas encore fait, la lecture est passee directement
      en Partie 3
- [ ] Finir `tp04-gpio` : config LED (PB3) et bouton joystick (PA0,
      `PUPDR` pull-up) en place, mais la logique de lecture/comparaison
      d'etat (le `while(1)` qui detecte l'appui et toggle la LED) reste a
      ecrire. Renommer aussi `TARGET` dans le Makefile (encore
      `tpXX-nom-du-tp`).
- [ ] `tp05-timer` : ajuster `PSC`/`ARR` pour un vrai 1 seconde a
      l'horloge actuelle (8 MHz) - ou attendre la configuration du PLL a
      72 MHz. Ajouter la logique de detection `UIF` + toggle LED
      (actuellement seule la config CR1/PSC/ARR est en place, pas encore
      de polling dans la boucle). Renommer `TARGET` dans le Makefile.
- [ ] Continuer la Partie 3 : "Gerez vos interruptions" (version timer,
      complementaire au chapitre generique NVIC deja vu en Partie 2),
      puis "Entrainez-vous en allumant une LED de maniere aleatoire" et
      le quiz de fin de partie
- [ ] Terminer le quiz de la Partie 2 (toujours en attente depuis le
      2026-09-08)
- [ ] Une fois plusieurs TP reels faits, reproposer la Skill Claude
      reutilisable (mapping + check-list + gabarit) - reportee le
      2026-09-02 a la demande de l'utilisateur
