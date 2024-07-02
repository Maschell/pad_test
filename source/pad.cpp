#include "pad.h"
#include "si.h"
#include "vi.h"
#include <coreinit/mutex.h>

static OSMutex sPADMutex           = {};
static bool sPADInitialized = false;
static uint32_t sPADEnabledBits    = {};
static uint32_t sPADAnalogMode     = 0x300;

void PADMakeStatus(PADStatus *status, const uint32_t *data) {
    uint32_t analogMode = sPADAnalogMode & 0x700;

    status->stickX = (int8_t) (data[0] >> 8);
    status->stickY = (int8_t) data[0];
    status->button = (uint16_t) (data[0] >> 0x10) & 0x3fff;

    if (analogMode == 0x100) {
        status->substickX = (int8_t) ((data[1] >> 24) & 0xf0);
        status->substickY = (int8_t) ((data[1] >> 8) & 0xff);
        status->triggerL  = (uint8_t) ((data[1] >> 16) & 0xff);
        status->triggerR  = (uint8_t) ((data[1] >> 8) & 0xff);
        status->analogA   = (uint8_t) (data[1] & 0xf0);
        status->analogB   = (uint8_t) ((data[1] << 4) & 0xf0);
    } else if (analogMode == 0x200) {
        status->substickX = (int8_t) ((data[1] >> 24) & 0xf0);
        status->substickY = (int8_t) ((data[1] >> 20) & 0xf0);
        status->triggerL  = (uint8_t) ((data[1] >> 16) & 0xf0);
        status->triggerR  = (uint8_t) ((data[1] >> 12) & 0xf0);
        status->analogA   = (uint8_t) ((data[1] >> 8) & 0xff);
        status->analogB   = (int8_t) data[1] & 0xff;
    } else if (analogMode == 0x300) {
        status->substickX = (int8_t) ((data[1] >> 24) & 0xff);
        status->substickY = (int8_t) ((data[1] >> 16) & 0xff);
        status->triggerL  = (uint8_t) ((data[1] >> 8) & 0xff);
        status->triggerR  = (uint8_t) data[1] & 0xff;
        status->analogA   = 0;
        status->analogB   = 0;
    } else if (analogMode == 0x400) {
        status->substickX = (int8_t) ((data[1] >> 24) & 0xff);
        status->substickY = (int8_t) ((data[1] >> 16) & 0xff);
        status->triggerL  = 0;
        status->triggerR  = 0;
        status->analogA   = (uint8_t) ((data[1] >> 8) & 0xff);
        status->analogB   = (uint8_t) data[1] & 0xff;
    } else if (!analogMode || analogMode == 0x500 || analogMode == 0x600 || analogMode == 0x700) {
        status->substickX = (int8_t) ((data[1] >> 24) & 0xff);
        status->substickY = (int8_t) ((data[1] >> 16) & 0xff);
        status->triggerL  = (uint8_t) ((data[1] >> 8) & 0xf0);
        status->triggerR  = (uint8_t) ((data[1] >> 4) & 0xf0);
        status->analogA   = (uint8_t) (data[1] & 0xf0);
        status->analogB   = (uint8_t) ((data[1] << 4) & 0xf0);
    }

    status->stickX -= 128;
    status->stickY -= 128;
    status->substickX -= 128;
    status->substickY -= 128;
    // TODO: clamp sticks?
}

void PADRead(PADStatusWrapper *status) {
    uint32_t inputBuffer[2] = {};
    SIStartTransfer();
    OSLockMutex(&sPADMutex);
    for (int chan = 0; chan < 4; chan++) {
        if ((sPADEnabledBits & PAD_ENABLEDMASK(chan)) == 0) {
            status->data[chan]     = {};
            status->data[chan].err = PAD_ERR_NO_CONTROLLER;
        } else {
            uint32_t siStatus = SIGetStatus(chan);
            if (siStatus & SI_ERROR_NO_RESPONSE) {
                status->data[chan]     = {};
                status->data[chan].err = PAD_ERR_NO_CONTROLLER;
                if (SIGetResponse(chan, inputBuffer) != 0) {
                    SIClearErrorBits(chan);
                }
            } else {
                if (SIGetResponse(chan, inputBuffer) == 0 || ((inputBuffer[0] & 0x80000000) != 0)) {
                    status->data[chan]     = {};
                    status->data[chan].err = PAD_ERR_TRANSFER;
                } else {
                    status->data[chan].err = PAD_ERR_NOT_READY;
                    PADMakeStatus(&status->data[chan], inputBuffer);
                    status->data[chan].err    = PAD_ERR_NONE;
                    status->data[chan].button = ~0x80;
                }
            }
        }
    }
    OSUnlockMutex(&sPADMutex);
}

void PADDisable(int chan) {
    OSLockMutex(&sPADMutex);
    uint32_t res = 0x80000000 >> (chan & 0x3fU);
    if ((sPADEnabledBits & res) != 0) {
        sPADEnabledBits = sPADEnabledBits & ~res;
        SIDisablePolling(res);
    }
    OSUnlockMutex(&sPADMutex);
}

void PADEnable(int chan) {
    uint32_t buffer[2] = {};

    OSLockMutex(&sPADMutex);
    uint32_t enabledMask = PAD_ENABLEDMASK(chan);
    if (!(sPADEnabledBits & enabledMask)) {
        sPADEnabledBits = sPADEnabledBits | enabledMask;
        SIGetResponse(chan, buffer);
        SISetCommand(chan, sPADAnalogMode | 0x400000);
        SIEnablePolling(sPADEnabledBits);
    }
    OSUnlockMutex(&sPADMutex);
}

const char *PADDriver_Name() {
    return "PAD";
}

void PADDriver_OnAcquiredForeground() {
    for (int chan = 0; chan < 4; chan++) {
        PADEnable(chan);
    }
}

void PADDriver_OnReleaseForeground() {
    for (int chan = 0; chan < 4; chan++) {
        PADDisable(chan);
    }
}

extern "C" void OSDriver_Register(uint32_t, uint32_t, void *, uint32_t, uint32_t, uint32_t, uint32_t);

void PadInit() {
    if (sPADInitialized) {
        return;
    }

    OSInitMutex(&sPADMutex);
    sPADInitialized = true;
    VIInit();
    SIInit();
    for (int chan = 0; chan < 4; chan++) {
        PADEnable(chan);
    }

    uint32_t driverData[5] = {};
    driverData[0]          = reinterpret_cast<uint32_t>(&PADDriver_Name);
    driverData[2]          = reinterpret_cast<uint32_t>(&PADDriver_OnAcquiredForeground);
    driverData[3]          = reinterpret_cast<uint32_t>(&PADDriver_OnReleaseForeground);

    OSDriver_Register(0, 0xfa, driverData, 1, 0, 0, 0);
}
