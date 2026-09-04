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
- [ ] Poursuivre le cours en autonomie guidee (Partie 3 : timers,
      interruptions) -> nouveaux `tpNN-...` via `template-tp/`, cf.
      `docs/correspondance-f103-f303.md` et `docs/organisation-tp.md`
- [ ] Une fois plusieurs TP reels faits, reproposer la Skill Claude
      reutilisable (mapping + check-list + gabarit) - reportee le
      2026-09-02 a la demande de l'utilisateur
