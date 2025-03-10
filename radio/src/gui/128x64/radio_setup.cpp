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

#define LANGUAGE_PACKS_DEFINITION

#include "opentx.h"

const unsigned char sticks[]  = {
#include "sticks.lbm"
};

#define RADIO_SETUP_2ND_COLUMN  79

int8_t slider_5pos(coord_t y, int8_t value, event_t event, uint8_t attr, const char* title)
{
  drawSlider(RADIO_SETUP_2ND_COLUMN, y, LCD_W - 2 - RADIO_SETUP_2ND_COLUMN, 2+value, 4, attr);
  return editChoice(RADIO_SETUP_2ND_COLUMN, y, title, nullptr, value, -2, +2, attr, event, INDENT_WIDTH);
}

#if defined(SPLASH)
  #define CASE_SPLASH_PARAM(x) x,
#else
  #define CASE_SPLASH_PARAM(x)
#endif

enum MenuRadioSetupItems {
  CASE_RTCLOCK(ITEM_SETUP_DATE)
  CASE_RTCLOCK(ITEM_SETUP_TIME)
  ITEM_SETUP_SOUND_LABEL,
  CASE_AUDIO(ITEM_SETUP_BEEP_MODE)
  CASE_BUZZER(ITEM_SETUP_BUZZER_MODE)
  // ITEM_SETUP_SPEAKER_VOLUME,
  ITEM_SETUP_BEEP_VOLUME,
  ITEM_SETUP_BEEP_LENGTH,
  ITEM_SETUP_SPEAKER_PITCH,
  CASE_DFPLAYER(ITEM_SETUP_WAV_VOLUME)
  // ITEM_SETUP_BACKGROUND_VOLUME,
  CASE_VARIO(ITEM_SETUP_VARIO_LABEL)
  CASE_VARIO(ITEM_SETUP_VARIO_VOLUME)
  CASE_VARIO(ITEM_SETUP_VARIO_PITCH)
  CASE_VARIO(ITEM_SETUP_VARIO_RANGE)
  CASE_VARIO(ITEM_SETUP_VARIO_REPEAT)
  CASE_HAPTIC(ITEM_SETUP_HAPTIC_LABEL)
  CASE_HAPTIC(ITEM_SETUP_HAPTIC_MODE)
  CASE_HAPTIC(ITEM_SETUP_HAPTIC_LENGTH)
  CASE_HAPTIC(ITEM_SETUP_HAPTIC_STRENGTH)
  ITEM_SETUP_ALARMS_LABEL,
  ITEM_SETUP_BATTERY_WARNING,
  CASE_CAPACITY(ITEM_SETUP_CAPACITY_WARNING)
  ITEM_SETUP_INACTIVITY_ALARM,
  ITEM_SETUP_MEMORY_WARNING,
  ITEM_SETUP_ALARM_WARNING,
#if defined(PWR_BUTTON_SOFT)
  ITEM_SETUP_RSSI_POWEROFF_ALARM,
#endif
  IF_ROTARY_ENCODERS(ITEM_SETUP_RE_NAVIGATION)
  ITEM_SETUP_BACKLIGHT_LABEL,
  ITEM_SETUP_BACKLIGHT_MODE,
  ITEM_SETUP_BACKLIGHT_DELAY,
  ITEM_SETUP_BRIGHTNESS,
  CASE_PWM_BACKLIGHT(ITEM_SETUP_BACKLIGHT_BRIGHTNESS_OFF)
  CASE_PWM_BACKLIGHT(ITEM_SETUP_BACKLIGHT_BRIGHTNESS_ON)
  ITEM_SETUP_FLASH_BEEP,
  ITEM_SETUP_CONTRAST,
  CASE_SPLASH_PARAM(ITEM_SETUP_DISABLE_SPLASH)
  #if defined(PXX2)
    ITEM_RADIO_OWNER_ID,
  #endif
  // CASE_GPS(ITEM_SETUP_TIMEZONE)
  // ITEM_SETUP_ADJUST_RTC,
  CASE_GPS(ITEM_SETUP_GPSFORMAT)
  CASE_PXX(ITEM_SETUP_COUNTRYCODE)
  // ITEM_SETUP_LANGUAGE,
  // ITEM_SETUP_IMPERIAL,
  ITEM_SETUP_PPM,
  IF_FAI_CHOICE(ITEM_SETUP_FAI)
  ITEM_SETUP_SWITCHES_DELAY,
  CASE_STM32(ITEM_SETUP_USB_MODE)
  ITEM_SETUP_RX_CHANNEL_ORD,
  ITEM_SETUP_STICK_MODE_LABELS,
  ITEM_SETUP_STICK_MODE,
  ITEM_SETUP_MAX
};

#if defined(FRSKY_STICKS) && !defined(PCBTARANIS)
  #define COL_TX_MODE 0
#else
  #define COL_TX_MODE LABEL(TX_MODE)
#endif

void menuRadioSetup(event_t event)
{
#if defined(RTCLOCK)
  struct gtm t;
  gettime(&t);

  if ((menuVerticalPosition==ITEM_SETUP_DATE+HEADER_LINE || menuVerticalPosition==ITEM_SETUP_TIME+HEADER_LINE) &&
      (s_editMode>0) &&
      (event==EVT_KEY_FIRST(KEY_ENTER) || event==EVT_KEY_FIRST(KEY_EXIT) || IS_ROTARY_BREAK(event) || IS_ROTARY_LONG(event))) {
    // set the date and time into RTC chip
    rtcSetTime(&t);
  }
#endif

#if defined(FAI_CHOICE)
  if (warningResult) {
    warningResult = 0;
    g_eeGeneral.fai = true;
    storageDirty(EE_GENERAL);
  }
#endif

  MENU(STR_MENURADIOSETUP, menuTabGeneral, MENU_RADIO_SETUP, HEADER_LINE+ITEM_SETUP_MAX, { 
    HEADER_LINE_COLUMNS CASE_RTCLOCK(2) CASE_RTCLOCK(2) 
    LABEL(SOUND), CASE_AUDIO(0)
    CASE_BUZZER(0)
    /*0,*/ 0, 0, 0, CASE_DFPLAYER(0) CASE_AUDIO(0)
    CASE_VARIO(LABEL(VARIO))
    CASE_VARIO(0)
    CASE_VARIO(0)
    CASE_VARIO(0)
    CASE_VARIO(0)
    CASE_HAPTIC(LABEL(HAPTIC))
    CASE_HAPTIC(0)
    CASE_HAPTIC(0)
    CASE_HAPTIC(0)
    LABEL(ALARMS), 0, CASE_CAPACITY(0)
    0, 0, 0,
#if defined(PWR_BUTTON_SOFT)
    0, /* rssi poweroff alarm */
#endif
    IF_ROTARY_ENCODERS(0)
    LABEL(BACKLIGHT), 0, 0,
    0, /* backlight */
    CASE_PWM_BACKLIGHT(0)
    CASE_PWM_BACKLIGHT(0)
    0, /* alarm */
    0, // Contrast
    CASE_SPLASH_PARAM(0)
#if defined(PXX2)
    0 /* owner registration ID */,
#endif
    /*CASE_GPS(0)*/
    /*0, rtc */
    CASE_GPS(0)
    CASE_PXX(0)
    /*0, 0,*/ /* voice language, imperial */
    0, /* PPM Unit */
    IF_FAI_CHOICE(0)
    0,
    CASE_STM32(0) // USB mode
    0,
    COL_TX_MODE, 0, 1/*to force edit mode*/});

  if (event == EVT_ENTRY) {
    reusableBuffer.generalSettings.stickMode = g_eeGeneral.stickMode;
  }

  uint8_t sub = menuVerticalPosition - HEADER_LINE;

  for (uint32_t i=0; i<LCD_LINES-1; i++) {
    coord_t y = MENU_HEADER_HEIGHT + 1 + i*FH;
    uint8_t k = i + menuVerticalOffset;
    uint8_t blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);
    uint8_t attr = (sub == k ? blink : 0);

    switch (k) {
#if defined(RTCLOCK)
      case ITEM_SETUP_DATE:
        lcdDrawTextAlignedLeft(y, STR_DATE);
        for (uint32_t j=0; j<3; j++) {
          uint8_t rowattr = (menuHorizontalPosition==j ? attr : 0);
          switch (j) {
            case 0:
              lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, t.tm_year+TM_YEAR_BASE, rowattr|RIGHT);
              lcdDrawChar(lcdNextPos, y, '-');
              if (rowattr && s_editMode > 0) t.tm_year = checkIncDec(event, t.tm_year, 112, 200, 0);
              break;
            case 1:
              lcdDrawNumber(lcdNextPos+3*FW-2, y, t.tm_mon+1, rowattr|LEADING0|RIGHT, 2);
              lcdDrawChar(lcdNextPos, y, '-');
              if (rowattr && s_editMode > 0) t.tm_mon = checkIncDec(event, t.tm_mon, 0, 11, 0);
              break;
            case 2:
            {
              int16_t year = TM_YEAR_BASE + t.tm_year;
              int8_t dlim = (((((year%4==0) && (year%100!=0)) || (year%400==0)) && (t.tm_mon==1)) ? 1 : 0);
              static const uint8_t dmon[]  = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
              dlim += dmon[t.tm_mon];
              lcdDrawNumber(lcdNextPos+6*FW-4, y, t.tm_mday, rowattr|LEADING0|RIGHT, 2);
              if (rowattr && s_editMode > 0) t.tm_mday = checkIncDec(event, t.tm_mday, 1, dlim, 0);
              break;
            }
          }
        }
        if (attr && checkIncDec_Ret) {
          g_rtcTime = gmktime(&t); // update local timestamp and get wday calculated
        }
        break;

      case ITEM_SETUP_TIME:
        lcdDrawTextAlignedLeft(y, STR_TIME);
        for (uint32_t j=0; j<3; j++) {
          uint8_t rowattr = (menuHorizontalPosition==j ? attr : 0);
          switch (j) {
            case 0:
              lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, t.tm_hour, rowattr|LEADING0|RIGHT, 2);
              lcdDrawChar(lcdNextPos + 1, y, ':');
              if (rowattr && s_editMode > 0) t.tm_hour = checkIncDec(event, t.tm_hour, 0, 23, 0);
              break;
            case 1:
              lcdDrawNumber(lcdNextPos + 1, y, t.tm_min, rowattr|LEADING0|RIGHT, 2);
              lcdDrawChar(lcdNextPos + 1, y, ':');
              if (rowattr && s_editMode > 0) t.tm_min = checkIncDec(event, t.tm_min, 0, 59, 0);
              break;
            case 2:
              lcdDrawNumber(lcdNextPos + 1, y, t.tm_sec, rowattr|LEADING0|RIGHT, 2);
              if (rowattr && s_editMode > 0) t.tm_sec = checkIncDec(event, t.tm_sec, 0, 59, 0);
              break;
          }
        }
        if (attr && checkIncDec_Ret) {
          g_rtcTime = gmktime(&t); // update local timestamp and get wday calculated
        }
        break;
#endif

      case ITEM_SETUP_SOUND_LABEL:
        lcdDrawTextAlignedLeft(y, STR_SOUND_LABEL);
        break;

#if defined(AUDIO)
      case ITEM_SETUP_BEEP_MODE:
        g_eeGeneral.beepMode = editChoice(LCD_W-2, y, STR_SPEAKER, STR_VBEEPMODE, g_eeGeneral.beepMode, -2, 1, attr|RIGHT, event, INDENT_WIDTH);
        break;

#if defined(BUZZER) // AUDIO + BUZZER
      case ITEM_SETUP_BUZZER_MODE:
        g_eeGeneral.buzzerMode = editChoice(LCD_W-2, y, STR_BUZZER, STR_VBEEPMODE, g_eeGeneral.buzzerMode, -2, 1, attr|RIGHT, event, INDENT_WIDTH);
        break;
#endif
#elif defined(BUZZER) // BUZZER only
      case ITEM_SETUP_BUZZER_MODE:
        g_eeGeneral.beepMode = editChoice(LCD_W-2, y, STR_SPEAKER, STR_VBEEPMODE, g_eeGeneral.beepMode, -2, 1, attr|RIGHT, event, INDENT_WIDTH);
        break;
#endif

#if defined(VOICE)
      case ITEM_SETUP_SPEAKER_VOLUME:
      {
        lcdDrawTextIndented(y, STR_SPEAKER_VOLUME);
        uint8_t b = g_eeGeneral.speakerVolume+VOLUME_LEVEL_DEF;
        drawSlider(RADIO_SETUP_2ND_COLUMN, y, LCD_W - 2 - RADIO_SETUP_2ND_COLUMN, b, VOLUME_LEVEL_MAX, attr);
        if (attr) {
          CHECK_INCDEC_GENVAR(event, b, 0, VOLUME_LEVEL_MAX);
          if (checkIncDec_Ret) {
            g_eeGeneral.speakerVolume = (int8_t)b-VOLUME_LEVEL_DEF;
          }
        }
        break;
      }
#endif

      case ITEM_SETUP_BEEP_VOLUME:
        g_eeGeneral.beepVolume = slider_5pos(y, g_eeGeneral.beepVolume, event, attr, STR_BEEP_VOLUME);
        break;

#if defined(DFPLAYER)
      case ITEM_SETUP_WAV_VOLUME:
      {
        int8_t vol = g_eeGeneral.wavVolume;
        g_eeGeneral.wavVolume = slider_5pos(y, g_eeGeneral.wavVolume, event, attr, STR_WAV_VOLUME);
        if (vol != g_eeGeneral.wavVolume) {
          dfplayerSetVolume(g_eeGeneral.wavVolume);
        }
      }
      break;
#endif
      // case ITEM_SETUP_BACKGROUND_VOLUME:
      //   g_eeGeneral.backgroundVolume = slider_5pos(y, g_eeGeneral.backgroundVolume, event, attr, STR_BG_VOLUME);
      //   break;

      case ITEM_SETUP_BEEP_LENGTH:
        g_eeGeneral.beepLength = slider_5pos(y, g_eeGeneral.beepLength, event, attr, STR_BEEP_LENGTH);
        break;

      case ITEM_SETUP_SPEAKER_PITCH:
        {
          lcdDrawTextIndented(y, STR_SPKRPITCH);
          lcdDrawNumber(LCD_W-14, y, g_eeGeneral.speakerPitch*15, attr|RIGHT);
          coord_t lp = lcdLastLeftPos - FW;
          lcdDrawText(lcdLastRightPos, y, "Hz", attr);
          lcdDrawChar(lp, y, '+', attr);
          if (attr) {
            CHECK_INCDEC_GENVAR(event, g_eeGeneral.speakerPitch, 0, 20);
          }
        }
        break;

#if defined(VARIO)
      case ITEM_SETUP_VARIO_LABEL:
        lcdDrawTextAlignedLeft(y, STR_VARIO);
        break;
      case ITEM_SETUP_VARIO_VOLUME:
        g_eeGeneral.varioVolume = slider_5pos(y, g_eeGeneral.varioVolume, event, attr, TR_SPEAKER_VOLUME);
        break;
      case ITEM_SETUP_VARIO_PITCH:
        lcdDrawTextIndented(y, STR_PITCH_AT_ZERO);
        lcdDrawNumber(LCD_W-14, y, VARIO_FREQUENCY_ZERO+(g_eeGeneral.varioPitch*10), attr|RIGHT);
        lcdDrawText(lcdLastRightPos, y, "Hz", attr);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.varioPitch, -40, 40);
        break;
      case ITEM_SETUP_VARIO_RANGE:
        lcdDrawTextIndented(y, STR_PITCH_AT_MAX);
        lcdDrawNumber(LCD_W-14, y, VARIO_FREQUENCY_ZERO+(g_eeGeneral.varioPitch*10)+VARIO_FREQUENCY_RANGE+(g_eeGeneral.varioRange*10), attr|RIGHT);
        lcdDrawText(lcdLastRightPos, y, "Hz", attr);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.varioRange, -80, 80);
        break;
      case ITEM_SETUP_VARIO_REPEAT:
        lcdDrawTextIndented(y, STR_REPEAT_AT_ZERO);
        lcdDrawNumber(LCD_W-14, y, VARIO_REPEAT_ZERO+(g_eeGeneral.varioRepeat*10), attr|RIGHT);
        lcdDrawText(lcdLastRightPos, y, STR_MS, attr);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.varioRepeat, -30, 50);
        break;
#endif

#if defined(HAPTIC)
      case ITEM_SETUP_HAPTIC_LABEL:
        lcdDrawTextAlignedLeft(y, STR_HAPTIC_LABEL);
        break;

      case ITEM_SETUP_HAPTIC_MODE:
        g_eeGeneral.hapticMode = editChoice(LCD_W-2s, y, TR_MODE, STR_VBEEPMODE, g_eeGeneral.hapticMode, -2, 1, attr|RIGHT, event);
        break;

      case ITEM_SETUP_HAPTIC_LENGTH:
        g_eeGeneral.hapticLength = slider_5pos(y, g_eeGeneral.hapticLength, event, attr, STR_LENGTH);
        break;

      case ITEM_SETUP_HAPTIC_STRENGTH:
        g_eeGeneral.hapticStrength = slider_5pos(y, g_eeGeneral.hapticStrength, event, attr, STR_HAPTICSTRENGTH);
        break;
#endif

      case ITEM_SETUP_CONTRAST:
        lcdDrawTextIndented(y, STR_CONTRAST);
        lcdDrawNumber(LCD_W-2, y, g_eeGeneral.contrast, attr|RIGHT);
        if (attr) {
          CHECK_INCDEC_GENVAR(event, g_eeGeneral.contrast, LCD_CONTRAST_MIN, LCD_CONTRAST_MAX);
          lcdSetContrast();
        }
        break;

      case ITEM_SETUP_ALARMS_LABEL:
        lcdDrawTextAlignedLeft(y, STR_ALARMS_LABEL);
        break;

      case ITEM_SETUP_BATTERY_WARNING:
        lcdDrawTextIndented(y, STR_BATTERYWARNING);
        putsVolts(LCD_W-7, y, g_eeGeneral.vBatWarn, attr|RIGHT);
        if(attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.vBatWarn, 30, 120); //3.5-12V
        break;

      case ITEM_SETUP_MEMORY_WARNING:
      {
        uint8_t b = 1-g_eeGeneral.disableMemoryWarning;
        g_eeGeneral.disableMemoryWarning = 1 - editCheckBox(b, LCD_W-9, y, STR_MEMORYWARNING, attr, event, INDENT_WIDTH);
        break;
      }

      case ITEM_SETUP_ALARM_WARNING:
      {
        uint8_t b = 1 - g_eeGeneral.disableAlarmWarning;
        g_eeGeneral.disableAlarmWarning = 1 - editCheckBox(b, LCD_W-9, y, STR_ALARMWARNING, attr, event, INDENT_WIDTH);
        break;
      }
#if defined(PWR_BUTTON_SOFT)
      case ITEM_SETUP_RSSI_POWEROFF_ALARM:
      {
        uint8_t b = 1 - g_eeGeneral.disableRssiPoweroffAlarm;
        g_eeGeneral.disableRssiPoweroffAlarm = 1 - editCheckBox(b, LCD_W-9, y, STR_RSSISHUTDOWNALARM, attr, event, INDENT_WIDTH);
        break;
      }
#endif

      case ITEM_SETUP_INACTIVITY_ALARM:
        lcdDrawTextIndented(y, STR_INACTIVITYALARM);
        lcdDrawNumber(LCD_W-7, y, g_eeGeneral.inactivityTimer, attr|RIGHT);
        lcdDrawChar(lcdLastRightPos, y, 'm');
        if(attr) g_eeGeneral.inactivityTimer = checkIncDec(event, g_eeGeneral.inactivityTimer, 0, 250, EE_GENERAL); //0..250minutes
        break;

#if defined(ROTARY_ENCODERS)
      case ITEM_SETUP_RE_NAVIGATION:
        g_eeGeneral.reNavigation = editChoice(RADIO_SETUP_2ND_COLUMN, y, STR_RENAVIG, STR_VRENAVIG, g_eeGeneral.reNavigation, 0, NUM_ROTARY_ENCODERS, attr, event);
        if (attr && checkIncDec_Ret) {
          ROTARY_ENCODER_NAVIGATION_VALUE = 0;
        }
        break;
#endif

      case ITEM_SETUP_BACKLIGHT_LABEL:
        lcdDrawTextAlignedLeft(y, STR_BACKLIGHT_LABEL);
        break;

      case ITEM_SETUP_BACKLIGHT_MODE:
        g_eeGeneral.backlightMode = editChoice(LCD_W-2, y, TR_MODE, STR_VBLMODE, g_eeGeneral.backlightMode, e_backlight_mode_off, e_backlight_mode_on, attr|RIGHT, event, INDENT_WIDTH);
        break;

      case ITEM_SETUP_FLASH_BEEP:
        g_eeGeneral.alarmsFlash = editCheckBox(g_eeGeneral.alarmsFlash, LCD_W-9, y, STR_ALARM, attr, event, INDENT_WIDTH);
        break;

      case ITEM_SETUP_BACKLIGHT_DELAY:
        lcdDrawTextIndented(y, STR_BLDELAY);
        lcdDrawNumber(LCD_W-7, y, g_eeGeneral.lightAutoOff*5, attr|RIGHT);
        lcdDrawChar(lcdLastRightPos, y, 's');
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.lightAutoOff, 0, 600/5);
        break;
      case ITEM_SETUP_BRIGHTNESS:
        lcdDrawTextIndented(y, STR_BRIGHTNESS);
        lcdDrawNumber(LCD_W-2, y, 100-g_eeGeneral.backlightBright, attr|RIGHT) ;
        if (attr) {
          uint8_t b = 100 - g_eeGeneral.backlightBright;
          CHECK_INCDEC_GENVAR(event, b, 0, 100);
          g_eeGeneral.backlightBright = 100 - b;
        }
        break;
#if defined(PWM_BACKLIGHT)
      case ITEM_SETUP_BACKLIGHT_BRIGHTNESS_OFF:
        lcdDrawTextAlignedLeft(y, STR_BLOFFBRIGHTNESS);
        lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, g_eeGeneral.blOffBright, attr|LEFT);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.blOffBright, 0, 15);
        break;

      case ITEM_SETUP_BACKLIGHT_BRIGHTNESS_ON:
        lcdDrawTextAlignedLeft(y, STR_BLONBRIGHTNESS);
        lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, 15-g_eeGeneral.blOnBright, attr|LEFT);
        if (attr) g_eeGeneral.blOnBright = 15 - checkIncDecGen(event, 15-g_eeGeneral.blOnBright, 0, 15);
        break;
#endif

#if defined(SPLASH)
      case ITEM_SETUP_DISABLE_SPLASH:
      {
        lcdDrawTextAlignedLeft(y, STR_SPLASHSCREEN);
        if (SPLASH_NEEDED()) {
          lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, SPLASH_TIMEOUT/100, attr|LEFT);
          lcdDrawChar(lcdLastRightPos, y, 's');
        }
        else {
          lcdDrawMMM(RADIO_SETUP_2ND_COLUMN, y, attr);
        }
        if (attr) g_eeGeneral.splashMode = -checkIncDecGen(event, -g_eeGeneral.splashMode, -3, 4);
        break;
      }
#endif

#if defined(PXX2)
      case ITEM_RADIO_OWNER_ID:
        editSingleName(RADIO_SETUP_2ND_COLUMN, y, STR_OWNER_ID, g_eeGeneral.ownerRegistrationID, PXX2_LEN_REGISTRATION_ID, event, attr);
        break;
#endif

#if defined(TELEMETRY_FRSKY) && defined(GPS)
#if defined(RTCLOCK)
      case ITEM_SETUP_TIMEZONE:
        lcdDrawTextAlignedLeft(y, STR_TIMEZONE);
        lcdDrawNumber(RADIO_SETUP_2ND_COLUMN, y, g_eeGeneral.timezone, attr|LEFT);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.timezone, -12, 12);
        break;

      case ITEM_SETUP_ADJUST_RTC:
        g_eeGeneral.adjustRTC = editCheckBox(g_eeGeneral.adjustRTC, RADIO_SETUP_2ND_COLUMN, y, STR_ADJUST_RTC, attr, event);
        break;
#endif
      case ITEM_SETUP_GPSFORMAT:
        g_eeGeneral.gpsFormat = editChoice(RADIO_SETUP_2ND_COLUMN, y, STR_GPSCOORD, STR_GPSFORMAT, g_eeGeneral.gpsFormat, 0, 1, attr, event);
        break;
#endif

#if defined(PXX)
      case ITEM_SETUP_COUNTRYCODE:
        g_eeGeneral.countryCode = editChoice(RADIO_SETUP_2ND_COLUMN, y, STR_COUNTRYCODE, STR_COUNTRYCODES, g_eeGeneral.countryCode, 0, 2, attr, event);
        break;
#endif

#if defined(VOICE)
      case ITEM_SETUP_LANGUAGE:
        lcdDrawTextAlignedLeft(y, STR_VOICELANG);
        lcdDrawText(RADIO_SETUP_2ND_COLUMN, y, currentLanguagePack->name, attr);
        if (attr) {
          currentLanguagePackIdx = checkIncDec(event, currentLanguagePackIdx, 0, DIM(languagePacks)-2, EE_GENERAL);
          if (checkIncDec_Ret) {
            currentLanguagePack = languagePacks[currentLanguagePackIdx];
            strncpy(g_eeGeneral.ttsLanguage, currentLanguagePack->id, 2);
          }
        }
        break;

      case ITEM_SETUP_IMPERIAL:
        g_eeGeneral.imperial = editChoice(RADIO_SETUP_2ND_COLUMN, y, STR_UNITSSYSTEM, STR_VUNITSSYSTEM, g_eeGeneral.imperial, 0, 1, attr, event);
        break;
#endif

      case ITEM_SETUP_PPM:
        g_eeGeneral.ppmunit = editChoice(LCD_W-2, y, STR_UNITS_PPM, "\004""0.--""0.0\0""us\0 ", g_eeGeneral.ppmunit, PPM_PERCENT_PREC0, PPM_US, attr|RIGHT, event);
        break;

#if defined(FAI_CHOICE)
      case ITEM_SETUP_FAI:
        editCheckBox(g_eeGeneral.fai, RADIO_SETUP_2ND_COLUMN, y, "FAI Mode", attr, event);
        if (attr && checkIncDec_Ret) {
          if (g_eeGeneral.fai)
            POPUP_WARNING("FAI\001mode blocked!");
          else
            POPUP_CONFIRMATION("FAI mode?");
        }
        break;
#endif


      case ITEM_SETUP_SWITCHES_DELAY:
        lcdDrawTextAlignedLeft(y, STR_SWITCHES_DELAY);
        lcdDrawNumber(LCD_W-14, y, 10*SWITCHES_DELAY(), attr|RIGHT);
        lcdDrawText(lcdLastRightPos, y, STR_MS, attr);
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.switchesDelay, -15, 100-15);
        break;

      case ITEM_SETUP_USB_MODE:
        g_eeGeneral.USBMode = editChoice(LCD_W-2, y, STR_USBMODE, STR_USBMODES, g_eeGeneral.USBMode, USB_UNSELECTED_MODE, USB_MAX_MODE, attr|RIGHT, event);
        break;

      case ITEM_SETUP_RX_CHANNEL_ORD:
        lcdDrawTextAlignedLeft(y, STR_RXCHANNELORD); // RAET->AETR
        for (uint32_t i=1; i<=4; i++) {
          putsChnLetter(LCD_W - 2 - 5*FW + i*FW, y, channelOrder(i), attr);
        }
        if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.templateSetup, 0, 23);
        break;

      case ITEM_SETUP_STICK_MODE_LABELS:
        lcdDrawTextAlignedLeft(y, STR_MODE);
        for (uint32_t i=0; i<4; i++) {
          lcdDraw1bitBitmap(5*FW+i*(4*FW+2), y, sticks, i, 0);
        }
        break;

      case ITEM_SETUP_STICK_MODE:
        lcdDrawChar(2*FW, y, '1'+reusableBuffer.generalSettings.stickMode, attr);
        for (uint32_t i=0; i<NUM_STICKS; i++) {
          drawSource((5*FW-3)+i*(4*FW+2), y, MIXSRC_Rud + *(modn12x3 + 4*reusableBuffer.generalSettings.stickMode + i), 0);
        }
        if (attr && s_editMode>0) {
          CHECK_INCDEC_GENVAR(event, reusableBuffer.generalSettings.stickMode, 0, 3);
        }
        else if (reusableBuffer.generalSettings.stickMode != g_eeGeneral.stickMode) {
          pausePulses();
          g_eeGeneral.stickMode = reusableBuffer.generalSettings.stickMode;
          checkThrottleStick();
          resumePulses();
          waitKeysReleased();
        }
        break;
    }
  }
}
