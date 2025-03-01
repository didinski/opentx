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

void runPopupCurvePreset(event_t event)
{
  warningResult = false;

  drawMessageBox(warningText);

  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+2*FH, STR_POPUPS_ENTER_EXIT);

  switch (event) {
    case EVT_KEY_BREAK(KEY_ENTER):
      if (warningType == WARNING_TYPE_ASTERISK)
        break;
      warningResult = true;
      // no break

    case EVT_KEY_BREAK(KEY_EXIT):
      warningText = NULL;
      warningType = WARNING_TYPE_ASTERISK;
      break;

    default:
      s_editMode = EDIT_MODIFY_FIELD;
      reusableBuffer.curveEdit.preset = checkIncDec(event, reusableBuffer.curveEdit.preset, -4, +4);
      s_editMode = EDIT_SELECT_FIELD;
      break;
  }

  lcdDrawNumber(WARNING_LINE_X+FW*7, WARNING_LINE_Y, 45 * reusableBuffer.curveEdit.preset / 4, LEFT|INVERS);
  lcdDrawChar(lcdLastRightPos, WARNING_LINE_Y, '@', INVERS);

  if (warningResult) {
    warningResult = 0;
    CurveInfo & crv = g_model.curves[s_curveChan];
    int8_t * points = curveAddress(s_curveChan);
    int k = 25 * reusableBuffer.curveEdit.preset;
    int dx = 2000 / (5+crv.points-1);
    for (uint32_t i=0; i<5+crv.points; i++) {
      int x = -1000 + i * dx;
      points[i] = div_and_round(div_and_round(k * x, 100), 10);
    }
    if (crv.type == CURVE_TYPE_CUSTOM) {
      resetCustomCurveX(points, 5+crv.points);
    }
  }
}

void onCurveOneMenu(const char * result)
{
  if (result == STR_CURVE_PRESET) {
    reusableBuffer.curveEdit.preset = 4; // 45°
    POPUP_INPUT(STR_PRESET, runPopupCurvePreset);
  }
  else if (result == STR_MIRROR) {
    CurveInfo & crv = g_model.curves[s_curveChan];
    int8_t * points = curveAddress(s_curveChan);
    for (int i=0; i<5+crv.points; i++)
      points[i] = -points[i];
  }
  else if (result == STR_CLEAR) {
    CurveInfo & crv = g_model.curves[s_curveChan];
    int8_t * points = curveAddress(s_curveChan);
    for (int i=0; i<5+crv.points; i++)
      points[i] = 0;
    if (crv.type == CURVE_TYPE_CUSTOM) {
      resetCustomCurveX(points, 5+crv.points);
    }
  }
}

void menuModelCurveOne(event_t event)
{
  static uint8_t pointsOfs = 0;
  CurveData & crv = g_model.curves[s_curveChan];
  int8_t * points = curveAddress(s_curveChan);

  drawStringWithIndex(PSIZE(TR_MENUCURVES)*FW+FW, 0, "CV", s_curveChan+1);

  lcdDrawText(11*FW+FW/2, 0, TR_PT "\002X\006Y");
  lcdDrawFilledRect(0, 0, LCD_W, FH, SOLID, FILL_WHITE|GREY_DEFAULT);

  SIMPLE_SUBMENU(STR_MENUCURVES, 4 + 5+crv.points + (crv.type==CURVE_TYPE_CUSTOM ? 5+crv.points-2 : 0));

  // Curve name
  lcdDrawTextAlignedLeft(FH + 1, STR_NAME);
  editName(INDENT_WIDTH, 2 * FH + 1, crv.name, sizeof(crv.name), event, menuVerticalPosition == 0);
  
  // Curve type
  lcdDrawTextAlignedLeft(3 * FH + 1, NO_INDENT(STR_TYPE));
  LcdFlags attr = (menuVerticalPosition == 1 ? (s_editMode > 0 ? INVERS | BLINK : INVERS) : 0);
  lcdDrawTextAtIndex(INDENT_WIDTH, 4 * FH + 1, STR_CURVE_TYPES, crv.type, attr);
  if (attr) {
    uint8_t newType = checkIncDecModelZero(event, crv.type, CURVE_TYPE_LAST);
    if (newType != crv.type) {
      for (int i = 1; i < 4 + crv.points; i++) {
        points[i] = calcRESXto100(applyCustomCurve(calc100toRESX(getCurveX(5 + crv.points, i)), s_curveChan));
      }
      if (moveCurve(s_curveChan, checkIncDec_Ret > 0 ? 3 + crv.points : -3 - crv.points)) {
        if (newType == CURVE_TYPE_CUSTOM) {
          resetCustomCurveX(points, 5 + crv.points);
        }
        crv.type = newType;
      }
    }
  }

  // Curve points
  attr = (menuVerticalPosition==2 ? (s_editMode>0 ? INVERS|BLINK : INVERS) : 0);
  lcdDrawTextAlignedLeft(5*FH+1, STR_COUNT);
  lcdDrawNumber(INDENT_WIDTH, 6*FH+1, 5+crv.points, LEFT|attr);
  lcdDrawText(lcdLastRightPos, 6*FH+1, STR_PTS, attr);
  if (attr) {
    int8_t count = checkIncDecModel(event, crv.points, -3, 12); // 2pts - 17pts
    if (checkIncDec_Ret) {
      int8_t newPoints[MAX_POINTS_PER_CURVE];
      newPoints[0] = points[0];
      newPoints[4 + count] = points[4 + crv.points];
      for (int i = 1; i < 4 + count; i++)
        newPoints[i] = calcRESXto100(applyCustomCurve(calc100toRESX(getCurveX(5 + count, i)), s_curveChan));
      if (moveCurve(s_curveChan, checkIncDec_Ret * (crv.type == CURVE_TYPE_CUSTOM ? 2 : 1))) {
        for (int i = 0; i < 5 + count; i++) {
          points[i] = newPoints[i];
          if (crv.type == CURVE_TYPE_CUSTOM && i != 0 && i != 4 + count)
            points[5 + count + i - 1] = getCurveX(5 + count, i);
        }
        crv.points = count;
      }
    }
  }

  // Curve smooth
  lcdDrawTextAlignedLeft(7*FH+1, STR_SMOOTH);
  drawCheckBox(7 * FW, 7 * FH + 1, crv.smooth, menuVerticalPosition == 3 ? INVERS : 0);
  if (menuVerticalPosition==3) crv.smooth = checkIncDecModel(event, crv.smooth, 0, 1);

  switch (event) {
    case EVT_ENTRY:
      pointsOfs = 0;
      SET_SCROLLBAR_X(0);
      break;

    case EVT_KEY_LONG(KEY_ENTER):
      if (menuVerticalPosition > 1) {
        killEvents(event);
        POPUP_MENU_ADD_ITEM(STR_CURVE_PRESET);
        POPUP_MENU_ADD_ITEM(STR_MIRROR);
        POPUP_MENU_ADD_ITEM(STR_CLEAR);
        POPUP_MENU_START(onCurveOneMenu);
      }
      break;

    case EVT_KEY_LONG(KEY_MENU):
      pushMenu(menuChannelsView);
      killEvents(event);
      break;
  }

  drawCurve(FW);

  uint8_t posY = FH+1;
  attr = (s_editMode > 0 ? INVERS|BLINK : INVERS);
  for (uint32_t i=0; i<5+crv.points; i++) {
    point_t point = getPoint(i);
    uint8_t selectionMode = 0;
    if (crv.type==CURVE_TYPE_CUSTOM) {
      if (menuVerticalPosition==4+2*i || (i==5+crv.points-1 && menuVerticalPosition==4+5+crv.points+5+crv.points-2-1))
        selectionMode = 2;
      else if (i>0 && menuVerticalPosition==3+2*i)
        selectionMode = 1;
    }
    else if (menuVerticalPosition == 4+i) {
      selectionMode = 2;
    }

    if (i>=pointsOfs && i<pointsOfs+7) {
      int8_t x = getCurveX(5+crv.points, i);
      if (crv.type==CURVE_TYPE_CUSTOM && i>0 && i<5+crv.points-1) {
        x = points[5+crv.points+i-1];
      }
      lcdDrawNumber(6+10*FW+FW/2, posY, i+1, LEFT);
      lcdDrawNumber(3+14*FW, posY, x, LEFT|(selectionMode==1?attr:0));
      lcdDrawNumber(3+18*FW, posY, points[i], LEFT|(selectionMode==2?attr:0));
      posY += FH;
    }

    if (selectionMode > 0) {
      // Selection square
      lcdDrawFilledRect(point.x-FW-1, point.y-2, 5, 5, SOLID, FORCE);
      lcdDrawFilledRect(point.x-FW, point.y-1, 3, 3, SOLID);
      if (s_editMode > 0) {
        if (selectionMode == 1)
          CHECK_INCDEC_MODELVAR(event, points[5+crv.points+i-1], i==1 ? -100 : points[5+crv.points+i-2], i==5+crv.points-2 ? 100 : points[5+crv.points+i]);  // edit X
        else if (selectionMode == 2)
          CHECK_INCDEC_MODELVAR(event, points[i], -100, 100);
      }
      if (i < pointsOfs)
        pointsOfs = i;
      else if (i > pointsOfs+6)
        pointsOfs = i-6;
    }
  }
}
