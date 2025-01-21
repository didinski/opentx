/**
 * ExpressLRS V3 configurator for i6X based on elrsV2/3.lua
 * @author Jan Kozak (ajjjjjjjj)
 *
 */
#include "opentx.h"

enum COMMAND_STEP {
    STEP_IDLE = 0,
    STEP_CLICK = 1,       // user has clicked the command to execute
    STEP_EXECUTING = 2,   // command is executing
    STEP_CONFIRM = 3,     // command pending user OK
    STEP_CONFIRMED = 4,   // user has confirmed
    STEP_CANCEL = 5,      // user has requested cancel
    STEP_QUERY = 6,       // UI is requesting status update
};

#define TYPE_UINT8				   0
#define TYPE_INT8				   1
#define TYPE_UINT16				   2
#define TYPE_INT16				   3
#define TYPE_FLOAT				   8
#define TYPE_SELECT                9
#define TYPE_STRING				  10
#define TYPE_FOLDER				  11
#define TYPE_INFO				  12
#define TYPE_COMMAND			  13
#define TYPE_BACK                 14
#define TYPE_DEVICE               15
#define TYPE_DEVICES_FOLDER       16

#define CRSF_FRAMETYPE_DEVICE_PING 0x28
#define CRSF_FRAMETYPE_DEVICE_INFO 0x29
#define CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY 0x2B
#define CRSF_FRAMETYPE_PARAMETER_READ 0x2C
#define CRSF_FRAMETYPE_PARAMETER_WRITE 0x2D
#define CRSF_FRAMETYPE_ELRS_STATUS 0x2E

/**
 * INT16 and FLOAT support:
 * Values (val,min,max[,step]) are keep in buffer after name.
 * prec - how many digits in fractional part.
 * FLOAT: { VALUE 4B | MIN 4B | MAX 4B | STEP 4B } = 16B
 * INT16: { VALUE 2B | MIN 2B | MAX 2B } =  6B
*/
struct Parameter {
  uint16_t offset;
  uint8_t type;
  uint8_t nameLength;
  uint8_t size;         // INT8/16/FLOAT/SELECT size
  uint8_t id;
  union {
#if defined(CRSF_EXTENDED_TYPES)
    int32_t value;
#else
    int16_t value;
#endif
    struct {
      uint8_t timeout;      // COMMAND
      uint8_t lastStatus;   // COMMAND
      uint8_t status;       // COMMAND
      uint8_t infoOffset;   // COMMAND, paramData info offset
    };
  };
};

struct ParamFunctions {
  void (*load)(Parameter*, uint8_t *, uint8_t);
  void (*save)(Parameter*);
  void (*display)(Parameter*, uint8_t, uint8_t);
};

static constexpr uint16_t BUFFER_SIZE = 720;
static uint8_t *buffer = &reusableBuffer.cToolData[0];
static uint16_t bufferOffset = 0;

static constexpr uint8_t PARAM_DATA_TAIL_SIZE = 44; // max popup packet size

static uint8_t *paramData = &reusableBuffer.cToolData[0];
static uint32_t paramDataLen = 0;

static constexpr uint8_t PARAMS_MAX_COUNT = 18;
static constexpr uint8_t PARAMS_SIZE = PARAMS_MAX_COUNT * sizeof(Parameter);
static Parameter *params = (Parameter *)&reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE];
static uint8_t allocatedParamsCount = 0;

static constexpr uint8_t DEVICES_MAX_COUNT = 8;
static uint8_t *deviceIds = &reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE + PARAMS_SIZE];
//static uint8_t deviceIds[DEVICES_MAX_COUNT];
static uint8_t devicesLen = 0;

static constexpr uint8_t backButtonId = 100;
static constexpr uint8_t otherDevicesId = 101;

enum {
  BTN_NONE,
  BTN_BACK,
  BTN_DEVICES,
};
static uint8_t btnState = BTN_NONE;

static uint8_t deviceId = 0xEE;
static uint8_t handsetId = 0xEF;

static constexpr uint8_t DEVICE_NAME_MAX_LEN = 20;
static char deviceName[DEVICE_NAME_MAX_LEN];
static uint8_t lineIndex = 1;
static uint8_t pageOffset = 0;
static Parameter * paramPopup = nullptr;
static tmr10ms_t paramTimeout = 0;
static uint8_t paramId = 1;
static uint8_t paramChunk = 0;

static struct LinkStat {
  uint16_t good;
  uint8_t bad;
  uint8_t flags;
} linkstat;

static constexpr uint8_t ELRS_FLAGS_INFO_MAX_LEN = 20;
static char elrsFlagsInfo[ELRS_FLAGS_INFO_MAX_LEN] = "";
static uint8_t expectedParamsCount = 0;

static tmr10ms_t devicesRefreshTimeout = 50;
static uint8_t allParamsLoaded = 0;
static uint8_t currentFolderId = 0;
static int8_t expectedChunks = -1;
static uint8_t deviceIsELRS_TX = 0;
static tmr10ms_t linkstatTimeout = 100;
static uint8_t titleShowWarn = 0;
static tmr10ms_t titleShowWarnTimeout = 100;

static constexpr uint8_t STRING_LEN_MAX = 15; // without trailing \0
static event_t currentEvent;

static constexpr uint8_t COL1          =  0;
static constexpr uint8_t COL2          = 70;
static constexpr uint8_t maxLineIndex  =  6;
static constexpr uint8_t textYoffset   =  3;
static constexpr uint8_t textSize      =  8;

#define getTime           get_tmr10ms
#define EVT_VIRTUAL_EXIT  EVT_KEY_BREAK(KEY_EXIT)
#define EVT_VIRTUAL_ENTER EVT_KEY_BREAK(KEY_ENTER)
#define EVT_VIRTUAL_NEXT  EVT_KEY_FIRST(KEY_DOWN)
#define EVT_VIRTUAL_PREV  EVT_KEY_FIRST(KEY_UP)

static constexpr uint8_t RESULT_NONE = 0;
static constexpr uint8_t RESULT_OK = 2;
static constexpr uint8_t RESULT_CANCEL = 1;

static void storeParam(Parameter * param);
static void clearParams();
static void addBackButton();
static void reloadAllParam();
static Parameter * getParam(uint8_t line);
static void paramBackExec(Parameter * param);
static void parseDeviceInfoMessage(uint8_t* data);
static void parseParameterInfoMessage(uint8_t* data, uint8_t length);
static void parseElrsInfoMessage(uint8_t* data);
static void runPopupPage(event_t event);
static void runDevicePage(event_t event);
static void lcd_title();
static void lcd_warn();
static void handleDevicePageEvent(event_t event);

static void luaLcdDrawGauge(coord_t x, coord_t y, coord_t w, coord_t h, int32_t val, int32_t max) {
  uint8_t len = limit<uint8_t>(1, w*val/max, w);
  lcdDrawSolidFilledRect(x+len, y, w - len, h-2);
}

static void bufferPush(char * data, uint8_t len) {
  memcpy(&buffer[bufferOffset], data, len);
  bufferOffset += len;
}

static void resetParamData() {
  paramData = &reusableBuffer.cToolData[bufferOffset];
  paramDataLen = 0;
}

static void crossfireTelemetryCmd(const uint8_t cmd, const uint8_t index, const uint8_t * data, const uint8_t size) {
  // TRACE("crsf cmd %x %x %x", cmd, index, size);
  uint8_t crsfPushData[3 + size] = { deviceId, handsetId, index };
  for (uint32_t i = 0; i < size; i++) {
    crsfPushData[3 + i] = data[i];
  }
  crossfireTelemetryPush(cmd, crsfPushData, sizeof(crsfPushData));
}

static void crossfireTelemetryCmd(const uint8_t cmd, const uint8_t index, const uint8_t value) {
  crossfireTelemetryCmd(cmd, index, &value, 1);
}

static void crossfireTelemetryPing(){
  const uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(CRSF_FRAMETYPE_DEVICE_PING, (uint8_t *) crsfPushData, 2);
}

static void clearParams() {
//  TRACE("clearParams %d", allocatedParamsCount);
  memclear(params, PARAMS_SIZE);
  btnState = BTN_NONE;
  allocatedParamsCount = 0;
}

// Both buttons must be added as last ones because i cannot overwrite existing Id
static void addBackButton() {
  Parameter backBtnParam;
  backBtnParam.id = backButtonId;
  backBtnParam.nameLength = 1; // mark as present
  backBtnParam.type = TYPE_BACK;
  storeParam(&backBtnParam);
  btnState = BTN_BACK;
}

static void addOtherDevicesButton() {
  Parameter otherDevicesParam;
  otherDevicesParam.id = otherDevicesId;
  otherDevicesParam.nameLength = 1;
  otherDevicesParam.type = TYPE_DEVICES_FOLDER;
  storeParam(&otherDevicesParam);
  btnState = BTN_DEVICES;
}

static void reloadAllParam() {
//  TRACE("reloadAllParam");
  allParamsLoaded = 0;
  paramId = 1;
  paramChunk = 0;
  paramDataLen = 0;
  bufferOffset = 0;
}

static Parameter * getParamById(const uint8_t id) {
  for (uint32_t i = 0; i < allocatedParamsCount; i++) {
    if (params[i].id == id)
      return &params[i];
  }
  return nullptr;
}

/**
 * Store param at its location or add new one if not found.
 */
static void storeParam(Parameter * param) {
  Parameter * storedParam = getParamById(param->id);
  if (storedParam == nullptr) {
    storedParam = &params[allocatedParamsCount];
    allocatedParamsCount++;
  }
  memcpy(storedParam, param, sizeof(Parameter));
}

static int32_t paramGetValue(uint8_t * data, uint8_t size) {
  int32_t result = 0;
  for (uint32_t i = 0; i < size; i++) {
    result = (result << 8) + data[i];
  }
  return result;
}

static int32_t paramGetMin(Parameter * param) {
  return paramGetValue(&buffer[param->offset + param->nameLength + (0 * param->size)], param->size);
}

static int32_t paramGetMax(Parameter * param) {
  return paramGetValue(&buffer[param->offset + param->nameLength + (1 * param->size)], param->size);
}

/**
 * Get param from line index taking only loaded current folder params into account.
 */
static Parameter * getParam(const uint8_t line) {
  return &params[line - 1];
}

//static void incrParam(int32_t step) {
//  Parameter * param = getParam(lineIndex);
//  int32_t min = paramGetMin(param);
//  int32_t max = paramGetMax(param);
//  param->value = limit<int32_t>(min, param->value + step, max);
//}

static void selectParam(int8_t step) {
  int32_t newLineIndex = lineIndex + step;

  if (newLineIndex <= 0) {
    newLineIndex = allocatedParamsCount;
  } else if (newLineIndex > allocatedParamsCount) {
    newLineIndex = 1;
    pageOffset = 0;
  }

  Parameter * param;
  do {
    param = getParam(newLineIndex);
    if (param != 0 && param->nameLength != 0) break;  // Valid param found
    newLineIndex = newLineIndex + step;
    if (newLineIndex <= 0) newLineIndex = allocatedParamsCount;
    if (newLineIndex > allocatedParamsCount) newLineIndex = 1;
  } while (newLineIndex != lineIndex);

  lineIndex = newLineIndex;

  if (lineIndex > maxLineIndex + pageOffset) {
    pageOffset = lineIndex - maxLineIndex;
  } else if (lineIndex <= pageOffset) {
    pageOffset = lineIndex - 1;
  }
}

static bool isExistingDevice(uint8_t devId) {
//   TRACE("isExistingDevice %x", devId);
  for (uint32_t i = 0; i < devicesLen; i++) {
    if (deviceIds[i] == devId) {
      return true;
    }
  }
  return false;
}

static void unitLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]) + 1;
  bufferPush((char*)&data[offset], len);
}

static void unitDisplay(Parameter * param, uint8_t y, uint16_t offset) {
  lcdDrawText(lcdLastRightPos, y, (char *)&buffer[offset]);
}

static void paramIntegerDisplay(Parameter *param, uint8_t y, uint8_t attr) {
    int32_t value = param->value;
    uint8_t offset = param->offset + param->nameLength + (2 * param->size);
    if (param->type == TYPE_FLOAT) {
#if defined(CRSF_EXTENDED_TYPES)
      uint8_t prec = buffer[offset];
      if (prec > 0) {
        attr |= (prec == 1 ? PREC1 : PREC2);
      }
      offset += 1; // skip prec
#else
      return;
#endif
    }
    lcdDrawNumber(COL2, y, (param->type == TYPE_UINT8) ? (uint8_t)value :
                          (param->type == TYPE_INT8)  ? (int8_t)value :
                          (param->type == TYPE_UINT16) ? (uint16_t)value :
#if defined(CRSF_EXTENDED_TYPES)
                          (param->type == TYPE_INT16) ? (int16_t)value : (int32_t)value, attr);
#else
                          (int16_t)value, attr);
#endif
    unitDisplay(param, y, offset);
}

static void paramIntegerLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t size = (param->type == TYPE_UINT16 || param->type == TYPE_INT16) ? 2 : 1; // else INT8, SELECT
  uint8_t loadSize = 2 * size; // min + max at once
#if defined(CRSF_EXTENDED_TYPES)
  if (param->type == TYPE_FLOAT) {
    size = 4;
    loadSize = 13; // min + max + prec + step at once
  }
#endif
  param->size = size;
  uint8_t valuesLen = 0;
  if (param->type == TYPE_SELECT) {
    valuesLen = strlen((char*)&data[offset]) + 1; // + \0
  }
  param->value = paramGetValue(&data[offset + valuesLen + (0 * size)], size);
  bufferPush((char *)&data[offset + valuesLen + (1 * size)], loadSize); // min + max at once
  bufferPush((char*)&data[offset], valuesLen); // TYPE_SELECT values
  unitLoad(param, data, offset + valuesLen + loadSize + 2 * size);
}

static void paramStringDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  char * str = (char *)&buffer[param->offset + param->nameLength];
  if (param->type == TYPE_INFO) 
    lcdDrawText(COL2, y, str, attr);
#if defined(CRSF_EXTENDED_TYPES)
  else
    editName(COL2, y, str, 10/* max len to fit screen */, currentEvent, attr);
#endif
}
static void paramStringLoad(Parameter * param, uint8_t * data, uint8_t offset) {
#if defined(CRSF_EXTENDED_TYPES)
  uint8_t len = strlen((char*)&data[offset]);
  // if (len) param->maxlen = data[offset + len + 1];
  char tmp[STRING_LEN_MAX] = {0};
  str2zchar(tmp, (char*)&data[offset], len);
  bufferPush(tmp, STRING_LEN_MAX);
#endif
}

static void paramStringSave(Parameter * param) {
#if defined(CRSF_EXTENDED_TYPES)
  char tmp[STRING_LEN_MAX + 1];
  zchar2str(tmp, (char*)&buffer[param->offset + param->nameLength], STRING_LEN_MAX);
  crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, (uint8_t *)&tmp, strlen(tmp) + 1);
#endif
}

static void paramMultibyteSave(Parameter * param) {
  uint8_t data[4];
  for (uint32_t i = 0; i < param->size; i++) {
      data[i] = (uint8_t)((param->value >> (8 * i)) & 0xFF);
    }
  crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, (uint8_t *)&data, param->size);
}

static void paramInfoLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]) + 1;
  bufferPush((char*)&data[offset], len); // info + \0
}

static bool getSelectedOption(char * option, char * options, const uint8_t value) {
  uint8_t current = 0;
  while (*options != '\0') {
    if (current == value) break;
    if (*options++ == ';') current++;
  }
  while (*options != ';' && *options != '\0') {
    *option++ = *options++;
  }
 *option = '\0';
  return (current == value);
}

static void paramTextSelectionDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  const uint32_t valuesOffset = param->offset + param->nameLength + 2 /* min, max */;
  const uint32_t valuesLen = strlen((char*)&buffer[valuesOffset]);
  char option[24];
  if (!getSelectedOption(option, (char*)&buffer[valuesOffset], param->value)) {
    option[0] = 'E';
    option[1] = 'R';
    option[2] = 'R';
    option[3] = '\0';
  }
  lcdDrawText(COL2, y, option, attr);
  unitDisplay(param, y, valuesOffset + valuesLen + 1);
}

static void paramFolderOpen(Parameter * param) {
  //TRACE("paramFolderOpen %d", param->id);
  lineIndex = 1;
  pageOffset = 0;
  currentFolderId = param->id;
  reloadAllParam();
  if (param->type == TYPE_FOLDER) { // guard because it is reused for devices
    paramId = param->id + 1; // UX hack: start loading from first folder item to fetch it faster
  }
  clearParams();
}

static void paramFolderDeviceOpen(Parameter * param) {
  // if currentFolderId == devices folder, store only devices instead of params
  expectedParamsCount = devicesLen;
  devicesLen = 0;
  crossfireTelemetryPing(); //broadcast with standard handset ID to get all node respond correctly
  paramFolderOpen(param);
}

static void noopLoad(Parameter * param, uint8_t * data, uint8_t offset) {}
static void noopSave(Parameter * param) {}

static void paramCommandLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  param->status = data[offset];
  param->timeout = data[offset+1];
  param->infoOffset = offset+2; // do not copy info, access directly
  if (param->status == STEP_IDLE) {
    paramPopup = nullptr;
  }
}

static void paramCommandSave(Parameter * param) {
  if (param->status < STEP_CONFIRMED) {
    param->status = STEP_CLICK;
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, param->status);
    paramPopup = param;
    paramPopup->lastStatus = STEP_IDLE;
    paramTimeout = getTime() + param->timeout;
  }
}

static void paramUnifiedDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  uint8_t textIndent = COL1 + 9;
  char tmp[28];
  char * tmpString = tmp;
  if (param->type == TYPE_FOLDER) {
    tmpString = strAppend(tmpString, "> ");
    strAppend(tmpString, (char *)&buffer[param->offset], param->nameLength);
    textIndent = COL1;
  } else if (param->type == TYPE_DEVICES_FOLDER) {
    strAppend(tmpString, "> Other Devices");
    textIndent = COL1;
  } else { // CMD || DEVICE || BACK
    tmpString = strAppend(tmpString, "[");
    if (param->type == TYPE_BACK) tmpString = strAppend(tmpString, "----BACK----");
    else tmpString = strAppend(tmpString, (char *)&buffer[param->offset], param->nameLength);
    strAppend(tmpString, "]");
  }
  lcdDrawText(textIndent, y, tmp, attr | BOLD);
}

static void paramBackExec(Parameter * param = 0) {
  currentFolderId = 0;
  clearParams();
  reloadAllParam();
  devicesLen = 0;
  expectedParamsCount = 0;
}

static void changeDeviceId(uint8_t devId) {
  //TRACE("changeDeviceId %x", devId);
  currentFolderId = 0;
  deviceIsELRS_TX = 0;
  //if the selected device ID (target) is a TX Module, we use our Lua ID, so TX Flag that user is using our LUA
  if (devId == 0xEE) {
    handsetId = 0xEF;
  } else { //else we would act like the legacy lua
    handsetId = 0xEA;
  }
  deviceId = devId;
  expectedParamsCount = 0; //set this because next target wouldn't have the same count, and this trigger to request the new count
}

static void paramDeviceIdSelect(Parameter * param) {
//  TRACE("paramDeviceIdSelect %x", param->id);
 changeDeviceId(param->id);
 crossfireTelemetryPing();
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
// TRACE("parseDev:%x, exp:%d, devs:%d", id, expectedParamsCount, devicesLen);
  offset = strlen((char*)&data[3]) + 1 + 3;
  if (!isExistingDevice(id)) {
    deviceIds[devicesLen] = id;
    devicesLen++;
    if (currentFolderId == otherDevicesId) { // if "Other Devices" opened store devices to params
      Parameter deviceParam;
      deviceParam.id = id;
      deviceParam.type = TYPE_DEVICE;
      deviceParam.nameLength = offset - 4;
      deviceParam.offset = bufferOffset;

      bufferPush((char *)&data[3], deviceParam.nameLength);
      storeParam(&deviceParam);
      if (devicesLen == expectedParamsCount) { // was it the last one?
        allParamsLoaded = 1;
      }
    }
  }

  if (deviceId == id && currentFolderId != otherDevicesId) {
    memcpy(&deviceName[0], (char *)&data[3], DEVICE_NAME_MAX_LEN);
    deviceIsELRS_TX = ((paramGetValue(&data[offset], 4) == 0x454C5253) && (deviceId == 0xEE)); // SerialNumber = 'E L R S' and ID is TX module
    uint8_t newParamCount = data[offset+12];
//    TRACE("deviceId match %x, newParamCount %d", deviceId, newParamCount);
    reloadAllParam();
    if (newParamCount != expectedParamsCount || newParamCount == 0) {
      expectedParamsCount = newParamCount;
      clearParams();
      if (newParamCount == 0) {
      // This device has no params so the Loading code never starts
        allParamsLoaded = 1;
      }
    }
  }
}

static const ParamFunctions functions[] = {
  { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay },    // UINT8(0), common for INTs, FLOAT
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // INT8(1)
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // UINT16(2)
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // INT16(3)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=paramFloatLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // FLOAT(8)
  { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramTextSelectionDisplay }, // SELECT(9)
  { .load=paramStringLoad, .save=paramStringSave, .display=paramStringDisplay }, // STRING(10) editing
  { .load=noopLoad, .save=paramFolderOpen, .display=paramUnifiedDisplay }, // FOLDER(11)
  { .load=paramInfoLoad, .save=noopSave, .display=paramStringDisplay }, // INFO(12)
  { .load=paramCommandLoad, .save=paramCommandSave, .display=paramUnifiedDisplay }, // COMMAND(13)
  { .load=noopLoad, .save=paramBackExec, .display=paramUnifiedDisplay }, // back(14)
  { .load=noopLoad, .save=paramDeviceIdSelect, .display=paramUnifiedDisplay }, // device(15)
  { .load=noopLoad, .save=paramFolderDeviceOpen, .display=paramUnifiedDisplay }, // deviceFOLDER(16)
};

static ParamFunctions getFunctions(uint32_t i) {
  if (i <= TYPE_FLOAT) return functions[0];
  else return functions[i - 8];
}

static void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
  // TRACE("parse %d...", data[3]);
  // DUMP(&data[4], length - 4);
  if (data[2] != deviceId || data[3] != paramId) {
    paramDataLen = 0;
    paramChunk = 0;
    return;
  }
  if (paramDataLen == 0) {
    expectedChunks = -1;
  }

  // Get by id or use temporary one to decide later if it should be stored
  Parameter tempParam = {0};
  Parameter* param = getParamById(paramId);
  if (param == nullptr) {
    param = &tempParam;
  }

  uint8_t chunksRemain = data[4];
  // If no param or the chunksRemain changed when we have data, don't continue
  if (/*param == 0 ||*/ (chunksRemain != expectedChunks && expectedChunks != -1)) {
    return;
  }
  expectedChunks = chunksRemain - 1;

  // skip on first chunk of not current folder
  if (paramDataLen == 0 && data[5] != currentFolderId) {
    if (paramId == expectedParamsCount) {
      allParamsLoaded = 1;
    }
    paramChunk = 0;
    paramId++;
    return;
  }

  memcpy(&paramData[paramDataLen], &data[5], length - 5);
  paramDataLen += length - 5;

  if (chunksRemain > 0) {
    paramChunk = paramChunk + 1;
  } else {
    paramChunk = 0;
    if (paramDataLen < 4) {
      paramDataLen = 0;
      return;
    }

    param->id = paramId;
    uint8_t parent = paramData[0];
    uint8_t type = paramData[1] & 0x7F;
    uint8_t hidden = paramData[1] & 0x80;

    if (param->nameLength != 0) {
      if (currentFolderId != parent || param->type != type/* || param->hidden != hidden*/) {
        paramDataLen = 0;
        return;
      }
    }

    param->type = type;
    uint8_t nameLen = strlen((char*)&paramData[2]);

    if (parent != currentFolderId) {
      param->nameLength = 0; // mark as clear
    } else if (!hidden) {
      if (param->nameLength == 0) {
        param->nameLength = nameLen;
        param->offset = bufferOffset;
        bufferPush((char*)&paramData[2], param->nameLength);
      }
      getFunctions(param->type).load(param, paramData, 2 + nameLen + 1);
      storeParam(param);
    }

    if (paramPopup == nullptr) {
      if (paramId == expectedParamsCount) { // if we have loaded all params
        allParamsLoaded = 1;
      } else if (allParamsLoaded == 0) {
        paramId++; // paramId = 1 + (paramId % (paramsLen-1));
      }
      paramTimeout = getTime() + 200;
    } else {
      paramTimeout = getTime() + paramPopup->timeout;
    }
    resetParamData();
  }
}

static void parseElrsInfoMessage(uint8_t* data) {
  if (data[2] != deviceId) {
    paramDataLen = 0;
    paramChunk = 0;
    return;
  }

  linkstat.bad = data[3];
  linkstat.good = paramGetValue(&data[4], 2);
  uint8_t newFlags = data[6];
  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (newFlags != linkstat.flags) {
    linkstat.flags = newFlags;
    titleShowWarnTimeout = 0;
  }
  strncpy(elrsFlagsInfo, (char*)&data[7], ELRS_FLAGS_INFO_MAX_LEN);
}

static void refreshNextCallback(uint8_t command, uint8_t* data, uint8_t length) {
  if (command == CRSF_FRAMETYPE_DEVICE_INFO) {
    parseDeviceInfoMessage(data);
  } else if (command == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY && currentFolderId != otherDevicesId /* !devicesFolderOpened */) {
    parseParameterInfoMessage(data, length);
    if (allParamsLoaded < 1) {
      paramTimeout = 0; // request next chunk immediately
    }
  } else if (command == CRSF_FRAMETYPE_ELRS_STATUS) {
    parseElrsInfoMessage(data);
  }

  if (btnState == BTN_NONE && allParamsLoaded) {
    if (currentFolderId == 0) {
      if (devicesLen > 1) addOtherDevicesButton();
    } else {
      addBackButton();
    }
  }
}

static void refreshNext() {
  tmr10ms_t time = getTime();
  if (paramPopup != nullptr) {
    if (time > paramTimeout && paramPopup->status != STEP_CONFIRM) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_QUERY);
      paramTimeout = time + paramPopup->timeout;
    }
  } else if (time > devicesRefreshTimeout && expectedParamsCount < 1) {
    devicesRefreshTimeout = time + 100;
    crossfireTelemetryPing();
  } else if (time > paramTimeout && expectedParamsCount != 0) {
    if (allParamsLoaded < 1) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramId, paramChunk);
      paramTimeout = time + 50; // 0.5s
    }
  }

  if (deviceIsELRS_TX && time > linkstatTimeout) {
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x0, 0x0); // request linkstat
    linkstatTimeout = time + 100;
  }

  if (time > titleShowWarnTimeout) {
    titleShowWarn = (linkstat.flags > 3) ? !titleShowWarn : 0;
    titleShowWarnTimeout = time + 100;
  }
}

static void lcd_title() {
  lcdClear();

  const uint8_t barHeight = 9;
  if (deviceIsELRS_TX && !titleShowWarn) {
    char tmp[16];
    char * tmpString = tmp;
    tmpString = strAppendUnsigned(tmpString, linkstat.bad);
    strAppendStringWithIndex(tmpString, "/", linkstat.good);
    lcdDrawText(LCD_W - 11, 1, tmp, RIGHT);
    lcdDrawVerticalLine(LCD_W - 10, 0, barHeight, SOLID, INVERS);
    lcdDrawChar(LCD_W - 7, 1, (linkstat.flags & 1) ? 'C' : '-');
  }

  lcdDrawFilledRect(0, 0, LCD_W, barHeight, SOLID);
  if (allParamsLoaded != 1 && expectedParamsCount > 0) {
    luaLcdDrawGauge(0, 1, COL2, barHeight, paramId, expectedParamsCount);
  } else {
    const char* textToDisplay = titleShowWarn ? elrsFlagsInfo :
                            (allParamsLoaded == 1) ? (char *)&deviceName[0] : "Loading...";
    uint8_t textLen = titleShowWarn ? ELRS_FLAGS_INFO_MAX_LEN : DEVICE_NAME_MAX_LEN;
    lcdDrawSizedText(COL1, 1, textToDisplay, textLen, INVERS);
  }
}

static void lcd_warn() {
  lcdDrawText(COL1, textSize*2, "Error:");
  lcdDrawText(COL1, textSize*3, elrsFlagsInfo);
  lcdDrawText(LCD_W/2, textSize*5, TR_ENTER, BLINK + INVERS + CENTERED);
}

static void handleDevicePageEvent(event_t event) {
  if (allocatedParamsCount == 0) { // if there is no param yet
    return;
  } else {
    // Will stuck on main page because back button is not present
    // if (getParamById(backButtonId)/*->nameLength*/ == nullptr) { // if back button is not assigned yet, means there is no param yet.
    //   return;
    // }
  }

  Parameter * param = getParam(lineIndex);

  if (event == EVT_VIRTUAL_EXIT) {
    if (s_editMode) {
      s_editMode = 0;
      paramTimeout = getTime() + 200;
      paramId = param->id;
      paramChunk = 0;
      paramDataLen = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramId, paramChunk);
    } else {
      if (currentFolderId == 0 && allParamsLoaded == 1) {
        if (deviceId != 0xEE) {
          changeDeviceId(0xEE); // change device id clear expectedParamsCount, therefore the next ping will do reloadAllParam()
        } else {
//          reloadAllParam(); // paramBackExec does it
        }
        crossfireTelemetryPing();
      }
      paramBackExec();
    }
  } else if (event == EVT_VIRTUAL_ENTER) {
    if (linkstat.flags > 0x1F) {
      linkstat.flags = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x2E, 0x00);
    } else {
      if (param != 0 && param->nameLength > 0) {
        if (param->type < TYPE_FOLDER) {
          s_editMode = (s_editMode) ? 0 : 1;
        }
        if (!s_editMode) {
          if (param->type == TYPE_COMMAND) {
            // For commands, request this param's
            // data again, with a short delay to allow the module EEPROM to
            // commit. Do this before save() to allow save to override
            paramId = param->id;
            paramChunk = 0;
            paramDataLen = 0;
          }
          paramTimeout = getTime() + 20;
          getFunctions(param->type).save(param);
          if (param->type < TYPE_FOLDER) {
            // For editable param types reload whole folder, but do it after save
            clearParams();
            reloadAllParam();
            paramId = currentFolderId + 1; // Start loading from first folder item
          }
        }
      }
    }
  } else if (s_editMode) {
    if (param->type == TYPE_STRING) {
      return;
    }
//    if (event == EVT_VIRTUAL_NEXT) {
//      incrParam(1);
//    } else if (event == EVT_VIRTUAL_PREV) {
//      incrParam(-1);
//    }
    // smaller but missing step support (FLOAT)
    param->value = checkIncDec(event, param->value, paramGetMin(param), paramGetMax(param), 0);
  } else {
    if (event == EVT_VIRTUAL_NEXT) {
      selectParam(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      selectParam(-1);
    }
  }
}

static void runDevicePage(event_t event) {
  currentEvent = event;
  handleDevicePageEvent(event);

  lcd_title();

  if (linkstat.flags > 0x1F) {
    lcd_warn();
  } else {
    Parameter * param;
    for (uint32_t y = 1; y < maxLineIndex + 2; y++) {
      if (pageOffset + y > allocatedParamsCount) break;
      param = getParam(pageOffset + y);
      if (param == nullptr) {
        break;
      } else if (param->nameLength > 0) {
        uint8_t attr = (lineIndex == (pageOffset+y)) ? ((s_editMode && BLINK) + INVERS) : 0;
        if (param->type < TYPE_FOLDER || param->type == TYPE_INFO) { // if not folder, command, or back
          lcdDrawSizedText(COL1, y * textSize+textYoffset, (char *)&buffer[param->offset], param->nameLength, 0);
        }
        getFunctions(param->type).display(param, y*textSize+textYoffset, attr);
      }
    }
  }
}

static uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&paramData[paramPopup->infoOffset]);
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+4*FH+2, STR_POPUPS_ENTER_EXIT);

  if (event == EVT_VIRTUAL_EXIT) {
    return RESULT_CANCEL;
  } else if (event == EVT_VIRTUAL_ENTER) {
    return RESULT_OK;
  }
  return RESULT_NONE;
}

static void runPopupPage(event_t event) {
  if (event == EVT_VIRTUAL_EXIT) {
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CANCEL);
    paramTimeout = getTime() + 200;
  }

  uint8_t result = RESULT_NONE;
  if (paramPopup->status == STEP_IDLE && paramPopup->lastStatus != STEP_IDLE) { // stopped
      popupCompat(event);
      reloadAllParam();
      paramPopup = nullptr;
  } else if (paramPopup->status == STEP_CONFIRM) { // confirmation required
    result = popupCompat(event);
    paramPopup->lastStatus = paramPopup->status;
    if (result == RESULT_OK) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CONFIRMED);
      paramTimeout = getTime() + paramPopup->timeout; // we are expecting an immediate response
      paramPopup->status = STEP_CONFIRMED;
    } else if (result == RESULT_CANCEL) {
      paramPopup = nullptr;
    }
  } else if (paramPopup->status == STEP_EXECUTING) {
    result = popupCompat(event);
    paramPopup->lastStatus = paramPopup->status;
    if (result == RESULT_CANCEL) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CANCEL);
      paramTimeout = getTime() + paramPopup->timeout;
      paramPopup = nullptr;
    }
  }
}

void elrsStop() {
  registerCrossfireTelemetryCallback(nullptr);
  // reloadAllParam();
  paramBackExec();
  paramPopup = nullptr;
  deviceId = 0xEE;
  handsetId = 0xEF;

  globalData.cToolRunning = 0;
  memset(reusableBuffer.cToolData, 0, sizeof(reusableBuffer.cToolData));
  popMenu();
}

void elrsRun(event_t event) {
  if (globalData.cToolRunning == 0) {
    globalData.cToolRunning = 1;
    registerCrossfireTelemetryCallback(refreshNextCallback);
  }

  if (event == EVT_KEY_LONG(KEY_EXIT)) {
    elrsStop();
  } else { 
    if (paramPopup != nullptr) {
      runPopupPage(event);
    } else {
      runDevicePage(event);
    }

    refreshNext();
  }
}
