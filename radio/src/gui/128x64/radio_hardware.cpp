/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

#define HW_SETTINGS_COLUMN1            30
#define HW_SETTINGS_COLUMN2            (HW_SETTINGS_COLUMN1 + 5*FW)
#define HW_SETTINGS_COLUMN3            HW_SETTINGS_COLUMN2 + FW

#if defined(BATTGRAPH)
  #define CASE_BATTGRAPH(x) x,
#else
  #define CASE_BATTGRAPH(x)
#endif

#if defined(PCBTARANIS) || defined(PCBI6X)
enum {
  ITEM_RADIO_HARDWARE_LABEL_STICKS = 0,
  ITEM_RADIO_HARDWARE_STICK1,
  ITEM_RADIO_HARDWARE_STICK2,
  ITEM_RADIO_HARDWARE_STICK3,
  ITEM_RADIO_HARDWARE_STICK4,
  ITEM_RADIO_HARDWARE_LABEL_POTS,
  ITEM_RADIO_HARDWARE_POT1,
  ITEM_RADIO_HARDWARE_POT2,
  ITEM_RADIO_HARDWARE_LABEL_SWITCHES,
  ITEM_RADIO_HARDWARE_SA,
  ITEM_RADIO_HARDWARE_SB,
  ITEM_RADIO_HARDWARE_SC,
  ITEM_RADIO_HARDWARE_SD,
#if NUM_SWITCHES >= 5
  ITEM_RADIO_HARDWARE_SF,
#endif
#if NUM_SWITCHES >= 6
  ITEM_RADIO_HARDWARE_SH,
#endif
  CASE_BATTGRAPH(ITEM_RADIO_HARDWARE_BATT_RANGE)
  ITEM_RADIO_HARDWARE_BATTERY_CALIB,
  ITEM_RADIO_HARDWARE_SERIAL_BAUDRATE,
#if defined(BLUETOOTH)
  ITEM_RADIO_HARDWARE_BLUETOOTH_MODE,
  ITEM_RADIO_HARDWARE_BLUETOOTH_PAIRING_CODE,
  ITEM_RADIO_HARDWARE_BLUETOOTH_LOCAL_ADDR,
  ITEM_RADIO_HARDWARE_BLUETOOTH_DISTANT_ADDR,
  ITEM_RADIO_HARDWARE_BLUETOOTH_NAME,
#endif
#if defined(AUX_SERIAL)
  ITEM_RADIO_HARDWARE_AUX_SERIAL_MODE,
#endif
  ITEM_RADIO_HARDWARE_JITTER_FILTER,
#if defined(MENU_DIAG_ANAS_KEYS)
  ITEM_RADIO_HARDWARE_DEBUG,
#endif
#if defined(EEPROM_RLC)
#if !defined(PCBI6X)
  ITEM_RADIO_BACKUP_EEPROM,
#endif
  ITEM_RADIO_FACTORY_RESET,
#endif
  ITEM_RADIO_HARDWARE_MAX
};

#if defined(PCBX3)
#define POTS_ROWS                      NAVIGATION_LINE_BY_LINE|1
#else
#define POTS_ROWS                      NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1
#endif

#if NUM_SWITCHES == 6
#define SWITCHES_ROWS                  NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1
#elif NUM_SWITCHES == 5
#define SWITCHES_ROWS                  NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1
#elif NUM_SWITCHES == 4
#define SWITCHES_ROWS                  NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1, NAVIGATION_LINE_BY_LINE|1
#endif

#if defined(PCBX3)
  #define BLUETOOTH_ROWS
#elif defined(PCBTARANIS)
#define BLUETOOTH_ROWS                 uint8_t(IS_BLUETOOTH_CHIP_PRESENT() ? 0 : HIDDEN_ROW), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_TELEMETRY ? -1 : HIDDEN_ROW), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : -1), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : -1), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : 0),
#elif defined(BLUETOOTH)
#define BLUETOOTH_ROWS                 0, uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : -1), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : -1), uint8_t(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? HIDDEN_ROW : 0),
#else
#define BLUETOOTH_ROWS
#endif
#if defined(PCBXLITE) || defined(PCBI6X)
#define SWITCH_TYPE_MAX(sw)            (SWITCH_3POS)
#else
#define SWITCH_TYPE_MAX(sw)            ((MIXSRC_SF-MIXSRC_FIRST_SWITCH == sw || MIXSRC_SH-MIXSRC_FIRST_SWITCH == sw) ? SWITCH_2POS : SWITCH_3POS)
#endif

#if defined(EEPROM_RLC)
void onFactoryResetConfirm(const char result)
{
  if (result) {
    warningResult = 0;
    showMessageBox(STR_STORAGE_FORMAT);
    storageEraseAll(false);
    NVIC_SystemReset();
  }
}
#endif

void menuRadioHardware(event_t event)
{
  MENU(STR_HARDWARE, menuTabGeneral, MENU_RADIO_HARDWARE, HEADER_LINE + ITEM_RADIO_HARDWARE_MAX, {
    HEADER_LINE_COLUMNS
    0 /* calibration button */,
      0 /* stick 1 */,
      0 /* stick 2 */,
      0 /* stick 3 */,
      0 /* stick 4 */,
    LABEL(Pots),
      POTS_ROWS,
    LABEL(Switches),
      SWITCHES_ROWS,
    CASE_BATTGRAPH(1) 
    0 /* battery calib */,
#if defined(CROSSFIRE)
    0 /* max bauds */,
#endif
    0 /* Aux serial mode */,
    BLUETOOTH_ROWS
    0 /*jitter filter*/,
#if defined(MENU_DIAG_ANAS_KEYS)
    1 /* debugs */,
#endif
    0 /* factory reset */
  });

  uint8_t sub = menuVerticalPosition - HEADER_LINE;

  for (uint32_t i=0; i<NUM_BODY_LINES; i++) {
    coord_t y = MENU_HEADER_HEIGHT + 1 + i*FH;
    uint8_t k = i+menuVerticalOffset;
    for (int j=0; j<=k; j++) {
      if (mstate_tab[j+HEADER_LINE] == HIDDEN_ROW) {
        k++;
      }
    }
    uint8_t blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);
    uint8_t attr = (sub == k ? blink : 0);

    switch(k) {
      case ITEM_RADIO_HARDWARE_LABEL_STICKS:
        lcdDrawTextAlignedLeft(y, STR_STICKS);
        lcdDrawText(HW_SETTINGS_COLUMN2, y, STR_CALIB_BTN, attr);
        if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
          pushMenu(menuRadioCalibration);
        }
        break;
      case ITEM_RADIO_HARDWARE_STICK1:
      case ITEM_RADIO_HARDWARE_STICK2:
      case ITEM_RADIO_HARDWARE_STICK3:
      case ITEM_RADIO_HARDWARE_STICK4:
        editStickHardwareSettings(HW_SETTINGS_COLUMN1, y, k - ITEM_RADIO_HARDWARE_STICK1, event, attr);
        break;

      case ITEM_RADIO_HARDWARE_LABEL_POTS:
        lcdDrawTextAlignedLeft(y, STR_POTS);
        break;

      case ITEM_RADIO_HARDWARE_POT1:
      case ITEM_RADIO_HARDWARE_POT2:
      {
        int idx = k - ITEM_RADIO_HARDWARE_POT1;
        uint8_t shift = (2*idx);
        uint8_t mask = (0x03 << shift);
        lcdDrawTextAtIndex(INDENT_WIDTH, y, STR_VSRCRAW, NUM_STICKS+idx+1, /*menuHorizontalPosition < 0 ? attr :*/ 0);
        if (ZEXIST(g_eeGeneral.anaNames[NUM_STICKS+idx]) || (attr && s_editMode > 0 && menuHorizontalPosition == 0))
          editName(HW_SETTINGS_COLUMN1, y, g_eeGeneral.anaNames[NUM_STICKS+idx], LEN_ANA_NAME, event, attr && menuHorizontalPosition == 0);
        else
          lcdDrawMMM(HW_SETTINGS_COLUMN1, y, menuHorizontalPosition==0 ? attr : 0);
        uint8_t potType = (g_eeGeneral.potsConfig & mask) >> shift;
        potType = editChoice(HW_SETTINGS_COLUMN2, y, "", STR_POTTYPES, potType, POT_NONE, POT_WITHOUT_DETENT, menuHorizontalPosition == 1 ? attr : 0, event);
        g_eeGeneral.potsConfig &= ~mask;
        g_eeGeneral.potsConfig |= (potType << shift);
        break;
      }

      case ITEM_RADIO_HARDWARE_LABEL_SWITCHES:
        lcdDrawTextAlignedLeft(y, STR_SWITCHES);
        break;

      case ITEM_RADIO_HARDWARE_SA:
      case ITEM_RADIO_HARDWARE_SB:
      case ITEM_RADIO_HARDWARE_SC:
      case ITEM_RADIO_HARDWARE_SD:
#if NUM_SWITCHES >= 5
      case ITEM_RADIO_HARDWARE_SF:
#endif
#if NUM_SWITCHES >= 6
      case ITEM_RADIO_HARDWARE_SH:
#endif
      {
        int index = k-ITEM_RADIO_HARDWARE_SA;
        int config = SWITCH_CONFIG(index);
        lcdDrawTextAtIndex(INDENT_WIDTH, y, STR_VSRCRAW, MIXSRC_FIRST_SWITCH-MIXSRC_Rud+index+1, /*menuHorizontalPosition < 0 ? attr :*/ 0);
        if (ZEXIST(g_eeGeneral.switchNames[index]) || (attr && s_editMode > 0 && menuHorizontalPosition == 0))
          editName(HW_SETTINGS_COLUMN1, y, g_eeGeneral.switchNames[index], LEN_SWITCH_NAME, event, menuHorizontalPosition == 0 ? attr : 0);
        else
          lcdDrawMMM(HW_SETTINGS_COLUMN1, y, menuHorizontalPosition == 0 ? attr : 0);
        config = editChoice(HW_SETTINGS_COLUMN2, y, "", STR_SWTYPES, config, SWITCH_NONE, SWITCH_TYPE_MAX(index), menuHorizontalPosition == 1 ? attr : 0, event);
        if (attr && checkIncDec_Ret) {
          swconfig_t mask = (swconfig_t)0x03 << (2*index);
          g_eeGeneral.switchConfig = (g_eeGeneral.switchConfig & ~mask) | ((swconfig_t(config) & 0x03) << (2*index));
        }
        break;
      }

#if defined(BATTGRAPH)
      case ITEM_RADIO_HARDWARE_BATT_RANGE:
        lcdDrawTextAlignedLeft(y, STR_BATTERY_RANGE);
        putsVolts(HW_SETTINGS_COLUMN3, y, 90+g_eeGeneral.vBatMin, (menuHorizontalPosition==0 ? attr : 0)|NO_UNIT);
        lcdDrawChar(lcdNextPos, y, '-');
        putsVolts(lcdNextPos, y, 120+g_eeGeneral.vBatMax, (menuHorizontalPosition>0 ? attr : 0)|NO_UNIT);
        if (attr && s_editMode>0) {
          if (menuHorizontalPosition==0)
            CHECK_INCDEC_GENVAR(event, g_eeGeneral.vBatMin, -60, g_eeGeneral.vBatMax+29); // min=3.0V
          else
            CHECK_INCDEC_GENVAR(event, g_eeGeneral.vBatMax, g_eeGeneral.vBatMin-29, +40); // max=16.0V
        }
        break;
#endif

      case ITEM_RADIO_HARDWARE_BATTERY_CALIB:
        lcdDrawTextAlignedLeft(y, STR_BATT_CALIB);
        putsVolts(HW_SETTINGS_COLUMN3, y, getBatteryVoltage(), attr|PREC2|LEFT);
        if (attr) {
          CHECK_INCDEC_GENVAR(event, g_eeGeneral.txVoltageCalibration, -127, 127);
        }
        break;

#if defined(CROSSFIRE)
      case ITEM_RADIO_HARDWARE_SERIAL_BAUDRATE:
        g_eeGeneral.telemetryBaudrate = editChoice(HW_SETTINGS_COLUMN2, y, STR_MAXBAUDRATE, "\005400k 115k 921k 1.87M3.75M5.25M", g_eeGeneral.telemetryBaudrate, 0, DIM(CROSSFIRE_BAUDRATES) - 1, attr, event);
        if (attr) {
          storageDirty(EE_GENERAL);
          if (checkIncDec_Ret && IS_EXTERNAL_MODULE_ON()) {
            pauseMixerCalculations();
            pausePulses();
            EXTERNAL_MODULE_OFF();
            RTOS_WAIT_MS(20); // 20ms so that the pulses interrupt will reinit the frame rate
            telemetryProtocol = 255; // force telemetry port + module reinitialization
            EXTERNAL_MODULE_ON();
            resumePulses();
            resumeMixerCalculations();
          }
        }
        break;
#endif

#if defined(AUX_SERIAL)
      case ITEM_RADIO_HARDWARE_AUX_SERIAL_MODE:
        g_eeGeneral.auxSerialMode = editChoice(HW_SETTINGS_COLUMN2, y, STR_AUX_SERIAL_MODE, STR_AUX_SERIAL_MODES, g_eeGeneral.auxSerialMode, 0, UART_MODE_MAX, attr, event);
        if (attr && checkIncDec_Ret) {
          auxSerialInit(g_eeGeneral.auxSerialMode, modelTelemetryProtocol());
          storageDirty(EE_GENERAL);
        }
        break;
#endif

      case ITEM_RADIO_HARDWARE_JITTER_FILTER:
        g_eeGeneral.jitterFilter = 1 - editCheckBox(1 - g_eeGeneral.jitterFilter, HW_SETTINGS_COLUMN2, y, STR_JITTER_FILTER, attr, event);
        if (attr && checkIncDec_Ret) {
          storageDirty(EE_GENERAL);
        }
        break;

#if defined(MENU_DIAG_ANAS_KEYS)
      case ITEM_RADIO_HARDWARE_DEBUG:
        lcdDrawTextAlignedLeft(y, STR_DEBUG);
        lcdDrawText(HW_SETTINGS_COLUMN2, y, STR_ANALOGS_BTN, menuHorizontalPosition == 0 ? attr : 0);
        lcdDrawText(lcdLastRightPos + 2, y, STR_KEYS_BTN, menuHorizontalPosition == 1 ? attr : 0);
        if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
          if (menuHorizontalPosition == 0)
            pushMenu(menuRadioDiagAnalogs);
          else
            pushMenu(menuRadioDiagKeys);
        }
        break;
#endif // MENU_DIAG_ANAS_KEYS
#if defined(SDCARD)
      case ITEM_RADIO_BACKUP_EEPROM:
        lcdDrawTextAlignedLeft(y, BUTTON(STR_EEBACKUP), attr);
        if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
          s_editMode = EDIT_SELECT_FIELD;
          eepromBackup();
        }
        break;
#endif
      case ITEM_RADIO_FACTORY_RESET:
        onFactoryResetConfirm(warningResult);
        if (LCD_W < 212)
          lcdDrawText(LCD_W / 2, y, BUTTON(TR_FACTORYRESET), attr | CENTERED);
        else
          lcdDrawText(HW_SETTINGS_COLUMN2, y, BUTTON(TR_FACTORYRESET), attr);
        if (attr && event == EVT_KEY_BREAK(KEY_ENTER)) {
          s_editMode = EDIT_SELECT_FIELD;
//          POPUP_CONFIRMATION(STR_CONFIRMRESET, onFactoryResetConfirm);
          POPUP_CONFIRMATION(STR_CONFIRMRESET);
       }
        break;
    }
  }
}
#endif // PCBSKY9X
