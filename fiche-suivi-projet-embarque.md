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

Voir aussi :
- [`docs/correspondance-f103-f303.md`](docs/correspondance-f103-f303.md) -
  tableau de correspondance registres/GPIO/timers F103 -> F303
- [`docs/organisation-tp.md`](docs/organisation-tp.md) - decoupage des TP et
  usage de la breadboard

## Historique / Etat d'avancement

| Date | Fait |
|---|---|
| 2026-09-02 | Setup initial : verification toolchain (arm-none-eabi-gcc 13.2.1, gdb-multiarch 15.1, openocd 0.12.0, make 4.3 - tous deja presents) ; mise en place environnement bare-metal (CMSIS ST vendorise dans `vendor/`, linker script `common/linker/STM32F303K8Tx_FLASH.ld` pour Flash 64K/RAM 12K, startup officiel `startup_stm32f303x8.s`, Makefile partage `common/mk/common.mk`) ; build de `tp00-smoke-test` (blink LD3/PB3) valide avec succes en local (compile+link, non flashe : ST-LINK pas encore visible depuis WSL) ; init du repo Git avec `.gitignore` ; tableau de correspondance F103RB->F303K8 redige (`docs/correspondance-f103-f303.md`) ; proposition de decoupage des TP + usage breadboard (`docs/organisation-tp.md`) ; gabarit de TP reutilisable (`template-tp/`). |

## Procedures d'installation / reprise

### Toolchain (deja installee sur cette machine)
```sh
sudo apt install gcc-arm-none-eabi gdb-multiarch openocd make
```

### ST-LINK depuis WSL2
Le ST-LINK n'est **pas visible par defaut** depuis WSL2 (USB non partage par
Hyper-V/WSLg). Cote Windows (PowerShell admin) :
```powershell
winget install --interactive --exact dorssel.usbipd-win
usbipd list                              # reperer le BUSID du ST-LINK
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```
Puis verifier cote WSL avec `lsusb` (vendor `0483`) et
`openocd -f tools/openocd_f303k8.cfg`. A refaire a chaque redemarrage
Windows (ou `usbipd attach --auto-attach` pour automatiser).

### Reprendre un TP
```sh
cd tpNN-nom-du-tp
make            # build
make flash      # flash (necessite ST-LINK visible, voir ci-dessus)
make debug      # openocd + gdb-multiarch
```

## Prochaines etapes

- [ ] Faire le passage USB ST-LINK -> WSL2 (usbipd-win, cote Windows) et
      confirmer `lsusb` + flash reel de `tp00-smoke-test` sur la carte
- [ ] Attaquer Partie 1 / Partie 3 du cours (GPIO) -> `tp02-gpio` en suivant
      `docs/correspondance-f103-f303.md` §4 pour la config MODER/AFR
- [ ] Une fois le mapping + la check-list de setup stabilises, les
      transformer en Skill Claude reutilisable (demande initiale, etape 8)
