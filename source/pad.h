#pragma once
#include <stdint.h>
#include <wut.h>

typedef struct WUT_PACKED PADStatus {
    uint16_t button;
    int8_t stickX;
    int8_t stickY;
    int8_t substickX;
    int8_t substickY;
    uint8_t triggerL;
    uint8_t triggerR;
    uint8_t analogA;
    uint8_t analogB;
    int8_t err;
    WUT_PADDING_BYTES(1);
} PADStatus;

WUT_CHECK_OFFSET(PADStatus, 0x0, button);
WUT_CHECK_OFFSET(PADStatus, 0x2, stickX);
WUT_CHECK_OFFSET(PADStatus, 0x3, stickY);
WUT_CHECK_OFFSET(PADStatus, 0x4, substickX);
WUT_CHECK_OFFSET(PADStatus, 0x5, substickY);
WUT_CHECK_OFFSET(PADStatus, 0x6, triggerL);
WUT_CHECK_OFFSET(PADStatus, 0x7, triggerR);
WUT_CHECK_OFFSET(PADStatus, 0x8, analogA);
WUT_CHECK_OFFSET(PADStatus, 0x9, analogB);
WUT_CHECK_OFFSET(PADStatus, 0xA, err);
WUT_CHECK_SIZE(PADStatus, 0xC);


typedef struct WUT_PACKED PADStatusWrapper {
    PADStatus data[4];
} PADStatusWrapper;
WUT_CHECK_OFFSET(PADStatusWrapper, 0x00, data[0]);
WUT_CHECK_OFFSET(PADStatusWrapper, 0x0C, data[1]);
WUT_CHECK_OFFSET(PADStatusWrapper, 0x18, data[2]);
WUT_CHECK_OFFSET(PADStatusWrapper, 0x24, data[3]);
WUT_CHECK_SIZE(PADStatusWrapper, 0x30);

#define PAD_ERR_NONE          0
#define PAD_ERR_NO_CONTROLLER (-1)
#define PAD_ERR_NOT_READY     (-2)
#define PAD_ERR_TRANSFER      (-3)

#define PAD_BUTTON_LEFT       0x0001
#define PAD_BUTTON_RIGHT      0x0002
#define PAD_BUTTON_DOWN       0x0004
#define PAD_BUTTON_UP         0x0008
#define PAD_TRIGGER_Z         0x0010
#define PAD_TRIGGER_R         0x0020
#define PAD_TRIGGER_L         0x0040
#define PAD_BUTTON_A          0x0100
#define PAD_BUTTON_B          0x0200
#define PAD_BUTTON_X          0x0400
#define PAD_BUTTON_Y          0x0800
#define PAD_BUTTON_MENU       0x1000
#define PAD_BUTTON_START      0x1000

#define PAD_MOTOR_STOP        0
#define PAD_MOTOR_RUMBLE      1
#define PAD_MOTOR_STOP_HARD   2

#define PAD_ENABLEDMASK(chan) (0x80000000 >> chan)

void PadInit();
void PADRead(PADStatusWrapper *status);
void PADControlMotor(int32_t chan, uint32_t cmd);
