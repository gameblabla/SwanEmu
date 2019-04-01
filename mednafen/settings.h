#ifndef MDFN_SETTINGS_H
#define MDFN_SETTINGS_H

#include <string.h>

uint64_t MDFN_GetSettingUI(const char *name);
int64_t MDFN_GetSettingI(const char *name);
uint32_t MDFN_GetSettingB(const char *name);
const char *MDFN_GetSettingS(const char *name);
#endif
