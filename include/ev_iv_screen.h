#ifndef GUARD_EV_IV_SCREEN_H
#define GUARD_EV_IV_SCREEN_H

struct EvIvScreen
{
    u8 state;
    u8 gfxStep;
    u8 callbackStep;
    u8 currMon;
    u16 totalAttributes[3];
    u16 hatch;
    u16 tilemapBuffer[0x400];
};

extern EWRAM_DATA struct EvIvScreen gEvIvScreen;

#endif // GUARD_EV_IV_SCREEN_H
