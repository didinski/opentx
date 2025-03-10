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

#ifndef _POPUPS_H_
#define _POPUPS_H_

#define MENU_X                         30
#define MENU_Y                         16
#define MENU_W                         LCD_W-(2*MENU_X)
#define WARNING_LINE_LEN               32
#define WARNING_LINE_X                 16
#define WARNING_LINE_Y                 3*FH

void drawMessageBox(const char * title);
void showMessageBox(const char * title);
void runPopupWarning(event_t event);

extern void (*popupFunc)(event_t event);
extern uint8_t warningInfoFlags;

#define DISPLAY_WARNING                (*popupFunc)
#define POPUP_WARNING(s)               (warningText = s, warningInfoText = 0, popupFunc = runPopupWarning)
#define POPUP_CONFIRMATION(s)          (warningText = s, warningType = WARNING_TYPE_CONFIRM, warningInfoText = 0, popupFunc = runPopupWarning)
#define POPUP_INPUT(s, func)           (warningText = s, popupFunc = func)
#define SET_WARNING_INFO(info, len, flags) (warningInfoText = info, warningInfoLength = len, warningInfoFlags = flags)

#define POPUP_MENU_ADD_ITEM(s)         do { popupMenuOffsetType = MENU_OFFSET_INTERNAL; if (popupMenuItemsCount < POPUP_MENU_MAX_LINES) popupMenuItems[popupMenuItemsCount++] = s; } while (0)
#define POPUP_MENU_SELECT_ITEM(s)      popupMenuSelectedItem =  (s > 0 ? (s < popupMenuItemsCount ? s : popupMenuItemsCount) : 0)
#define POPUP_MENU_START(func)         do { popupMenuHandler = (func); AUDIO_KEY_PRESS(); } while (0)
#define POPUP_MENU_MAX_LINES           12
#define MENU_MAX_DISPLAY_LINES         6
#define POPUP_MENU_ADD_SD_ITEM(s)      POPUP_MENU_ADD_ITEM(s)
#define MENU_LINE_LENGTH               (LEN_MODEL_NAME+12)

extern const char * popupMenuItems[POPUP_MENU_MAX_LINES];
extern uint16_t popupMenuItemsCount;
extern uint16_t popupMenuOffset;
enum {
  MENU_OFFSET_INTERNAL,
  MENU_OFFSET_EXTERNAL
};
extern uint8_t popupMenuOffsetType;
extern uint8_t popupMenuSelectedItem;
const char * runPopupMenu(event_t event);
extern void (*popupMenuHandler)(const char * result);

#endif // _POPUPS_H_
