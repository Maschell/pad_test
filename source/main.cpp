#include "SDL.h"
#include "SDL2_gfxPrimitives.h"
#include "SDL_FontCache.h"
#include "pad.h"

#include <coreinit/memory.h>
#include <nn/hpad.h>

SDL_Window *window     = NULL;
SDL_Renderer *renderer = NULL;
FC_Font *font          = NULL;

int initGraphics(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to init SDL video\n");
        return -1;
    }

    window = SDL_CreateWindow("hpad_test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
    if (!window) {
        fprintf(stderr, "Failed to create SDL window\n");
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Failed to create SDL renderer\n");
        return -1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Failed to init SDL TTF\n");
        return -1;
    }

    font = FC_CreateFont();
    if (!font) {
        fprintf(stderr, "Failed to create font\n");
        return -1;
    }

#ifdef __WIIU__
    void *fontData;
    uint32_t fontSize;
    if (!OSGetSharedData(OS_SHAREDDATATYPE_FONT_STANDARD, 0, &fontData, &fontSize)) {
        return -1;
    }

    if (!FC_LoadFont_RW(font, renderer, SDL_RWFromMem(fontData, fontSize), 1, 48, (SDL_Color){255, 255, 255, 255}, TTF_STYLE_NORMAL)) {
        return -1;
    }
#else
    if (!FC_LoadFont(font, renderer, "/usr/share/fonts/noto/NotoSans-Regular.ttf", 48, (SDL_Color){255, 255, 255, 255}, TTF_STYLE_NORMAL)) {
        fprintf(stderr, "Failed to load font\n");
        return -1;
    }
#endif

    return 0;
}

void deinitGraphics() {
    FC_FreeFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void drawText(float posX, float posY, float size, const char *fmt, ...) {
    va_list lst;
    va_start(lst, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, lst);
    va_end(lst);

    FC_Effect effect;
    effect.alignment = FC_ALIGN_LEFT;
    effect.color.r   = 255;
    effect.color.g   = 255;
    effect.color.b   = 255;
    effect.color.a   = 255;
    effect.scale.x   = size / 48.0f;
    effect.scale.y   = size / 48.0f;

    FC_DrawEffect(font, renderer, posX, posY, effect, buffer);
}

// All of these pos offsets are hardcoded lol
// Don't use this as a reference for clean code
void drawStick(float posX, float posY, const char *name, float valX, float valY) {
    drawText(posX, posY, 20.0f, name);
    posY += 40.0f;

    circleRGBA(renderer, posX + 40, posY + 40, 40, 128, 128, 128, 255);
    filledCircleRGBA(renderer, posX + 40 + valX * 20.0f, posY + 40 + valY * -20.0f, 30, 255, 255, 255, 255);

    drawText(posX, posY + 100.0f, 20.0f, "X: %8.2f\nY: %8.2f", valX, valY);
}

void drawTrigger(float posX, float posY, const char *name, float val) {
    drawText(posX, posY, 20.0f, name);
    posY += 40.0f;

    rectangleRGBA(renderer, posX, posY, posX + 50.0f, posY + 100.0f, 128, 128, 128, 255);
    boxRGBA(renderer, posX, posY + 100.0f, posX + 50.0f, posY + 100.0f - val * 100.f, 255, 255, 255, 255);

    drawText(posX, posY + 120.0f, 20.0f, "%.2f", val);
}

void drawController(float posX, float posY, int index, PADStatus *status) {
    drawText(posX, posY, 30.0f, "HPAD %d", index);
    posX += 150.0f;

    if (status->err == PAD_ERR_NO_CONTROLLER) {
        drawText(posX, posY, 24.0f, "Not connected");
        return;
    }
    if (status->err != PAD_ERR_NONE) {
        drawText(posX, posY, 24.0f, "ERROR");
        return;
    }

#define PAD_STICK_AXIS_MAX    127
#define PAD_SUBSTICK_AXIS_MAX 127
#define PAD_TRIGGER_MAX       256

    drawStick(posX, posY, "Stick", status->stickX / (float) PAD_STICK_AXIS_MAX, status->stickY / (float) PAD_STICK_AXIS_MAX);
    posX += 150.0f;
    drawStick(posX, posY, "C-Stick", status->substickX / (float) PAD_SUBSTICK_AXIS_MAX, status->substickY / (float) PAD_SUBSTICK_AXIS_MAX);
    posX += 150.0f;

    drawTrigger(posX, posY, "Left Trigger", status->triggerL / (float) PAD_TRIGGER_MAX);
    posX += 150.0f;
    drawTrigger(posX, posY, "Right Trigger", status->triggerR / (float) PAD_TRIGGER_MAX);
    posX += 150.0f;

    drawText(posX, posY, 20.0f, "Hold: 0x%05x\n\n\nStatus: 0x%x", status->button, status->err);
}

int main(int argc, char const *argv[]) {
    if (initGraphics() != 0) {
        return 1;
    }

    PadInit();
    PADStatusWrapper status = {};

    int running = 1;
    SDL_Event e;
    while (running != 0) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        drawText(32.0f, 16.0f, 48.0f, "PAD Test by Maschell (based on HPAD Test by GaryOderNichts)");

        PADRead(&status);

        float posX = 32.0f, posY = 200.0f;
        for (int i = 0; i < 4; i++) {
            drawController(posX, posY, i, &status.data[i]);
            posY += 200.0f;
        }

        SDL_RenderPresent(renderer);
    }

    HPADShutdown();
    deinitGraphics();

    return 0;
}
