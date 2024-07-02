#pragma once

#include <stdint.h>

enum OSDeviceID {
    OS_DEVICE_VI             = 0,
    OS_DEVICE_DSP            = 1,
    OS_DEVICE_GFXSP          = 2,
    OS_DEVICE_UNKOWN         = 3,
    OS_DEVICE_SI             = 6,
    OS_DEVICE_LATTE_REGS     = 11,
    OS_DEVICE_LATTE_SI       = 12,
    OS_DEVICE_LEGACY_AI_I2S3 = 13,
    OS_DEVICE_LEGACY_AI_I2S5 = 14,
    OS_DEVICE_LEGACY_EXI     = 15,
};

extern "C" uint32_t OSReadRegister16(OSDeviceID device, uint32_t id);
extern "C" uint32_t OSWriteRegister16(OSDeviceID device, uint32_t id, uint32_t val);
extern "C" uint32_t __OSReadRegister32Ex(OSDeviceID device, uint32_t id);
extern "C" uint32_t __OSWriteRegister32Ex(OSDeviceID device, uint32_t id, uint32_t value);
extern "C" void OSEnforceInorderIO();
