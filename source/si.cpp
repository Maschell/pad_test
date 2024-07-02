#include "si.h"
#include "regs.h"
#include <coreinit/debug.h>
#include <stdint.h>


uint32_t sSIInputBuffer[4][2] = {};
bool sSIInputBufferValid[4] = {};

uint32_t sSIPolling = 0;
bool sSIInitialized = false;

static struct _xy {
    uint16_t line;
    uint8_t cnt;
} sXYData[2][12] = {
        {{0x00F6, 0x02}, {0x000F, 0x12}, {0x001E, 0x09}, {0x002C, 0x06}, {0x0034, 0x05}, {0x0041, 0x04}, {0x0057, 0x03}, {0x0057, 0x03}, {0x0057, 0x03}, {0x0083, 0x02}, {0x0083, 0x02}, {0x0083, 0x02}},
        {{0x0128, 0x02}, {0x000F, 0x15}, {0x001D, 0x0B}, {0x002D, 0x07}, {0x0034, 0x06}, {0x003F, 0x05}, {0x004E, 0x04}, {0x0068, 0x03}, {0x0068, 0x03}, {0x0068, 0x03}, {0x0068, 0x03}, {0x009C, 0x02}}};

int32_t sSISamplingRate = 0;

void SISetXY(uint32_t line, uint32_t cnt) {
   sSIPolling = (sSIPolling & 0xfc0000ff) | line << 0x10 | cnt << 8;
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0xC, sSIPolling);
   OSEnforceInorderIO();
}

void SISetSamplingRate(int samplingRate) {
   if (samplingRate > 11) {
      samplingRate = 11;
   }
   sSISamplingRate = samplingRate;

   int div = 1;
   if ((OSReadRegister16(OS_DEVICE_VI, 0x6c) & 1) != 0) {
      div = 2;
   }

   SISetXY(div * sXYData[0][samplingRate].line, sXYData[0][samplingRate].cnt);
}

void SIRefreshSamplingRate() {
   SISetSamplingRate(sSISamplingRate);
}

void SIDisablePolling(uint32_t val) {
   if (val == 0) {
      return;
   }
   sSIPolling = sSIPolling & ~(val >> 0x18 & 0xf0);
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0x0C, sSIPolling);
   OSEnforceInorderIO();
}


void SIInit() {
   if (sSIInitialized) {
      return;
   }
   sSIPolling = 0;
   SISetSamplingRate(0);
   uint32_t res;
   OSEnforceInorderIO();


   do {
      res = __OSReadRegister32Ex(OS_DEVICE_SI, 0xd);
   } while ((res & 1) != 0);
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0xd, 0x80000000);
   OSEnforceInorderIO();
   sSIInitialized = true;
}

int SIStartTransfer() {
   int ret;
   do {
      ret = __OSReadRegister32Ex(OS_DEVICE_SI, 0xd);
   } while ((ret & 1U) != 0);
   return ret;
}

uint32_t SIGetStatus(int chan) {
   uint32_t ret;
   ret = __OSReadRegister32Ex(OS_DEVICE_SI, 0xe);
   return ret >> ((3 - chan) << 3);
}

static void SIGetResponseRaw(int chan) {
   sSIInputBuffer[chan][0] = __OSReadRegister32Ex(OS_DEVICE_SI, (chan * 3 + 1U) & 0x3fffffff);
   sSIInputBuffer[chan][1] = __OSReadRegister32Ex(OS_DEVICE_SI, (chan * 3 + 2U) & 0x3fffffff);
   sSIInputBufferValid[chan] = true;
}

void SISetCommand(int chan, uint32_t val) {
   __OSWriteRegister32Ex(OS_DEVICE_SI, chan * 3 & 0x3fffffff, val);
   OSEnforceInorderIO();
}

static void SITransferCommands() {
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0x0e, 0x80000000);
   OSEnforceInorderIO();
}

void SIEnablePolling(uint32_t enabledBits) {
   if (enabledBits == 0) {
      return;
   }

   sSIPolling = (sSIPolling & ~(enabledBits >> 0x1c)) | (enabledBits >> 0x18 & (enabledBits >> 0x1c | 0x3fffff0));
   SITransferCommands();
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0x0c, sSIPolling);
   OSEnforceInorderIO();
}

bool SIGetResponse(int chan, uint32_t *inputBuffer) {
   SIGetResponseRaw(chan);
   bool chanIsValid = sSIInputBufferValid[chan];
   sSIInputBufferValid[chan] = false;
   if (chanIsValid) {
      inputBuffer[0] = sSIInputBuffer[chan][0];
      inputBuffer[1] = sSIInputBuffer[chan][1];
   }
   return chanIsValid;
}

void SIClearErrorBits(uint32_t chan) {
   uint32_t ret = __OSReadRegister32Ex(OS_DEVICE_SI, 0xe);
   __OSWriteRegister32Ex(OS_DEVICE_SI, 0xe, ret & 0xf000000 >> ((chan & 7) << 3));
   OSEnforceInorderIO();
}
