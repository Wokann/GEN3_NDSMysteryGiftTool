/*
 * savegame_manager: a tool to backup and restore savegames from Nintendo
 *  DS cartridges. Nintendo DS and all derivative names are trademarks
 *  by Nintendo. EZFlash 3-in-1 is a trademark by EZFlash.
 *
 * display.cpp: A collection of shared functions used to print various
 *    status messages and feedback on the screens.
 *
 * Copyright (C) Pokedoc (2010)
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "display.h"

#include <fat.h>
#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "auxspi.h"
#include "fileselect.h"
#include "gba.h"
#include "globals.h"
#include "hardware.h"
#include "languages.h"
#include "strings.h"
#include "supported_games.h"

// some more recent versions no longer define this macro...
#ifndef MAXPATHLEN
#define MAXPATHLEN 128
#endif

PrintConsole upperScreen;
PrintConsole lowerScreen;

//===========================================================
void displayInit() {
  videoSetMode(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  consoleInit(&upperScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true,
              true);

  videoSetModeSub(MODE_0_2D);
  vramSetBankC(VRAM_C_SUB_BG);
  consoleInit(&lowerScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false,
              true);
}

//===========================================================
//ENG_TEXT_START
//===========================================================
//
void displayTitle() {
  displayMessageF(STR_TITLE_MSG, VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO,
                  VERSION_EXTRA);

  displayStateF(STR_STR, "Press (B) to continue");
  while (!(keysCurrent() & KEY_B))
    ;
}

void displayPrintUpper(bool fc) {
  bool gba = (mode == 1);
  u32 dstype = (mode == 3) ? 1 : 0;

  // print upper screen (background)
  consoleSelect(&upperScreen);
  consoleSetWindow(&upperScreen, 0, 0, 32, 24);
  consoleClear();
  iprintf("Mode     :\n");
  iprintf("Memory   :\n");
  iprintf("--- SLOT 1 ---------------------");
  iprintf("Game ID  :\n");
  iprintf("Game name:\n");
  iprintf("Game save:\n");
  iprintf("Special  :\n");
  if (dstype == 1) {
    // DSi mode
    iprintf("--- SD-SLOT --------------------");
    iprintf("Status   :\n");
  } else {
    // old DS phat/lite
    iprintf("--- SLOT 2 ---------------------");
    iprintf("Game ID  :\n");
    iprintf("Game name:\n");
    iprintf("Game save:\n");
    iprintf("Special  :\n");
  }

  // print upper screen
  consoleSetWindow(&upperScreen, 10, 3, 22, 4);
  consoleClear();
  consoleSetWindow(&upperScreen, 10, 8, 22, 4);
  consoleClear();

  // fetch cartridge header (maybe, calling "cardReadHeader" on a FC messes with
  // libfat!)
  sNDSHeader nds;
  if (!fc && (slot_1_type != AUXSPI_FLASH_CARD))
    cardReadHeader((uint8 *)&nds);
  else
    slot_1_type = AUXSPI_FLASH_CARD;

  char name[MAXPATHLEN];
  // 0) print the mode
  consoleSetWindow(&upperScreen, 10, 0, 20, 1);
  switch (mode) {
    case 0:
      sprintf(&name[0], "WiFi/FTP");
      break;
    case 1:
      sprintf(&name[0], "GBA");
      break;
    case 2:
      sprintf(&name[0], "3in1");
      break;
    case 3:
      if (sdslot) {
        sprintf(&name[0], "DSi/SD");
      } else {
        sprintf(&name[0], "DSi/iEvo");
      }
      break;
    case 4:
      sprintf(&name[0], "Slot 2");
      break;
    case 5:
      sprintf(&name[0], "Download Play");
      break;
    default:
      sprintf(&name[0], "* ??? *");
      break;
  }
  consoleClear();
  iprintf("%s", name);
  // 0.5) memory buffer size
  consoleSetWindow(&upperScreen, 10, 1, 20, 1);
  iprintf("%lu kB", size_buf >> 10);

  // 1) The cart id.
  consoleSetWindow(&upperScreen, 10, 3, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    memcpy(&name[0], &nds.gameCode[0], 4);
    name[4] = 0x00;
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 2) The cart name.
  consoleSetWindow(&upperScreen, 10, 4, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    memcpy(&name[0], &nds.gameTitle[0], 12);
    name[12] = 0x00;
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 3) The save type
  consoleSetWindow(&upperScreen, 10, 5, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    uint8 type = auxspi_save_type(slot_1_type);
    uint8 size = auxspi_save_size_log_2(slot_1_type);
    // some debug output may need this so iprintf prints to the correct region
    consoleSetWindow(&upperScreen, 10, 5, 22, 1);
    switch (type) {
      case 1:
        sprintf(&name[0], "Eeprom (%i Bytes)", size);
        break;
      case 2:
        sprintf(&name[0], "FRAM (%i kB)", 1 << (size - 10));
        break;
      case 3:
        if (size == 0)
          sprintf(&name[0], "Flash (ID:%lx)",
                  auxspi_save_jedec_id(slot_1_type));
        else
          sprintf(&name[0], "Flash (%i kB)", 1 << (size - 10));
        break;
      default:
        sprintf(&name[0], "???");
        break;
    }
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 4) Special properties (infrared device...)
  consoleSetWindow(&upperScreen, 10, 6, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  switch (slot_1_type) {
    case AUXSPI_INFRARED:
      sprintf(&name[0], "Infrared");
      break;
    case AUXSPI_BBDX:
      sprintf(name, "XXL");
      break;
    case AUXSPI_BLUETOOTH:
      sprintf(name, "Bluetooth");
      break;
    default:
      sprintf(&name[0], "----");
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // Slot 2/SD status
  if (dstype == 1) {
    consoleSetWindow(&upperScreen, 10, 8, 22, 1);
    consoleClear();
    memset(&name[0], 0, MAXPATHLEN);
    // Test if we booted from sudokuhax/DSi Homebrew Channel,
    //  which means that the SD-slot is accessible.
    if (sdslot)
      iprintf("Available.");
    else
      iprintf("LOCKED.");

    return;
  }
  // 5) GBA game id
  consoleSetWindow(&upperScreen, 10, 8, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash) {
    if (ezflash == 0x89168916)
      sprintf(name, "3in1 (512M)");
    else if (ezflash == 0x227E2218)
      sprintf(name, "3in1 (256M V2)");
    else if (ezflash == 0x227E2202)
      sprintf(name, "3in1 (256M V1)");
    else
      sprintf(name, "3in1 (???M)");
  } else if (gba)
    sprintf(name, "%.4s", (char *)0x080000ac);
  else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf("%s", name);

  // 6) GBA game name
  consoleSetWindow(&upperScreen, 10, 9, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "3in1");
  else if (gba)
    sprintf(name, "%.12s", (char *)0x080000a0);
  else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);

  // 7) GBA save size
  consoleSetWindow(&upperScreen, 10, 10, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "SRAM");
  else if (gba) {
    saveTypeGBA type = GetSlot2SaveType(CART_GBA_GAME);
    u8 size = gbaGetSaveSizeLog2(type);
    switch (type) {
      case SAVE_GBA_EEPROM_05:
      case SAVE_GBA_EEPROM_8:
        sprintf(name, "EEPROM (%i bytes)", 1 << size);
        break;
      case SAVE_GBA_SRAM_32:
        sprintf(name, "SRAM (%i kB)", 1 << (size - 10));
        break;
      case SAVE_GBA_FLASH_64:
      case SAVE_GBA_FLASH_128:
        sprintf(name, "Flash (%i kB)", 1 << (size - 10));
        break;
      default:
        sprintf(name, "(none)");
    }
  } else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);

  // 8) GBA special stuff
  consoleSetWindow(&upperScreen, 10, 11, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "NOR + PSRAM");
  else if (gba)
    // TODO: test for RTC, add function for syncing RTC?
    sprintf(name, "???");
  else if (slot2 > 0)
    sprintf(name, "----");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);
}

void displayChangeCart(int mode) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  iprintf("\n\n");
  if (mode)
    printf("Inserted cartridge is not valid!\n\n");
  else
    printf("\n\n");
  iprintf("Please insert one of these and\npress START:\n\n");
  iprintf("     - Pokemon Ruby\n");
  iprintf("     - Pokemon Sapphire\n");
  iprintf("     - Pokemon Emerald\n");
  iprintf("     - Pokemon FireRed\n");
  iprintf("     - Pokemon LeafGreen\n");  
}

void displayLoadingCart() {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();
  printf("Loading cartridge....");
}

void sleep(int seconds) {
  time_t calltime = time(NULL);
  time_t timenow;

  while (true) {
    timenow = time(NULL);
    if (timenow - calltime >= seconds) break;
  }
}
void displayPrintTicketError(int error) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();
  iprintf("\n\n\n\n\n\n\n\n\n\n");
  switch (error) {
    case -1:
      iprintf("This is not a valid\nRu/Sa/Em/FR/LG save file!\n");
      break;
    case -2:
      iprintf("Mistery Event is not enabled\nin savegame!\n");
      break;
    case -3:
      iprintf("Mistery Gift is not enabled\nin savegame!\n");
      break;
  }
  
  sleep(5);
}

void displayPrintTickets(int cursor_position, SupportedGames games, Language language) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  iprintf("Select your event:\n\n");

  switch (language) {
    case JAPANESE:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    Eon Ticket\n");
          iprintf("    E-Berry: �����(Pumkin)\n");
          iprintf("    E-Berry: �����(Drash )\n");
          iprintf("    E-Berry: �����(Eggant)\n");
          iprintf("    E-Berry: �����(Strib )\n");
          iprintf("    E-Berry: ���� (Chilan)\n");
          iprintf("    E-Berry: �����(Nutpea)\n");
          iprintf("    E-Berry: �����(Ginema)\n");
          iprintf("    E-Berry: ���� (Kuo   )\n");
          iprintf("    E-Berry: ���� (Yago  )\n");
          iprintf("    E-Berry: �����(Touga )\n");
          iprintf("    E-Berry: �����(Niniku)\n");
          iprintf("    E-Berry: ���� (Topo  )\n");
          break;
        case EMERALD:
          iprintf("    Eon Ticket\n");
          iprintf("    Mystic Ticket 2005\n");
          iprintf("    Old Sea Map\n");
          iprintf("    Aurora Ticket (unofficial)\n");
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    Aurora Ticket 2004\n");
          iprintf("    Mystic Ticket 2005\n");
          break;
      }
      break;
    case ENGLISH:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    Eon Ticket (e-card)\n");
          iprintf("    Eon Ticket (nintendo Italy)\n");
          iprintf("    E-Berry: Pumkin(�����)\n");
          iprintf("    E-Berry: Drash (�����)\n");
          iprintf("    E-Berry: Eggant(�����)\n");
          iprintf("    E-Berry: Strib (�����)\n");
          iprintf("    E-Berry: Chilan(���� )\n");
          iprintf("    E-Berry: Nutpea(�����)\n");
          break;
        case EMERALD:
          iprintf("    Aurora Ticket\n");
          iprintf("    Mystic Ticket\n");
          iprintf("    Old Sea Map (unofficial)\n");
          iprintf("    Eon ticket (unofficial)\n");
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    Aurora Ticket\n");
          iprintf("    Mystic Ticket\n");
          break;
      }
      break;
    default:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    Eon Ticket\n");
          iprintf("    E-Berry: Pumkin(�����)\n");
          iprintf("    E-Berry: Drash (�����)\n");
          iprintf("    E-Berry: Eggant(�����)\n");
          iprintf("    E-Berry: Strib (�����)\n");
          iprintf("    E-Berry: Chilan(���� )\n");
          iprintf("    E-Berry: Nutpea(�����)\n");
          break;
        case EMERALD:
          iprintf("    Aurora Ticket\n");
          iprintf("    Mystic Ticket (USA)\n");
          iprintf("    Old Sea Map (unofficial)\n");
          iprintf("    Eon ticket (unofficial)\n");
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    Aurora Ticket\n");
          iprintf("    Mystic Ticket (USA)\n");
          break;
      }
      break;
  }
  printf("\n\nPress START to change cartridge");
  // Print cursor
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  iprintf("\n\n");
  int i = 0;
  for (i = 0; i < cursor_position; i++) {
    iprintf("\n");
  }
  iprintf("-->");
}
//
//===========================================================
//ENG_TEXT_END
//===========================================================

//===========================================================
//CHS_TEXT_START
//===========================================================
/*
void displayTitle() {
  displayMessageF(STR_TITLE_MSG, VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO,
                  VERSION_EXTRA);

  displayStateF(STR_STR, "�(B)���!             ");//"按(B)!"
  while (!(keysCurrent() & KEY_B))
    ;
}

void displayPrintUpper(bool fc) {
  bool gba = (mode == 1);
  u32 dstype = (mode == 3) ? 1 : 0;

  // print upper screen (background)
  consoleSelect(&upperScreen);
  consoleSetWindow(&upperScreen, 0, 0, 32, 24);
  consoleClear();
  iprintf("��       :\n");
  iprintf("��       :\n");
  iprintf("---  ��  1 ---------------------");
  iprintf("�� ID    :\n");//"游戏ID     :"
  iprintf("�� name  :\n");//"游戏Name   :"
  iprintf("�� ��    :\n");//"游戏存档     :"
  iprintf("��       :\n");
  if (dstype == 1) {
    // DSi mode
    iprintf("--- SD-SLOT --------------------");
    iprintf("Status   :\n");
  } else {
    // old DS phat/lite
    iprintf("---  ��  2 ---------------------");
    iprintf("�� ID    :\n");//"游戏ID     :"
    iprintf("�� name  :\n");//"游戏Name   :"
    iprintf("�� ��    :\n");//"游戏存档     :"
    
    iprintf("��       :\n");
  }

  // print upper screen
  consoleSetWindow(&upperScreen, 10, 3, 22, 4);
  consoleClear();
  consoleSetWindow(&upperScreen, 10, 8, 22, 4);
  consoleClear();

  // fetch cartridge header (maybe, calling "cardReadHeader" on a FC messes with
  // libfat!)
  sNDSHeader nds;
  if (!fc && (slot_1_type != AUXSPI_FLASH_CARD))
    cardReadHeader((uint8 *)&nds);
  else
    slot_1_type = AUXSPI_FLASH_CARD;

  char name[MAXPATHLEN];
  // 0) print the mode
  consoleSetWindow(&upperScreen, 10, 0, 20, 1);
  switch (mode) {
    case 0:
      sprintf(&name[0], "WiFi/FTP");
      break;
    case 1:
      sprintf(&name[0], "GBA");
      break;
    case 2:
      sprintf(&name[0], "3in1");
      break;
    case 3:
      if (sdslot) {
        sprintf(&name[0], "DSi/SD");
      } else {
        sprintf(&name[0], "DSi/iEvo");
      }
      break;
    case 4:
      sprintf(&name[0], "Slot 2");
      break;
    case 5:
      sprintf(&name[0], "Download Play");
      break;
    default:
      sprintf(&name[0], "* ??? *");
      break;
  }
  consoleClear();
  iprintf("%s", name);
  // 0.5) memory buffer size
  consoleSetWindow(&upperScreen, 10, 1, 20, 1);
  iprintf("%lu kB", size_buf >> 10);

  // 1) The cart id.
  consoleSetWindow(&upperScreen, 10, 3, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    memcpy(&name[0], &nds.gameCode[0], 4);
    name[4] = 0x00;
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 2) The cart name.
  consoleSetWindow(&upperScreen, 10, 4, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    memcpy(&name[0], &nds.gameTitle[0], 12);
    name[12] = 0x00;
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 3) The save type
  consoleSetWindow(&upperScreen, 10, 5, 22, 1);
  sprintf(&name[0], "----");
  if (slot_1_type == AUXSPI_FLASH_CARD) {
    sprintf(&name[0], "Flash Card");
  } else {
    uint8 type = auxspi_save_type(slot_1_type);
    uint8 size = auxspi_save_size_log_2(slot_1_type);
    // some debug output may need this so iprintf prints to the correct region
    consoleSetWindow(&upperScreen, 10, 5, 22, 1);
    switch (type) {
      case 1:
        sprintf(&name[0], "Eeprom (%i Bytes)", size);
        break;
      case 2:
        sprintf(&name[0], "FRAM (%i kB)", 1 << (size - 10));
        break;
      case 3:
        if (size == 0)
          sprintf(&name[0], "Flash (ID:%lx)",
                  auxspi_save_jedec_id(slot_1_type));
        else
          sprintf(&name[0], "Flash (%i kB)", 1 << (size - 10));
        break;
      default:
        sprintf(&name[0], "???");
        break;
    }
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // 4) Special properties (infrared device...)
  consoleSetWindow(&upperScreen, 10, 6, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  switch (slot_1_type) {
    case AUXSPI_INFRARED:
      sprintf(&name[0], "Infrared");
      break;
    case AUXSPI_BBDX:
      sprintf(name, "XXL");
      break;
    case AUXSPI_BLUETOOTH:
      sprintf(name, "Bluetooth");
      break;
    default:
      sprintf(&name[0], "----");
  }
  if (dstype == 1) sprintf(name, "LOCKED");
  consoleClear();
  iprintf("%s", name);

  // Slot 2/SD status
  if (dstype == 1) {
    consoleSetWindow(&upperScreen, 10, 8, 22, 1);
    consoleClear();
    memset(&name[0], 0, MAXPATHLEN);
    // Test if we booted from sudokuhax/DSi Homebrew Channel,
    //  which means that the SD-slot is accessible.
    if (sdslot)
      iprintf("Available.");
    else
      iprintf("LOCKED.");

    return;
  }
  // 5) GBA game id
  consoleSetWindow(&upperScreen, 10, 8, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash) {
    if (ezflash == 0x89168916)
      sprintf(name, "3in1 (512M)");
    else if (ezflash == 0x227E2218)
      sprintf(name, "3in1 (256M V2)");
    else if (ezflash == 0x227E2202)
      sprintf(name, "3in1 (256M V1)");
    else
      sprintf(name, "3in1 (???M)");
  } else if (gba)
    sprintf(name, "%.4s", (char *)0x080000ac);
  else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf("%s", name);

  // 6) GBA game name
  consoleSetWindow(&upperScreen, 10, 9, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "3in1");
  else if (gba)
    sprintf(name, "%.12s", (char *)0x080000a0);
  else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);

  // 7) GBA save size
  consoleSetWindow(&upperScreen, 10, 10, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "SRAM");
  else if (gba) {
    saveTypeGBA type = GetSlot2SaveType(CART_GBA_GAME);
    u8 size = gbaGetSaveSizeLog2(type);
    switch (type) {
      case SAVE_GBA_EEPROM_05:
      case SAVE_GBA_EEPROM_8:
        sprintf(name, "EEPROM (%i bytes)", 1 << size);
        break;
      case SAVE_GBA_SRAM_32:
        sprintf(name, "SRAM (%i kB)", 1 << (size - 10));
        break;
      case SAVE_GBA_FLASH_64:
      case SAVE_GBA_FLASH_128:
        sprintf(name, "Flash (%i kB)", 1 << (size - 10));
        break;
      default:
        sprintf(name, "(none)");
    }
  } else if (slot2 > 0)
    sprintf(name, "Flash Card");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);

  // 8) GBA special stuff
  consoleSetWindow(&upperScreen, 10, 11, 22, 1);
  consoleClear();
  memset(&name[0], 0, MAXPATHLEN);
  if (ezflash)
    sprintf(name, "NOR + PSRAM");
  else if (gba)
    // TODO: test for RTC, add function for syncing RTC?
    sprintf(name, "???");
  else if (slot2 > 0)
    sprintf(name, "----");
  else if (dstype == 0)
    sprintf(name, "----");
  if (dstype == 0) iprintf(name);
}

void displayChangeCart(int mode) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  iprintf("\n\n");
  if (mode)
    printf("������!                         \n\n");//"插入卡带无效!"
  else
    printf("\n\n");
  iprintf("���������                     \n�START�:    \n\n");//"插入卡带按START:"
  iprintf("     - ��� ���     \n");//"宝可梦 红宝石"
  iprintf("     - ��� ���         \n");//"宝可梦 蓝宝石"
  iprintf("     - ��� ���        \n");//"宝可梦 绿宝石"
  iprintf("     - ��� ��         \n");//"宝可梦 火红"
  iprintf("     - ��� ��           \n");  //"宝可梦 叶绿"
  
}

void displayLoadingCart() {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();
  printf("����ing...           ");//"载入卡带ing..."
}

void sleep(int seconds) {
  time_t calltime = time(NULL);
  time_t timenow;

  while (true) {
    timenow = time(NULL);
    if (timenow - calltime >= seconds) break;
  }
}
void displayPrintTicketError(int error) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();
  iprintf("\n\n\n\n\n\n\n\n\n\n");
  switch (error) {
    case -1:
      //"红宝石/蓝宝石/绿宝石/火红/叶绿存档无效"
      iprintf("���/���/���/��/��  \n����!                    \n");
      break;
    case -2:
      //"神秘事件未开"
      iprintf("�����                       \n����!       \n");
      break;
    case -3:
      //"神秘礼物未开"
      iprintf("�����                      \n����!       \n");
      break;
  }
  
  sleep(5);
}

void displayPrintTickets(int cursor_position, SupportedGames games, Language language) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  iprintf("����:             \n\n");

  switch (language) {
    case JAPANESE:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    ����      \n");//"无限船票"
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: ���� (���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: ���� (���)      \n");
          iprintf("    E���: ���� (���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: �����(���)      \n");
          iprintf("    E���: ���� (���)      \n");
          break;
        case EMERALD:
          iprintf("    ����      \n");//"无限船票"
          iprintf("    ���� 2005         \n");//"神秘船票 2005"
          iprintf("    ����       \n");//"古航海图"
          iprintf("    ���� (unofficial)         \n");//"极光船票 (unofficial)"
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    ���� 2004         \n");//"极光船票 2004"
          iprintf("    ���� 2005         \n");//"神秘船票 2005"
          break;
      }
      break;
    case ENGLISH:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    ���� (e�)          \n");//"无限船票 (e-card)"
          iprintf("    ���� (nintendo Italy)      \n");//"无限船票 (nintendo Italy)"
          iprintf("    E���: Pumkin(���)     \n");
          iprintf("    E���: Drash (���)     \n");
          iprintf("    E���: Eggant(���)     \n");
          iprintf("    E���: Strib (���)     \n");
          iprintf("    E���: Chilan(���)     \n");
          iprintf("    E���: Nutpea(���)     \n");
          break;
        case EMERALD:
          iprintf("    ����         \n");//"极光船票"
          iprintf("    ����         \n");//"神秘船票"
          iprintf("    ���� (unofficial)       \n");//"古航海图 (unofficial)"
          iprintf("    ���� (unofficial)      \n");//"无限船票 (unofficial)"
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    ����         \n");//"极光船票"
          iprintf("    ����         \n");//"神秘船票"
          break;
      }
      break;
    default:
      switch (games) {
        case RUBY_AND_SAPPHIRE:
          iprintf("    ����      \n");//"无限船票"
          iprintf("    E���: Pumkin(���)     \n");
          iprintf("    E���: Drash (���)     \n");
          iprintf("    E���: Eggant(���)     \n");
          iprintf("    E���: Strib (���)     \n");
          iprintf("    E���: Chilan(���)     \n");
          iprintf("    E���: Nutpea(���)     \n");
          break;
        case EMERALD:
          iprintf("    ����         \n");//"极光船票"
          iprintf("    ���� (USA)         \n");//"神秘船票 (USA)"
          iprintf("    ���� (unofficial)       \n");//"古航海图 (unofficial)"
          iprintf("    ���� (unofficial)      \n");//"无限船票 (unofficial)"
          break;
        case FIRE_RED_AND_LEAF_GREEN:
          iprintf("    ����         \n");//"极光船票"
          iprintf("    ���� (USA)         \n");//"神秘船票 (USA)"
          break;
      }
      break;
  }
  printf("\n\n�START���!                   ");//"按START换卡带"
  // Print cursor
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  iprintf("\n\n");
  int i = 0;
  for (i = 0; i < cursor_position; i++) {
    iprintf("\n");
  }
  iprintf("-->");
}
*/
//===========================================================
//CHS_TEXT_END
//===========================================================

void displayPrintLower(int cursor_position) {
  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();
  iprintf("+------------------------------+");
  for (int i = 0; i < 6; i++) {
    iprintf("|                              |");
  }
  iprintf("+------------------------------+");
  iprintf("+------------------------------+");
  for (int i = 0; i < 6; i++) {
    iprintf("|                              |");
  }
  iprintf("+------------------------------+");
  iprintf("+------------------------------+");
  for (int i = 0; i < 6; i++) {
    iprintf("|                              |");
  }
  iprintf("+------------------------------+");

  consoleSetWindow(&lowerScreen, 1, 1, 30, 6);
  if (cursor_position == 0) {
    iprintf("\n\n       ==>  BACKUP  <==\n");
  } else {
    iprintf("\n\n            BACKUP\n");
  }
  iprintf("         Game -> .sav");

  consoleSetWindow(&lowerScreen, 1, 9, 30, 6);
  if (cursor_position == 1) {
    iprintf("\n\n       ==>  RESTORE  <==\n");
  } else {
    iprintf("\n\n            RESTORE\n");
  }
  iprintf("         .sav -> Game");

  consoleSetWindow(&lowerScreen, 1, 17, 30, 6);
  if (cursor_position == 2) {
    iprintf("\n\n        ==>  RESET  <==\n");
  } else {
    iprintf("\n\n             RESET\n");
  }
  iprintf(stringsGetMessageString(STR_MM_WIPE));
}

void displayPrintState(const char *txt) {
  swiWaitForVBlank();
  consoleSelect(&upperScreen);
  consoleSetWindow(0, 0, 22, 32, 1);
  consoleClear();
  iprintf("%s", txt);
}

// ----- internal function
char *ParseLine(char *start, const char *end, int &length) {
  // This function takes a line and does a quick word-wrap, by fitting it into a
  // certain fixed-length line
  int len = 0;
  char *cur = start;
  char *separator = start;

  while (start < end) {
    if (*start == '\n') {
      length = 0;
      return start;
    }

    // look for a working "start" position
    if ((*start == ' ') || (*start == '\n') || (*start == '\t')) {
      start++;
      cur++;
      separator++;
      continue;
    }

    // do count characters
    if (len < length) {
      cur++;
      len++;
      if ((*cur == ' ') || (*cur == '\n') || (*cur == '\t') || (*cur == '\0')) {
        separator = cur;
        if ((*cur == '\n') || (*cur == '\0')) break;
      }
    } else {
      // emergency exit
      if (!separator) separator = cur;
      break;
    }
  }

  length = separator - start;
  return start;
}
// ------------------

void displayStateF(int id, ...) {
  // prevent flickering
  swiWaitForVBlank();

  va_list argp;
  va_start(argp, id);
  memset(txt, 0, 256);
  vsnprintf(txt, 256, stringsGetMessageString(id), argp);
  va_end(argp);

  consoleSelect(&upperScreen);
  consoleSetWindow(0, 0, 22, 32, 1);
  consoleClear();

  iprintf("%s", txt);
}

void displayProgressBar(int cur, int max0) {
  swiWaitForVBlank();
  consoleSelect(&upperScreen);
  consoleSetWindow(0, 0, 23, 32, 1);
  consoleClear();

  char buffer[33];

  int percent = float(cur) / float(max0) * 100;
  if (percent > 100) percent = 100;
  sprintf(&buffer[14], "%i%%", percent);

  buffer[0] = '[';
  int steps = float(cur) / float(max0) * 30;
  if (steps > 30) steps = 30;
  for (int i = 1; i <= 30; i++) {
    if ((i >= 14) && (i <= 15)) continue;
    if ((i == 16) && (percent >= 10)) continue;
    if ((i == 17) && (percent == 100)) continue;
    if (i <= steps)
      buffer[i] = '#';
    else
      buffer[i] = '-';
  }
  buffer[31] = ']';
  buffer[32] = 0;

  if (max0 == 0) buffer[0] = 0;

  iprintf("%s", buffer);
}

void displayMessageF(int id, ...) {
  va_list argp;
  va_start(argp, id);
  memset(txt, 0, 256);
  vsnprintf(txt, 256, stringsGetMessageString(id), argp);
  va_end(argp);

  consoleSelect(&upperScreen);
  consoleSetWindow(&upperScreen, 0, 15, 32, 7);
  consoleClear();

  char *start = txt;
  char *end = start + strlen(txt);
  for (int i = 0; i < 7; i++) {
    int l;
    l = 32;
    start = ParseLine(start, end, l);
    consoleSetWindow(&upperScreen, 0, i + 15, 32, 1);
    char tmp = *(start + l);
    *(start + l) = '\0';
    iprintf(start);
    *(start + l) = tmp;
    start += l + 1;
    if (start >= end) break;
  }
}

void displayMessage2F(int id, ...) {
  va_list argp;
  va_start(argp, id);
  memset(txt, 0, 256);
  vsnprintf(txt, 256, stringsGetMessageString(id), argp);
  va_end(argp);

  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  char *start = txt;
  char *end = start + strlen(txt);
  for (int i = 0; i < 20; i++) {
    int l;
    l = 28;
    start = ParseLine(start, end, l);
    consoleSetWindow(&lowerScreen, 2, i + 2, 28, 1);
    char tmp = *(start + l);
    *(start + l) = '\0';
    iprintf(start);
    *(start + l) = tmp;
    start += l + 1;
    if (start >= end) break;
  }
}

void displayWarning2F(int id, ...) {
  va_list argp;
  va_start(argp, id);
  memset(txt, 0, 256);
  vsnprintf(txt, 256, stringsGetMessageString(id), argp);
  va_end(argp);

  consoleSelect(&lowerScreen);
  consoleSetWindow(&lowerScreen, 0, 0, 32, 24);
  consoleClear();

  iprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
  for (int i = 0; i < 22; i++) iprintf("!                              !");
  iprintf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

  char *start = txt;
  char *end = start + strlen(txt);
  for (int i = 0; i < 20; i++) {
    int l;
    l = 28;
    start = ParseLine(start, end, l);
    consoleSetWindow(&lowerScreen, 2, i + 2, 28, 1);
    char tmp = *(start + l);
    *(start + l) = '\0';
    iprintf(start);
    *(start + l) = tmp;
    start += l + 1;
    if (start >= end) break;
  }
}

void displayDebugF(const char *format, ...) {
  va_list argp;
  va_start(argp, format);
  memset(txt, 0, 256);

  consoleSelect(&upperScreen);
  consoleSetWindow(&upperScreen, 0, 12, 32, 4);
  consoleClear();

  vprintf(format, argp);
  va_end(argp);
}
