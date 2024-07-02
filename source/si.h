#pragma once

#include <stdint.h>

#define SI_ERROR_UNDER_RUN   0x0001
#define SI_ERROR_OVER_RUN    0x0002
#define SI_ERROR_COLLISION   0x0004
#define SI_ERROR_NO_RESPONSE 0x0008
#define SI_ERROR_WRST        0x0010
#define SI_ERROR_RDST        0x0020
#define SI_ERROR_UNKNOWN     0x0040
#define SI_ERROR_BUSY        0x0080

void SIInit();
int SIStartTransfer();

uint32_t SIGetStatus(int chan);
uint32_t SIGetStatus(int chan);

void SIRefreshSamplingRate();

void SISetCommand(int chan, uint32_t val);
void SIEnablePolling(uint32_t enabledBits);
void SIDisablePolling(uint32_t val);

bool SIGetResponse(int chan, uint32_t *inputBuffer);

void SIClearErrorBits(uint32_t chan);