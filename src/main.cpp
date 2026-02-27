#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

const float SPRITE_SIZE = 32.0f;

struct EngineState {
  SDL_Window *window;
  SDL_Renderer *renderer;
  int winWidth;
  int winHeight;
  int logicalWidth;
  int logicalHeight;
  Uint64 lastTime;
  float deltaTime;
  const bool *keys;
} typedef EngineState;

struct GameResources {
  SDL_Texture *idleTex;
} typedef GameResources;

struct Player {
  float x;
  bool flipH;
} typedef Player;

struct Game {
  float floor;
} typedef Game;

// Global variables
GameResources gr;
Player player;
Game game;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
  }

  EngineState *es = (EngineState *)SDL_calloc(1, sizeof(EngineState));
  if (!es) {
    return SDL_APP_FAILURE;
  }

  *appstate = es;
  es->winWidth = 1600;
  es->winHeight = 900;
  es->logicalWidth = 640;
  es->logicalHeight = 320;
  es->lastTime = SDL_GetTicks();

  // Create window
  es->window = SDL_CreateWindow("Game", es->winWidth, es->winHeight, SDL_WINDOW_RESIZABLE);
  if (!es->window) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL window", nullptr);
    return SDL_APP_FAILURE;
  }

  // Create renderer
  es->renderer = SDL_CreateRenderer(es->window, "software");
  if (!es->renderer) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL renderer", nullptr);
    return SDL_APP_FAILURE;
  }

  // Configure presentation
  SDL_SetRenderLogicalPresentation(es->renderer, es->logicalWidth, es->logicalHeight,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // Load game assets
  gr.idleTex = IMG_LoadTexture(es->renderer, "resources/idle.png");
  SDL_SetTextureScaleMode(gr.idleTex, SDL_SCALEMODE_NEAREST);

  // Setup keyboard
  es->keys = SDL_GetKeyboardState(nullptr);

  // Setup player
  player.x = 150;
  player.flipH = false;

  // Setup game
  game.floor = es->logicalHeight;

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  EngineState *es = (EngineState *)appstate;
  // close the window on request
  if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
    return SDL_APP_SUCCESS;
  }

  if (event->type == SDL_EVENT_WINDOW_RESIZED) {
    es->winWidth = event->window.data1;
    es->winHeight = event->window.data2;
    return SDL_APP_CONTINUE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

  EngineState *es = (EngineState *)appstate;
  Uint64 nowTime = SDL_GetTicks();
  float deltaTime = (nowTime - es->lastTime) / 1000.0f;
  es->deltaTime = deltaTime;
  es->lastTime = nowTime;

  // Handle movement
  float moveAmount = 0;
  if (es->keys[SDL_SCANCODE_A]) {
    moveAmount += -75.0f;
    player.flipH = true;
  }
  if (es->keys[SDL_SCANCODE_D]) {
    moveAmount += 75.0f;
    player.flipH = false;
  }

  player.x += moveAmount * deltaTime;

  // Draw
  SDL_SetRenderDrawColor(es->renderer, 20, 10, 30, 255);
  SDL_RenderClear(es->renderer);

  SDL_FRect src{.x = 0, .y = 0, .w = SPRITE_SIZE, .h = SPRITE_SIZE};
  SDL_FRect dst{.x = player.x, .y = game.floor - SPRITE_SIZE, .w = SPRITE_SIZE, .h = SPRITE_SIZE};

  SDL_RenderTextureRotated(es->renderer, gr.idleTex, &src, &dst, 0, nullptr,
                           (player.flipH) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

  // Swap buffers and present
  SDL_RenderPresent(es->renderer);

  return SDL_APP_CONTINUE;
}

void CleanupResources() {
  SDL_DestroyTexture(gr.idleTex);
  return;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  CleanupResources();

  if (appstate != NULL) {
    EngineState *es = (EngineState *)appstate;
    SDL_DestroyRenderer(es->renderer);
    SDL_DestroyWindow(es->window);
    SDL_free(es);
  }
}
