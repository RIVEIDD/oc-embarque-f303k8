# Cours "Developpez en C pour l'embarque" (OpenClassrooms) - adapte NUCLEO-F303K8

Suivi du cours https://openclassrooms.com/fr/courses/4117396-developpez-en-c-pour-l-embarque
(cible d'origine : NUCLEO-F103RB) sur une carte **NUCLEO-F303K8**
(STM32F303K8, Cortex-M4F, Nucleo-32) + breadboard.

Voir [`fiche-suivi-projet-embarque.md`](fiche-suivi-projet-embarque.md) pour
l'etat d'avancement et [`docs/correspondance-f103-f303.md`](docs/correspondance-f103-f303.md)
pour le detail des differences F103 -> F303 (registres, GPIO, timers...).

## Structure du repo

```
vendor/cmsis/        Fichiers CMSIS officiels ST (registres) - ne pas modifier
vendor/startup/       Startup + table des vecteurs officielle STM32F303x8
common/linker/         Linker script (Flash 64K / RAM 12K)
common/mk/common.mk    Regles de build partagees (include depuis chaque TP)
tools/                Config OpenOCD (ST-LINK / STM32F3)
template-tp/           Gabarit a copier pour demarrer un nouveau TP
tp00-smoke-test/       Verification de la chaine de build (pas un TP du cours)
tpXX-.../               Un dossier par TP du cours, cree au fur et a mesure
docs/                  Tableau de correspondance F103->F303, notes
```

## Demarrer un nouveau TP

```sh
cp -r template-tp tp01-nom-du-tp
# editer tp01-nom-du-tp/Makefile : TARGET = tp01-nom-du-tp
# editer tp01-nom-du-tp/src/main.c
cd tp01-nom-du-tp
make            # build -> build/tp01-nom-du-tp.elf/.bin/.hex
make flash      # flash via openocd + ST-LINK
make debug      # openocd (arriere-plan) + gdb-multiarch attaches
make clean
```

## ST-LINK depuis WSL2

Le ST-LINK USB doit etre partage depuis Windows vers WSL2 via **usbipd-win**
(cote Windows, PowerShell en administrateur) :

```powershell
# 1. Installer usbipd-win (une seule fois)
winget install --interactive --exact dorssel.usbipd-win

# 2. Lister les peripheriques USB, reperer le ST-LINK ("STMicroelectronics
#    STLink" ou "STM32 STLink")
usbipd list

# 3. Partager puis attacher le peripherique a la distro WSL (BUSID a adapter)
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

Cote WSL, verifier ensuite avec `lsusb` (vendor `0483` = STMicroelectronics)
puis `openocd -f tools/openocd_f303k8.cfg` pour confirmer la connexion.
A refaire a chaque redemarrage de Windows/WSL (ou automatiser avec
`usbipd attach --auto-attach`).

## Toolchain requise

`arm-none-eabi-gcc`, `gdb-multiarch`, `openocd`, `make` (verifies presents
sur cette machine). Si manquants :

```sh
sudo apt install gcc-arm-none-eabi gdb-multiarch openocd make
```
