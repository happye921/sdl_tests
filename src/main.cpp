#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "gameobject.h"

glm::mat2 mat;

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
  const int ANIM_PLAYER_IDLE = 0;
  std::vector<Animation> playerAnims;
  std::vector<SDL_Texture *> textures;
  SDL_Texture *idleTex;

  SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath) {
    SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    textures.push_back(tex);
    return tex;
  }

  void load(EngineState &state) {
    playerAnims.resize(5);
    playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);

    idleTex = loadTexture(state.renderer, "resources/idle.png");
    return;
  }

  void unload() {
    for (SDL_Texture *tex : textures) {
      SDL_DestroyTexture(tex);
      continue;
    }
  }

} typedef GameResources;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
struct GameState {
  float floor;
  std::array<std::vector<GameObject>, 2> layers;
  int playerIndex;

  GameState() {
    playerIndex = 0;
    return;
  }

} typedef GameState;

// Forward decls
void drawObject(EngineState &es, GameObject &obj, float deltaTime);
void cleanupResources();

// Global variables
GameResources res;
GameState gameState;
GameObject player;

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
  es->renderer = SDL_CreateRenderer(es->window, "vulkan");
  if (!es->renderer) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL renderer", nullptr);
    return SDL_APP_FAILURE;
  }

  // Configure presentation
  SDL_SetRenderLogicalPresentation(es->renderer, es->logicalWidth, es->logicalHeight,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // Load game assets
  res.load(*es);

  // Setup keyboard
  es->keys = SDL_GetKeyboardState(nullptr);

  // Player
  player.type = ObjectType::player;
  player.texture = res.idleTex;
  player.animations = res.playerAnims;
  player.currentAnimation = res.ANIM_PLAYER_IDLE;
  gameState.layers[LAYER_IDX_CHARACTERS].push_back(player);

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

  // Update objects
  for (auto &layer : gameState.layers) {
    for (GameObject &obj : layer) {
      if (obj.currentAnimation != -1) {
        obj.animations[obj.currentAnimation].step(deltaTime);
      }
    }
  }

  // Render options
  SDL_SetRenderDrawColor(es->renderer, 20, 10, 30, 255);
  SDL_RenderClear(es->renderer);

  // Draw objects
  for (auto &layer : gameState.layers) {
    for (GameObject &obj : layer) {
      drawObject(*es, obj, deltaTime);
    }
  }

  // Swap buffers and present
  SDL_RenderPresent(es->renderer);

  return SDL_APP_CONTINUE;
}

void cleanupResources() {
  res.unload();
  return;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  cleanupResources();

  if (appstate != NULL) {
    EngineState *es = (EngineState *)appstate;
    SDL_DestroyRenderer(es->renderer);
    SDL_DestroyWindow(es->window);
    SDL_free(es);
  }
}

void drawObject(EngineState &es, GameObject &obj, float deltaTime) {
  float srcX = obj.currentAnimation != 1 ? obj.animations[obj.currentAnimation].currentFrame() * SPRITE_SIZE : 0.0f;

  SDL_FRect src{.x = srcX, .y = 0, .w = SPRITE_SIZE, .h = SPRITE_SIZE};
  SDL_FRect dst{.x = obj.position.x, .y = obj.position.y, .w = SPRITE_SIZE, .h = SPRITE_SIZE};

  SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  SDL_RenderTextureRotated(es.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}
