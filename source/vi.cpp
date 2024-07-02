#include "regs.h"
#include <coreinit/bsp.h>
#include <coreinit/debug.h>
#include <stdint.h>

static bool sIsInitialized = false;
static bool sVIIsHollywood = false;
static int sCurrTVMode     = {};

struct WUT_PACKED VITimings {
    uint8_t equ;
    WUT_PADDING_BYTES(1);
    uint16_t acv;
    uint16_t prbOdd, prbEven;
    uint16_t psbOdd, psbEven;
    uint8_t bs1, bs2, bs3, bs4;
    uint16_t be1, be2, be3, be4;
    uint16_t nhlines, hlw;
    uint8_t hsy, hcs, hce, hbe640;
    uint16_t hbs640;
    WUT_UNKNOWN_BYTES(4);
};
WUT_CHECK_OFFSET(VITimings, 0x00, equ);
WUT_CHECK_OFFSET(VITimings, 0x02, acv);
WUT_CHECK_OFFSET(VITimings, 0x04, prbOdd);
WUT_CHECK_OFFSET(VITimings, 0x06, prbEven);
WUT_CHECK_OFFSET(VITimings, 0x08, psbOdd);
WUT_CHECK_OFFSET(VITimings, 0x0A, psbEven);
WUT_CHECK_OFFSET(VITimings, 0x0C, bs1);
WUT_CHECK_OFFSET(VITimings, 0x0D, bs2);
WUT_CHECK_OFFSET(VITimings, 0x0E, bs3);
WUT_CHECK_OFFSET(VITimings, 0x0F, bs4);
WUT_CHECK_OFFSET(VITimings, 0x10, be1);
WUT_CHECK_OFFSET(VITimings, 0x12, be2);
WUT_CHECK_OFFSET(VITimings, 0x14, be3);
WUT_CHECK_OFFSET(VITimings, 0x16, be4);
WUT_CHECK_OFFSET(VITimings, 0x18, nhlines);
WUT_CHECK_OFFSET(VITimings, 0x1A, hlw);
WUT_CHECK_OFFSET(VITimings, 0x1C, hsy);
WUT_CHECK_OFFSET(VITimings, 0x1D, hcs);
WUT_CHECK_OFFSET(VITimings, 0x1E, hce);
WUT_CHECK_OFFSET(VITimings, 0x1F, hbe640);
WUT_CHECK_OFFSET(VITimings, 0x20, hbs640);
WUT_CHECK_SIZE(VITimings, 0x26);

static VITimings sVITimingsCase0 =
        {
                .equ     = 0x06,
                .__unk0  = {},
                .acv     = 0x00F0,
                .prbOdd  = 0x0018,
                .prbEven = 0x0019,
                .psbOdd  = 0x0003,
                .psbEven = 0x0002,
                .bs1     = 0x0C,
                .bs2     = 0x0D,
                .bs3     = 0x0C,
                .bs4     = 0x0D,
                .be1     = 0x0208,
                .be2     = 0x0207,
                .be3     = 0x0208,
                .be4     = 0x0207,
                .nhlines = 0x020D,
                .hlw     = 0x01AD,
                .hsy     = 0x40,
                .hcs     = 0x47,
                .hce     = 0x69,
                .hbe640  = 0xA2,
                .hbs640  = 0x0175,
                .__unk1  = {}};

VITimings *getTiming(int mode) {
    // different mode require different timing but we only call this function with 0 anyway.
    if (mode != 0) {
        OSFatal("VIGetTiming: Only mode 0 is supported");
        return nullptr;
    }
    return &sVITimingsCase0;
}

uint32_t
VIGetReg(uint32_t param_1) {
    if (sVIIsHollywood) {
        return OSReadRegister16(OS_DEVICE_VI, param_1 << 1);
    }
    auto uVar2 = __OSReadRegister32Ex(OS_DEVICE_UNKOWN, 3);
    __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 3, uVar2 | 1);
    OSEnforceInorderIO();
    __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 2, param_1);
    OSEnforceInorderIO();
    auto uVar3 = __OSReadRegister32Ex(OS_DEVICE_UNKOWN, 0);
    __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 3, uVar2);
    OSEnforceInorderIO();
    return uVar3 & 0xffff;
}

void __VISetReg(uint32_t val1, uint32_t val2) {
    uint32_t uVar1;

    if (sVIIsHollywood == 0) {
        uVar1 = __OSReadRegister32Ex(OS_DEVICE_UNKOWN, 3);
        __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 3, uVar1 | 1);
        OSEnforceInorderIO();
        __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 2, val1);
        OSEnforceInorderIO();
        __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 0, val2);
        OSEnforceInorderIO();
        __OSWriteRegister32Ex(OS_DEVICE_UNKOWN, 3, uVar1);
        OSEnforceInorderIO();
    } else {
        OSWriteRegister16(static_cast<OSDeviceID>(val2), 0, val1 << 1);
        OSEnforceInorderIO();
    }
}


void __VIInit(int mode) {
    BSPHardwareVersion hardwareVersion;

    if (bspGetHardwareVersion(&hardwareVersion) != BSP_ERROR_OK) {
        OSPanic("vi.c", 0x2e6, "%s: bspGetHardwareVersion failed\n", "__VIInit");
    }
    if ((hardwareVersion >> 0x18 != 0) && (hardwareVersion >> 0x18 != 0x10)) {
        sVIIsHollywood = false;
    }

    auto *timing = getTiming(mode);
    if (timing == nullptr) {
        OSPanic("vi.c", 0x2e6, "%s: getTiming failed\n", "__VIInit");
        return;
    }
    __VISetReg(1, 2);

    int uVar2 = 0;
    do {
        uVar2 = uVar2 + 1;
    } while (uVar2 < 1000);
    __VISetReg(1, 0);
    __VISetReg(3, timing->hlw);
    uint16_t tmp          = {};
    ((uint8_t *) &tmp)[0] = timing->hcs;
    ((uint8_t *) &tmp)[1] = timing->hce;
    __VISetReg(2, tmp);
    __VISetReg(5, (uint32_t) timing->hsy | (uint32_t) timing->hbe640 << 7);
    __VISetReg(4, (timing->hbs640 & 0x7fff) << 1);
    __VISetReg(0, timing->equ);
    __VISetReg(7, timing->prbOdd + timing->acv * 2 + -2);
    __VISetReg(6, timing->psbOdd + 2);
    __VISetReg(9, timing->prbEven + timing->acv * 2 + -2);
    __VISetReg(8, timing->psbEven + 2);
    __VISetReg(0xb, (uint32_t) timing->bs1 | (timing->be1 & 0x7ff) << 5);
    __VISetReg(10, (uint32_t) timing->bs3 | (timing->be3 & 0x7ff) << 5);
    __VISetReg(0xd, (uint32_t) timing->bs2 | (timing->be2 & 0x7ff) << 5);
    __VISetReg(0xc, (uint32_t) timing->bs4 | (timing->be4 & 0x7ff) << 5);
    __VISetReg(0x24, 0x2828);
    __VISetReg(0x1b, 1);
    __VISetReg(0x1a, 0x1001);
    uVar2 = timing->nhlines;
    __VISetReg(0x19, timing->hlw + 1);
    __VISetReg(0x18, ((uVar2 >> 1) + 1) | 0x1000);
    int modeU = mode >> 2;
    if ((modeU == 0) || (3 < modeU)) {
        modeU = 0;
    }
    if (((mode & 3) != 0) && ((mode & 3) != 1)) {
        __VISetReg(1, (modeU & 0xff) << 8 | 5);
        __VISetReg(0x36, 1);
        return;
    }
    __VISetReg(1, (mode & 1) << 2 | 1 | (modeU & 0xff) << 8);
    __VISetReg(0x36, 0);
}

void VIInit() {
    if (sIsInitialized) {
        return;
    }

    sIsInitialized = true;
    if ((VIGetReg(1) & 1) == 0) {
        __VIInit(0);
    }
    sCurrTVMode = VIGetReg(1) >> 8 & 3;
}
