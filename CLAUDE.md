# Instructions projet - OC "C pour l'embarque" adapte NUCLEO-F303K8

## Contexte

Ce repo suit le cours OpenClassrooms "Developpez en C pour l'embarque"
(https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque),
concu pour une NUCLEO-F103RB (STM32F103RB, Cortex-M3), mais adapte pour
etre suivi sur une **NUCLEO-F303K8** (STM32F303K8, Cortex-M4F, format
Nucleo-32, moins de broches, registres GPIO differents) + une breadboard
pour compenser les broches/peripheriques absents. Bare-metal/CMSIS pur
(pas de HAL ST complete, pas de framework Arduino), sous Linux (WSL2
Ubuntu 24.04).

L'utilisateur est ingenieur embarque en formation (Centrale Nantes,
alternance EDF), il reactive ses bases (registres, GPIO, timers, UART,
IT) - ce n'est pas un debutant a qui tout expliquer depuis zero, mais
quelqu'un qui veut re-manipuler ces concepts lui-meme.

## Regle de collaboration la plus importante

**Ne pas ecrire le code des exercices/TP a la place de l'utilisateur.**
Guider : pointer les pieges lies au changement de carte (F103->F303),
corriger les erreurs de registre/adresse, expliquer le "pourquoi" d'un
bug, mais laisser l'utilisateur ecrire son `main.c` lui-meme.

Exception explicite deja survenue : `tp01-hello-uart` a ete ecrit
entierement par Claude, mais uniquement parce que l'utilisateur l'a
demande explicitement et exceptionnellement ("uniquement pour cette
fois"), pour valider que l'environnement fonctionnait avant de reprendre
en autonomie. Ne pas generaliser cette exception sans demande explicite
similaire.

En revanche, la mise en place de l'**environnement** (Makefiles, linker
script, CMSIS vendorise, gabarit de TP, scaffolding d'un nouveau dossier
`tpNN-.../`) releve de l'outillage, pas de l'exercice : ca peut etre fait
directement.

## Structure du repo

```
vendor/cmsis/          CMSIS officiel ST (registres) - ne pas modifier
vendor/startup/         Startup + table des vecteurs officielle STM32F303x8
common/linker/          Linker script (Flash 64K / RAM 12K)
common/mk/common.mk     Regles de build partagees, inclues par chaque Makefile de TP
tools/                  Config OpenOCD (target stm32f3x, pas stm32f1x)
template-tp/            Gabarit a copier (cp -r template-tp tpNN-nom) pour un nouveau TP
tpNN-.../               Un dossier par TP/chapitre du cours
docs/correspondance-f103-f303.md   Tableau de correspondance registres/GPIO/timers F103->F303
docs/organisation-tp.md            Decoupage des TP + usage de la breadboard
fiche-suivi-projet-embarque.md     Suivi de projet (voir protocole plus bas)
```

## Commandes de build (depuis un dossier `tpNN-.../`)

```sh
make          # build -> build/<TARGET>.elf/.bin/.hex
make flash    # flash via openocd + ST-LINK (verify + reset)
make debug    # openocd (arriere-plan) + gdb-multiarch attaches
make clean
```

Le ST-LINK doit etre passe de Windows vers WSL2 via `usbipd-win`
(`usbipd bind` en PowerShell **administrateur**, puis `usbipd attach --wsl`)
avant que `make flash`/`make debug` ne fonctionnent - voir
`fiche-suivi-projet-embarque.md` pour le detail et le troubleshooting.

## Pieges F103->F303 les plus frequents

Table complete dans `docs/correspondance-f103-f303.md`. Deja rencontres
concretement pendant les TP :
- Horloge GPIO : `RCC->AHBENR` (bit `GPIOxEN`) sur F303, PAS
  `RCC->APB2ENR`/`IOPxEN` comme sur F103.
- Config GPIO : `MODER`/`OTYPER`/`PUPDR`/`AFR[]` (2 bits/broche pour
  `MODER`) sur F303, PAS `CRL`/`CRH` (4 bits/broche) comme sur F103 -
  un masque `0xF` copie du style F103 efface aussi la broche voisine sur
  `MODER`.
- LED utilisateur : LD3 sur **PB3** (pas PA5/LD2), partagee avec
  SPI1_SCK. Pas de bouton utilisateur GPIO (B1 = RESET uniquement).
- Une boucle de toggle sans delai (ou avec un delai bien trop court)
  clignote a une frequence largement superieure a la persistance
  retinienne humaine -> LED apparemment figee, pas un bug de registre.
- Ressources telechargeables du cours (ex. chapitre "Entrainez-vous en
  creant un projet") : certaines sont liees a Keil MDK-ARM/µVision
  (fichiers `.lib` compiles avec `armcc`, format incompatible avec
  `arm-none-eabi-gcc`/GNU ld) - verifier avec `file`/`xxd` avant de
  supposer une erreur de manipulation.

## Protocole de la fiche de suivi (`fiche-suivi-projet-embarque.md`)

- **Debut de session** : la lire pour se resynchroniser sur l'etat reel
  avant de proposer quoi que ce soit.
- **Pendant la session** : consigner **chaque commande shell** utilisee
  pour un TP (scaffold, build, flash, git...) dans la section "Journal
  des commandes (par TP)", au fil de l'eau - pas seulement en resume a la
  fin. Si Claude execute une commande via son propre outil Bash, ou si
  l'utilisateur rapporte une commande qu'il a lancee lui-meme, elle va
  dans le journal.
- **Fin de session** (l'utilisateur dit qu'il arrete) : mettre a jour le
  tableau "Historique / Etat d'avancement" (ligne datee) et "Prochaines
  etapes", puis **committer ce fichier seul, dans un commit separe** du
  code de TP eventuellement modifie dans la session.

## Conventions de commit

- Le code des TP : l'utilisateur gere ses propres commits (guider avec
  les commandes `git add`/`commit` si demande, ne pas commit a sa place
  sauf demande explicite).
- La fiche de suivi : commit gere par Claude en fin de session, separement.

## Style de communication attendu

Expliquer les elements non-deja-vus dans les commandes shell donnees a
l'utilisateur (flags, outils) - pas besoin de re-expliquer un element
deja explique plus tot dans la conversation.
