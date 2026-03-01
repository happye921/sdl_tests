#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "gameobject.h"

const float SPRITE_SIZE = 32.0f;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct EngineState
{
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

struct GameResources
{
  const int ANIM_PLAYER_IDLE = 0;
  const int ANIM_PLAYER_RUN = 1;
  std::vector<Animation> playerAnims;
  std::vector<SDL_Texture *> textures;

  // Player textures
  SDL_Texture *texIdle, *texRun;

  // Tiles textures
  SDL_Texture *texBrick, *texGrass, *texGround, *texPanel;

  SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath)
  {
    SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    textures.push_back(tex);
    return tex;
  }

  void load(EngineState &state)
  {
    playerAnims.resize(5);
    playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
    playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);

    // Load player textures
    texIdle = loadTexture(state.renderer, "resources/idle.png");
    texRun = loadTexture(state.renderer, "resources/run.png");

    // Load tiles textures
    texBrick = loadTexture(state.renderer, "resources/tiles/brick.png");
    texGrass = loadTexture(state.renderer, "resources/tiles/grass.png");
    texGround = loadTexture(state.renderer, "resources/tiles/ground.png");
    texPanel = loadTexture(state.renderer, "resources/tiles/panel.png");
  }

  void unload()
  {
    for (SDL_Texture *tex : textures)
    {
      SDL_DestroyTexture(tex);
      continue;
    }
  }

} typedef GameResources;

struct GameState
{
  float floor;
  std::array<std::vector<GameObject>, 2> layers;
  int playerIndex;

  GameState()
  {
    playerIndex = 0;
  }

} typedef GameState;

// Forward decls
void update(const EngineState &state, GameObject &obj, float deltaTime);
void drawObject(const EngineState &es, GameObject &obj, float deltaTime);
void cleanupResources();
void createTiles(const EngineState &es);

// Global variables
GameResources res;
GameState gameState;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
  }

  EngineState *es = (EngineState *)SDL_calloc(1, sizeof(EngineState));
  if (!es)
  {
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
  if (!es->window)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL window", nullptr);
    return SDL_APP_FAILURE;
  }

  // Create renderer
  es->renderer = SDL_CreateRenderer(es->window, "software");
  if (!es->renderer)
  {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL renderer", nullptr);
    return SDL_APP_FAILURE;
  }

  SDL_SetRenderVSync(es->renderer, 1);

  // Configure presentation
  SDL_SetRenderLogicalPresentation(es->renderer, es->logicalWidth, es->logicalHeight,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // Load game assets
  res.load(*es);

  // Initialize tiles
  createTiles(*es);

  // Setup keyboard
  es->keys = SDL_GetKeyboardState(nullptr);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  EngineState *es = (EngineState *)appstate;
  // close the window on request
  if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
  {
    return SDL_APP_SUCCESS;
  }

  if (event->type == SDL_EVENT_WINDOW_RESIZED)
  {
    es->winWidth = event->window.data1;
    es->winHeight = event->window.data2;
    return SDL_APP_CONTINUE;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  EngineState *es = (EngineState *)appstate;
  Uint64 nowTime = SDL_GetTicks();
  float deltaTime = (nowTime - es->lastTime) / 1000.0f;
  es->deltaTime = deltaTime;
  es->lastTime = nowTime;

  // Update objects
  for (auto &layer : gameState.layers)
  {
    for (GameObject &obj : layer)
    {
      update(*es, obj, deltaTime);
      if (obj.currentAnimation != -1)
      {
        obj.animations[obj.currentAnimation].step(deltaTime);
      }
    }
  }

  // Render options
  SDL_SetRenderDrawColor(es->renderer, 20, 10, 30, 255);
  SDL_RenderClear(es->renderer);

  // Draw objects
  for (auto &layer : gameState.layers)
  {
    for (GameObject &obj : layer)
    {
      drawObject(*es, obj, deltaTime);
    }
  }

  // Swap buffers and present
  SDL_RenderPresent(es->renderer);

  return SDL_APP_CONTINUE;
}

void cleanupResources()
{
  res.unload();
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  cleanupResources();

  if (appstate != NULL)
  {
    EngineState *es = (EngineState *)appstate;
    SDL_DestroyRenderer(es->renderer);
    SDL_DestroyWindow(es->window);
    SDL_free(es);
  }
}

void drawObject(const EngineState &es, GameObject &obj, float deltaTime)
{
  float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * SPRITE_SIZE : 0.0f;

  SDL_FRect src{.x = srcX, .y = 0, .w = SPRITE_SIZE, .h = SPRITE_SIZE};
  SDL_FRect dst{.x = obj.position.x, .y = obj.position.y, .w = SPRITE_SIZE, .h = SPRITE_SIZE};

  SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  SDL_RenderTextureRotated(es.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}

void update(const EngineState &state, GameObject &obj, float deltaTime)
{
  // Apply gravity
  if (obj.dynamic)
  {
    obj.velocity += glm::vec2(0, 500) * deltaTime;
  }

  if (obj.type == ObjectType::player)
  {
    float currentDirection = 0;
    if (state.keys[SDL_SCANCODE_A])
    {
      currentDirection += -1;
    }
    if (state.keys[SDL_SCANCODE_D])
    {
      currentDirection += 1;
    }

    if (currentDirection)
    {
      obj.direction = currentDirection;
    }

    switch (obj.data.player.state)
    {
      case PlayerState::idle:
      {
        if (currentDirection)
        {
          obj.data.player.state = PlayerState::running;
          obj.texture = res.texRun;
          obj.currentAnimation = res.ANIM_PLAYER_RUN;
        }
        else
        {
          if (obj.velocity.x)
          {
            const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
            float amount = factor * obj.acceleration.x * deltaTime;
            if (std::abs(obj.velocity.x) < std::abs(amount))
            {
              obj.velocity.x = 0;
            }
            else
            {
              obj.velocity.x += amount;
            }
          }
        }
        break;
      }
      case PlayerState::running:
      {
        if (!currentDirection)
        {
          obj.data.player.state = PlayerState::idle;
          obj.texture = res.texIdle;
          obj.currentAnimation = res.ANIM_PLAYER_IDLE;
        }
        break;
      }
      case PlayerState::jumping:
        break;
    }

    // Add acceleration to velocity
    obj.velocity += currentDirection * obj.acceleration * deltaTime;
    if (std::abs(obj.velocity.x) > obj.maxSpeedX)
    {
      obj.velocity.x = obj.maxSpeedX * currentDirection;
    }
  }

  // Add velocity to position
  obj.position += obj.velocity * deltaTime;
}

void createTiles(const EngineState &es)
{
  // clang-format off
  /* 
   1 - Ground
   2 - Panel
   3 - Enemy
   4 - Player
   5 - Grass
   6 - Brick
  */
  short map[MAP_ROWS][MAP_COLS] = {
      {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };
  // clang-format on

  const auto createObject = [&es](int r, int c, SDL_Texture *tex, ObjectType type)
  {
    GameObject o;
    float posX = c * TILE_SIZE;
    float posY = es.logicalHeight - (MAP_ROWS - r) * TILE_SIZE;
    o.type = type;
    o.position = glm::vec2(posX, posY);
    o.texture = tex;
    return o;
  };

  for (int r = 0; r < MAP_ROWS; r++)
  {
    for (int c = 0; c < MAP_COLS; c++)
    {
      switch (map[r][c])
      {
        case 1:
        {
          GameObject ground = createObject(r, c, res.texGround, ObjectType::level);
          gameState.layers[LAYER_IDX_LEVEL].push_back(ground);
          break;
        }
        case 2:
        {
          GameObject panel = createObject(r, c, res.texPanel, ObjectType::level);
          gameState.layers[LAYER_IDX_LEVEL].push_back(panel);
          break;
        }

        case 4: // player
        {
          GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
          player.data.player = PlayerData();
          player.animations = res.playerAnims;
          player.currentAnimation = res.ANIM_PLAYER_IDLE;
          player.acceleration = glm::vec2(300, 0);
          player.maxSpeedX = 100;
          player.dynamic = true;
          gameState.layers[LAYER_IDX_CHARACTERS].push_back(player);
          break;
        }
      }
    }
  }
}
