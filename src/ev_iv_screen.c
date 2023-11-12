#include "global.h"
#include "diploma.h"
#include "palette.h"
#include "main.h"
#include "gpu_regs.h"
#include "scanline_effect.h"
#include "task.h"
#include "malloc.h"
#include "decompress.h"
#include "bg.h"
#include "window.h"
#include "string_util.h"
#include "text.h"
#include "overworld.h"
#include "menu.h"
#include "pokedex.h"
#include "constants/rgb.h"
#include "ev_iv_screen.h"
#include "script.h"
#include "sound.h"
#include "party_menu.h"
#include "field_effect.h"
#include "constants/songs.h"
#include "strings.h"
#include "string_util.h"

extern void CB2_ShowEvIv(void);
extern void Task_EvIvInit(u8);
static u8 EvIvLoadGfx(void);
static void EvIvVblankHandler(void);
static void Task_WaitForExit(u8);
static void Task_EvIvReturnToOverworld(u8);
static void EvIvPrintText(struct Pokemon *mon);
static void AddColorByNature(u8 nature, u8 up1, u8 up2, u8 up3, u8 up4, u8 down1, u8 down2, u8 down3, u8 down4);
static void AddNumber(u16 num);
static void ShowPokemonPic(u16 species, u8 x, u8 y);
static void Task_ScriptShowMonPic(u8 taskId);
static void PicboxCancel2(void);

static const u32 sEvIvScreenTilemap[] = INCBIN_U32("graphics/diploma/tilemap.bin.lz");
static const u32 sEvIvScreenTiles[] = INCBIN_U32("graphics/diploma/tiles.4bpp.lz");
static const u32 sEvIvScreenPalette[] = INCBIN_U32("graphics/diploma/national.gbapal");

EWRAM_DATA struct EvIvScreen gEvIvScreen = {0};

void ShowEvIvScreen(void)
{
    SetMainCallback2(CB2_ShowEvIv);
    LockPlayerFieldControls();
}

static void EvIvBgInit(void)
{
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    ResetTasks();
    ScanlineEffect_Stop();
}

static void CB2_EvIv(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

void CB2_ShowEvIv(void)
{
    gEvIvScreen.state = 0;
    gEvIvScreen.gfxStep = 0;
    gEvIvScreen.callbackStep = 0;
    gEvIvScreen.currMon = 0;
    EvIvBgInit();
    CreateTask(Task_EvIvInit, 0);
    SetMainCallback2(CB2_EvIv);
}

static const struct BgTemplate plantilaBG[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 12,
        .screenSize = 0, // 0 = 256x256;  1 = 512x256;  2 = 256x51
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 1,
    },
    {
        .bg = 1,
        .charBaseIndex = 2,
        .mapBaseIndex = 22,
        .screenSize = 0, // 0 = 256x256;  1 = 512x256;  2 = 256x51
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0,
    },
};

#define SCREEN_WIDTH    17
#define SCREEN_HEIGHT   11
#define SCREEN2_WIDTH   29
#define SCREEN2_HEIGHT   2

static const struct WindowTemplate plantilaWindow[] = {
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 4,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .paletteNum = 15,
        .baseBlock = 0x000
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 2,
        .width = SCREEN2_WIDTH,
        .height = SCREEN2_HEIGHT,
        .paletteNum = 15,
        .baseBlock = SCREEN_WIDTH * SCREEN_HEIGHT //(width * height) + baseBlock del WindowTemplate anterior
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 15,
        .width = 29,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = SCREEN2_WIDTH * SCREEN2_HEIGHT + (SCREEN_WIDTH * SCREEN_HEIGHT)
    },
    DUMMY_WIN_TEMPLATE,
};


static void VCBC_EvIvOam(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void Task_EvIvInit(u8 taskId)
{
    switch (gEvIvScreen.callbackStep)
    {
    case 0:
        SetVBlankCallback(NULL);
        break;
    case 1:
        EvIvVblankHandler();
        break;
    case 2:
        if (!EvIvLoadGfx())
            return;
        break;
    case 3:
        CopyToBgTilemapBuffer(1, sEvIvScreenTilemap, 0, 0);
        break;
    case 4:
        SetGpuReg(REG_OFFSET_BG1HOFS, 0);
        break;
    case 5:
        EvIvPrintText(&gPlayerParty[gEvIvScreen.currMon]);
        break;
    case 6:
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        break;
    case 7:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        break;
    case 8:
        SetVBlankCallback(VCBC_EvIvOam);
        break;
    default:
        if (gPaletteFade.active)
            break;
        gTasks[taskId].func = Task_WaitForExit;
    }
    gEvIvScreen.callbackStep++;
}

static void Task_WaitForExit(u8 taskId)
{
    switch (gEvIvScreen.state)
    {
    case 0:
        PlaySE(SE_CARD);
        gEvIvScreen.state++;
        break;
    case 1:
        if (JOY_NEW(DPAD_DOWN) && gPlayerPartyCount > 1)
        {
            PlaySE(SE_SELECT);
            if (gEvIvScreen.currMon == (gPlayerPartyCount - 1))
                gEvIvScreen.currMon = 0;
            else
                gEvIvScreen.currMon++;
            PicboxCancel2();
            EvIvPrintText(&gPlayerParty[gEvIvScreen.currMon]);
        }
        if (JOY_NEW(DPAD_UP) && gPlayerPartyCount > 1)
        {
            PlaySE(SE_SELECT);
            if (gEvIvScreen.currMon == 0)
                gEvIvScreen.currMon = (gPlayerPartyCount - 1);
            else
                gEvIvScreen.currMon--;
            PicboxCancel2();
            EvIvPrintText(&gPlayerParty[gEvIvScreen.currMon]);
        }
        if (JOY_NEW(A_BUTTON) || JOY_NEW(B_BUTTON))
        {
            PlaySE(SE_CARD);
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            gEvIvScreen.state++;
        }
        break;
    case 2:
        Task_EvIvReturnToOverworld(taskId);
        break;
    }
}

static void Task_EvIvReturnToOverworld(u8 taskId)
{
    if (gPaletteFade.active)
        return;
    DestroyTask(taskId);
    FreeAllWindowBuffers();
    SetMainCallback2(CB2_ReturnToField);
}

static void EvIvVblankHandler(void)
{
    void *vram = (void *)VRAM;
    DmaClearLarge16(3, vram, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, plantilaBG, 2);
    ChangeBgX(0, 0, 0);
    ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0);
    ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0);
    ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0);
    ChangeBgY(3, 0, 0);
    InitWindows(plantilaWindow);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetBgTilemapBuffer(1, gEvIvScreen.tilemapBuffer);
    ShowBg(0);
    ShowBg(1);
    FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 30, 20);
    FillBgTilemapBufferRect_Palette0(1, 0, 0, 0, 30, 20);
}

static u8 EvIvLoadGfx(void)
{
    switch (gEvIvScreen.gfxStep)
    {
    case 0:
        ResetTempTileDataBuffers();
        break;
    case 1:
        DecompressAndCopyTileDataToVram(1, sEvIvScreenTiles, 0, 0, 0);
        break;
    case 2:
        if (!(FreeTempTileDataBuffersIfPossible() == 1))
            break;
        return 0;
    case 3:
        LoadPalette(sEvIvScreenPalette, 0x10, 0x20);
    default:
        return 1;
    }

    gEvIvScreen.gfxStep++;
    return 0;
}

/**
 * Parámetro 'u8 color' en AddTextPrinterParameterized3 que
 * define el color que tendrá el texto a imprimir, usando los
 * colores de la paleta de la ventana de la siguiente manera:
 * static const ALIGNED(4) u8 COLOR[3] = {FONDO, FUENTE, SOMBRA};
 * -----Colores PAL14 y PAL15----- (0=transparente)
 * 1=blanco         2=negro         3=gris
 * 4=rojo           5=rojo claro    6=verde
 * 7=verde claro    8=azul          9=azul claro
*/

static const ALIGNED(4) u8 sBlackTextColor[3]  = {0, 2, 3};  //paleta 15 usada en plantilaWindow
static const ALIGNED(4) u8 sGreyTextColor[3]   = {0, 3, 2};  //paleta 15 usada en plantilaWindow

/**
 * COLOR_HIGHLIGHT_SHADOW = FC 04 @ takes 3 bytes
 * Cambio de color en línea de texto, FC 04 + 3 bytes.
 * Son bytes de texto que el juego interpreta como cambio de color
 * en el párrafo durante la impresión del texto, reemplaza el color
 * de la fuente del texto que le precede, usando los colores de la
 * paleta de la ventana, se configura de la siguiene manera:
 * static const u8 color[] = {0xFC, 0x04, fuente, fondo, sombra, 0xFF};
 * Es concatenado antes del texto a cambiar de color.
*/

static const u8 sBlackColor[]   = {0xFC, 0x04, 0x02, 0x00, 0x03, 0xFF}; // Colors based on the Palette 15 used in WindowTemplate
static const u8 sBlueColor[]    = {0xFC, 0x04, 0x08, 0x00, 0x03, 0xFF};
static const u8 sRedColor[]    = {0xFC, 0x04, 0x04, 0x00, 0x03, 0xFF};

static const u8 sText_TwoEmptySpaces[] = _("  ");
static const u8 sText_Newline[] = _("\n");

static const u8 sText_BsEvIv[] = _("  BS   EV   IV"); // Base Stats, Effort Values, Individual Values

static const u8 sText_Your[] = _("Your ");
static const u8 sText_Is[] = _(" is ");
static const u8 sText_Happy[] = _("% happy!");

static const u8 sText_LessThan[] = _("Less than ");
static const u8 sText_StepsLeftToHatch[] = _(" steps left to hatch!");

// This function prints all the juicy information on the screen.
// We're taking advantage of the big space allocated for gStringVar4
// to fit all of our text in it before actually printing it.
static void EvIvPrintText(struct Pokemon *mon)
{
    u8 temp     = 0;
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u8 nature   = GetNature(mon);
    u8 isEgg    = GetMonData(mon, MON_DATA_IS_EGG, NULL);

    FillWindowPixelBuffer(0, 0);
    FillWindowPixelBuffer(1, 0);
    FillWindowPixelBuffer(2, 0);

    // Print the party slot of the species that is currently being checked.
    temp = gEvIvScreen.currMon + 1;
    ConvertIntToDecimalStringN(gStringVar4, temp, 2, 1);
    StringAppend(gStringVar4, gText_Slash);
    temp = gPlayerPartyCount;
    ConvertIntToDecimalStringN(gStringVar1, temp, 2, 1);
    StringAppend(gStringVar4, gStringVar1);
    AddTextPrinterParameterized3(1, 2, 12, 2, sGreyTextColor, 0, gStringVar4);

    // Print "BS EV IV" on screen followed by the name/nickname of the species.
    // If what's currently being checked is a Pokémon Egg, it'll only display that string.
    if (!isEgg)
    {
        StringCopy(gStringVar4, sText_BsEvIv); // StringCopy can be used to reset the contents of gStringVar4.
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, gText_Space);
        GetMonNickname(mon, gStringVar1);
        StringAppend(gStringVar4, gStringVar1);
        AddTextPrinterParameterized3(1, 2, 68, 2, sGreyTextColor, 0, gStringVar4);
    }
    else
    {
        StringCopy(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, gText_Space);
        StringAppend(gStringVar4, gText_Pokemon);
        StringAppend(gStringVar4, gText_Space);
        GetMonNickname(mon, gStringVar1);
        StringAppend(gStringVar4, gStringVar1);
        AddTextPrinterParameterized3(1, 2, 128, 2, sGreyTextColor, 0, gStringVar4);
    }

    // Print the different stats' names on screen, unless what's currently being checked is a Pokémon Egg.
    if (!isEgg)
    {
        StringCopy(gStringVar4, gText_HP3);
        StringAppend(gStringVar4, sText_Newline);
        AddColorByNature(nature, 1, 2, 3, 4, 5, 10, 15, 20);
        StringAppend(gStringVar4, gText_Attack);
        StringAppend(gStringVar4, sText_Newline);
        AddColorByNature(nature, 5, 7, 8, 9, 1, 11, 16, 21);
        StringAppend(gStringVar4, gText_Defense);
        StringAppend(gStringVar4, sText_Newline);
        AddColorByNature(nature, 15, 16, 17, 19, 3, 8, 13, 23);
        StringAppend(gStringVar4, gText_SpAtk);
        StringAppend(gStringVar4, sText_Newline);
        AddColorByNature(nature, 20, 21, 22, 23, 4, 9, 14, 19);
        StringAppend(gStringVar4, gText_SpDef);
        StringAppend(gStringVar4, sText_Newline);
        AddColorByNature(nature, 10, 11, 13, 14, 2, 7, 17, 22);
        StringAppend(gStringVar4, gText_Speed);
        AddTextPrinterParameterized3(0, 2, 4, 3, sBlackTextColor, 0, gStringVar4);
    }

    // Print a Pokémon's stats on screen. If what's currently being checked is a Pokémon Egg, nothing gets printed.
    if (!isEgg)
    {
        // HP Stat
        temp = gSpeciesInfo[species].baseHP; // Base HP
        gEvIvScreen.totalAttributes[0] = temp;
        if (temp < 10)
        {
            StringCopy(gStringVar4, sText_TwoEmptySpaces);
            ConvertIntToDecimalStringN(gStringVar2, temp, STR_CONV_MODE_RIGHT_ALIGN, 1); // 1 digit
            StringAppend(gStringVar4, gStringVar2);
        }
        else if (temp < 100)
        {
            StringCopy(gStringVar4, gText_Space);
            ConvertIntToDecimalStringN(gStringVar2, temp, STR_CONV_MODE_RIGHT_ALIGN, 2); // 2 digits
            StringAppend(gStringVar4, gStringVar2);
        }
        else
        {
            ConvertIntToDecimalStringN(gStringVar4, temp, STR_CONV_MODE_RIGHT_ALIGN, 3); // 3 digits
        }

        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_HP_EV, NULL); // HP Effort Values
        gEvIvScreen.totalAttributes[1] = temp;
        AddNumber(temp);
        StringAppend(gStringVar4, gText_Space);
        temp = GetMonData(mon, MON_DATA_HP_IV, NULL); // HP Individual Values
        gEvIvScreen.totalAttributes[2] = temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Newline);

        // Attack Stat
        temp = gSpeciesInfo[species].baseAttack; // Base Attack
        gEvIvScreen.totalAttributes[0] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_ATK_EV, NULL); // Attack Effort Values
        AddNumber(temp);
        gEvIvScreen.totalAttributes[1] += temp;
        StringAppend(gStringVar4, gText_Space);
        AddColorByNature(nature, 1, 2, 3, 4, 5, 10, 15, 20);
        temp = GetMonData(mon, MON_DATA_ATK_IV, NULL); // Attack Individual Values
        gEvIvScreen.totalAttributes[2] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Newline);

        // Defense Stat
        StringAppend(gStringVar4, sBlackColor);
        temp = gSpeciesInfo[species].baseDefense; // Base Defense
        gEvIvScreen.totalAttributes[0] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_DEF_EV, NULL); // Defense Effort Values
        gEvIvScreen.totalAttributes[1] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, gText_Space);
        AddColorByNature(nature, 5, 7, 8, 9, 1, 11, 16, 21);
        temp = GetMonData(mon, MON_DATA_DEF_IV, NULL); // Defense Individual Values
        gEvIvScreen.totalAttributes[2] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Newline);

        // Sp. Atk. Stat
        StringAppend(gStringVar4, sBlackColor);
        temp = gSpeciesInfo[species].baseSpAttack; // Base Sp. Atk.
        gEvIvScreen.totalAttributes[0] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_SPATK_EV, NULL); // Sp. Atk. Effort Values
        gEvIvScreen.totalAttributes[1] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, gText_Space);
        AddColorByNature(nature, 15, 16, 17, 19, 3, 8, 13, 23);
        temp = GetMonData(mon, MON_DATA_SPATK_IV, NULL); // Sp. Atk. Individual Values
        gEvIvScreen.totalAttributes[2] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Newline);

        // Sp. Def. stat
        StringAppend(gStringVar4, sBlackColor);
        temp = gSpeciesInfo[species].baseSpDefense; // Base Sp. Def.
        gEvIvScreen.totalAttributes[0] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_SPDEF_EV, NULL); // Sp. Def. Effort Values
        gEvIvScreen.totalAttributes[1] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, gText_Space);
        AddColorByNature(nature, 20, 21, 22, 23, 4, 9, 14, 19);
        temp = GetMonData(mon, MON_DATA_SPDEF_IV, NULL); // Sp. Def. Individual Values
        gEvIvScreen.totalAttributes[2] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Newline);

        // Speed Stat
        StringAppend(gStringVar4, sBlackColor);
        temp = gSpeciesInfo[species].baseSpeed; // Base Speed Stat
        gEvIvScreen.totalAttributes[0] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        temp = GetMonData(mon, MON_DATA_SPEED_EV, NULL); // Speed Effort Values
        gEvIvScreen.totalAttributes[1] += temp;
        AddNumber(temp);
        StringAppend(gStringVar4, gText_Space);
        AddColorByNature(nature, 10, 11, 13, 14, 2, 7, 17, 22);
        temp = GetMonData(mon, MON_DATA_SPEED_IV, NULL); // Speed Individual Values
        gEvIvScreen.totalAttributes[2] += temp;
        AddNumber(temp);
        AddTextPrinterParameterized3(0, 2, 54, 3, sBlackTextColor, 0, gStringVar4);
    }
    else
    {
        StringCopy(gStringVar4, gText_EmptyString2);
        AddTextPrinterParameterized3(0, 2, 54, 3, sBlackTextColor, 0, gStringVar4);
    }

    // Friendship / Egg Cycles
    if (!isEgg)
    {
        StringCopy(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, gText_Total);
        StringAppend(gStringVar4, gText_Colon2);
        StringAppend(gStringVar4, gText_Space);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, gText_Space);
        AddNumber(gEvIvScreen.totalAttributes[0]); // Total sum of Base Stats
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        AddNumber(gEvIvScreen.totalAttributes[1]); // Total sum of Effort Values
        StringAppend(gStringVar4, gText_Space);
        AddNumber(gEvIvScreen.totalAttributes[2]); // Total sum of Individual Values
        AddTextPrinterParameterized3(2, 2, 6, 0, sBlackTextColor, 0, gStringVar4);
    }

    // Adds "Your Pokémon is X% happy!"
    if (!isEgg)
    {
        StringCopy(gStringVar4, sText_Newline);
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_Your);
        GetSpeciesName(species);
        StringAppend(gStringVar4, gStringVar1);
        StringAppend(gStringVar4, sText_Is);
        temp = (GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) * 100) / 0xFF;
        AddNumber(temp);
        StringAppend(gStringVar4, sText_Happy);
        AddTextPrinterParameterized3(2, 2, 6, 0, sBlackTextColor, 0, gStringVar4);
    }
    else
    {
        StringAppend(gStringVar4, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, sText_LessThan);
        gEvIvScreen.hatch = ((GetMonData(mon, MON_DATA_FRIENDSHIP, NULL) + 1) * 0xFF);
        if (gEvIvScreen.hatch < 10)
            ConvertIntToDecimalStringN(gStringVar2, gEvIvScreen.hatch, 2, 1);
        else if (gEvIvScreen.hatch < 100)
            ConvertIntToDecimalStringN(gStringVar2, gEvIvScreen.hatch, 2, 2);
        else if (gEvIvScreen.hatch < 1000)
            ConvertIntToDecimalStringN(gStringVar2, gEvIvScreen.hatch, 2, 3);
        else if (gEvIvScreen.hatch < 10000)
            ConvertIntToDecimalStringN(gStringVar2, gEvIvScreen.hatch, 2, 4);
        else
            ConvertIntToDecimalStringN(gStringVar2, gEvIvScreen.hatch, 2, 5);
        StringAppend(gStringVar4, gStringVar2);
        StringAppend(gStringVar4, sText_StepsLeftToHatch);
        AddTextPrinterParameterized3(2, 2, 2, 4, sBlackTextColor, 0, gStringVar4);
    }

    // Print the windows containing the different text strings on screen.
    PutWindowTilemap(0);
    PutWindowTilemap(1);
    PutWindowTilemap(2);

    // Print the species sprite on screen and play their cry.
    // If a Pokémon Egg is being checked, simply print SPECIES_EGG's sprite.
    if (!isEgg)
    {
        ShowPokemonPic(species, 18, 5);// X and Y are calculated by tiles (number * 8 pixels) in BG0
        PlayCry_Normal(species, 0);
    }
    else
    {
        ShowPokemonPic(SPECIES_EGG, 18, 5);
    }
}

static void PicboxCancel2(void)
{
    u8 taskId = FindTaskIdByFunc(Task_ScriptShowMonPic);
    struct Task * task;
    if (taskId != 0xFF)
    {
        task = &gTasks[taskId];
        switch (task->data[0])
        {
        case 0:
        case 1:
        case 2:
            FreeResourcesAndDestroySprite(&gSprites[task->data[2]], task->data[2]);
            DestroyTask(taskId);
            break;
        case 3:

            DestroyTask(taskId);
            break;
        }
    }
}

static void Task_ScriptShowMonPic(u8 taskId)
{
    struct Task * task = &gTasks[taskId];
    switch (task->data[0])
    {
    case 0:
        task->data[0]++;
        break;
    case 1:
        break;
    case 2:
        FreeResourcesAndDestroySprite(&gSprites[task->data[2]], task->data[2]);
        task->data[0]++;
        break;
    case 3:
        DestroyTask(taskId);
        break;
    }
}

static void ShowPokemonPic(u16 species, u8 x, u8 y)
{
    u8 spriteId;
    u8 taskId;

    spriteId = CreateMonSprite_PicBox(species, 8 * x + 40, 8 * y + 40, FALSE);
    taskId = CreateTask(Task_ScriptShowMonPic, 80);

    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = species;
    gTasks[taskId].data[2] = spriteId;
    gSprites[spriteId].callback = SpriteCallbackDummy;
    gSprites[spriteId].oam.priority = 0;
}

static void AddColorByNature (u8 nature, u8 up1, u8 up2, u8 up3, u8 up4, u8 down1, u8 down2, u8 down3, u8 down4)
{
    if (nature == up1 || nature == up2 || nature == up3 || nature == up4)
        StringCopy(gStringVar3, sRedColor);
    else if (nature == down1 || nature == down2 || nature == down3 || nature == down4)
        StringCopy(gStringVar3, sBlueColor);
    else
        StringCopy(gStringVar3, sBlackColor);
    StringAppend(gStringVar4, gStringVar3);
}

// Converts numbers to text. If the number is smaller than 100 it fills with empty spaces to align the numbers to the right.
static void AddNumber(u16 num)
{
    if (num < 10)
    {
        StringCopy(gStringVar3, sText_TwoEmptySpaces);
        StringAppend(gStringVar4, gStringVar3);
        ConvertIntToDecimalStringN(gStringVar2, num, STR_CONV_MODE_RIGHT_ALIGN, 1);
    }
    else if (num < 100)
    {
        StringCopy(gStringVar3, gText_Space);
        StringAppend(gStringVar4, gStringVar3);
        ConvertIntToDecimalStringN(gStringVar2, num, STR_CONV_MODE_RIGHT_ALIGN, 2);
    }
    else if (num < 1000)
    {
        ConvertIntToDecimalStringN(gStringVar2, num, STR_CONV_MODE_RIGHT_ALIGN, 3);
    }
    else if (num < 10000)
    {
        ConvertIntToDecimalStringN(gStringVar2, num, STR_CONV_MODE_RIGHT_ALIGN, 4);
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar2, num, STR_CONV_MODE_RIGHT_ALIGN, 5);
    }
    StringAppend(gStringVar4, gStringVar2);
}
