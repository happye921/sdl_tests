#include "SDL3/SDL_rect.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <format>
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
  const int ANIM_PLAYER_SLIDE = 2;
  std::vector<Animation> playerAnims;
  std::vector<SDL_Texture *> textures;

  // Player textures
  SDL_Texture *texIdle, *texRun, *texSlide;

  // Tiles textures
  SDL_Texture *texBrick, *texGrass, *texGround, *texPanel;

  SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath)
  {
    SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    textures.push_back(tex);
    return tex;
  }

  void load(EngineState &es)
  {
    playerAnims.resize(5);
    playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
    playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
    playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 5.0f);

    // Load player textures
    texIdle = loadTexture(es.renderer, "resources/idle.png");
    texRun = loadTexture(es.renderer, "resources/run.png");
    texSlide = loadTexture(es.renderer, "resources/slide.png");

    // Load tiles textures
    texBrick = loadTexture(es.renderer, "resources/tiles/brick.png");
    texGrass = loadTexture(es.renderer, "resources/tiles/grass.png");
    texGround = loadTexture(es.renderer, "resources/tiles/ground.png");
    texPanel = loadTexture(es.renderer, "resources/tiles/panel.png");
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
    playerIndex = -1;
  }

  GameObject &player()
  {
    return layers[LAYER_IDX_CHARACTERS][playerIndex];
  }

} typedef GameState;

// Forward decls
void update(const EngineState &es, GameObject &obj, float deltaTime);
void drawObject(const EngineState &es, GameObject &obj, float deltaTime);
void checkCollision(const EngineState &es, GameObject &a, GameObject &b, float deltaTime);
void handleKeyInput(const EngineState &es, GameObject &obj, SDL_Scancode key, bool keyDown);
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
  es->renderer = SDL_CreateRenderer(es->window, "vulkan");
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

  if (event->type == SDL_EVENT_KEY_DOWN)
  {
    handleKeyInput(*es, gameState.player(), event->key.scancode, true);
    return SDL_APP_CONTINUE;
  };

  if (event->type == SDL_EVENT_KEY_UP)
  {
    handleKeyInput(*es, gameState.player(), event->key.scancode, false);
    return SDL_APP_CONTINUE;
  };

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

  // Render Debug info
  SDL_SetRenderDrawColor(es->renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(es->renderer, 5, 5,
                      std::format("State: {} | Grounded: {}", static_cast<int>(gameState.player().data.player.state),
                                  gameState.player().grounded)
                          .c_str());
  SDL_RenderDebugText(
      es->renderer, 5, 15,
      std::format("Position: {} | Velocity: {}",
                  std::format("{:.0f},{:.0f}", gameState.player().position.x, gameState.player().position.y),
                  std::format("{:.0f},{:.0f}", gameState.player().velocity.x, gameState.player().velocity.y))
          .c_str());

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

void update(const EngineState &es, GameObject &obj, float deltaTime)
{
  // Apply gravity
  if (obj.dynamic)
  {
    obj.velocity += glm::vec2(0, 500) * deltaTime;
  }

  if (obj.type == ObjectType::player)
  {
    float currentDirection = 0;
    if (es.keys[SDL_SCANCODE_A])
    {
      currentDirection += -1;
    }
    if (es.keys[SDL_SCANCODE_D])
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
        obj.texture = res.texIdle;
        obj.currentAnimation = res.ANIM_PLAYER_IDLE;
        break;
      }
      case PlayerState::running:
      {
        if (!currentDirection)
        {
          obj.data.player.state = PlayerState::idle;
        }

        // Moving in opposite direction of velocity
        if (obj.velocity.x * obj.direction < 0 && obj.grounded)
        {
          obj.texture = res.texSlide;
          obj.currentAnimation = res.ANIM_PLAYER_SLIDE;
        }
        else
        {
          obj.texture = res.texRun;
          obj.currentAnimation = res.ANIM_PLAYER_RUN;
        }
        break;
      }
      case PlayerState::jumping:
      {
        obj.texture = res.texRun;
        obj.currentAnimation = res.ANIM_PLAYER_RUN;
        break;
      }
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

  // handle collision detection
  bool foundGround = false;
  for (auto &layer : gameState.layers)
  {
    for (GameObject &objB : layer)
    {
      if (&obj != &objB)
      {
        checkCollision(es, obj, objB, deltaTime);

        // Grounded sensor
        SDL_FRect sensor{
            .x = obj.position.x + obj.collider.x,
            .y = obj.position.y + obj.collider.y + obj.collider.h,
            .w = obj.collider.w,
            .h = 1,
        };

        SDL_FRect rectB{
            .x = objB.position.x + objB.collider.x,
            .y = objB.position.y + objB.collider.y,
            .w = objB.collider.w,
            .h = objB.collider.h,
        };

        if (SDL_HasRectIntersectionFloat(&sensor, &rectB))
        {
          foundGround = true;
        }
      }
    }
  }
  if (obj.grounded != foundGround)
  {
    // switching state
    obj.grounded = foundGround;
    if (foundGround && obj.type == ObjectType::player)
    {
      obj.data.player.state = PlayerState::running;
    }
  }
}

void collisionResponse(const EngineState &es, const SDL_FRect &rectA, const SDL_FRect &rectB, const SDL_FRect &rectC,
                       GameObject &objA, GameObject &objB, float deltaTime)
{
  if (objA.type == ObjectType::player)
  {
    switch (objB.type)
    {
      case ObjectType::player:
        break;
      case ObjectType::level:
        if (rectC.w < rectC.h)
        {
          // Horizontal collision
          if (objA.velocity.x > 0) // going right
          {
            objA.position.x -= rectC.w;
          }
          else if (objA.velocity.x < 0) // going left
          {
            objA.position.x += rectC.w;
          }
          objA.velocity.x = 0;
        }
        else
        {
          // Vertical collision
          if (objA.velocity.y > 0) // going down
          {
            objA.position.y -= rectC.h;
          }
          else if (objA.velocity.y < 0) // going up
          {
            objA.position.y += rectC.h;
          }
          objA.velocity.y = 0;
        }
        break;
      case ObjectType::enemy:
        break;
    }
  }
}

void checkCollision(const EngineState &es, GameObject &a, GameObject &b, float deltaTime)
{
  SDL_FRect rectA{
      .x = a.position.x + a.collider.x,
      .y = a.position.y + a.collider.y,
      .w = a.collider.w,
      .h = a.collider.h,
  };

  SDL_FRect rectB{
      .x = b.position.x + b.collider.x,
      .y = b.position.y + b.collider.y,
      .w = b.collider.w,
      .h = b.collider.h,
  };

  SDL_FRect rectC{0, 0, 0, 0};

  if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC))
  {
    // Found intersection
    collisionResponse(es, rectA, rectB, rectC, a, b, deltaTime);
  }
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
      {0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
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
    o.collider = {.x = 0, .y = 0, .w = TILE_SIZE, .h = TILE_SIZE};
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
          player.collider = {.x = 11, .y = 6, .w = 10, .h = 26};

          gameState.layers[LAYER_IDX_CHARACTERS].push_back(player);
          gameState.playerIndex = gameState.layers[LAYER_IDX_CHARACTERS].size() - 1;
          break;
        }

        default:
          break;
      }
    }
  }

  assert(gameState.playerIndex != -1);
}

void handleKeyInput(const EngineState &es, GameObject &obj, SDL_Scancode key, bool keyDown)
{
  const float JUMP_FORCE = -200.0f;

  if (obj.type == ObjectType::player)
  {
    switch (obj.data.player.state)
    {
      case PlayerState::idle:
      {
        if (key == SDL_SCANCODE_K && keyDown)
        {
          obj.data.player.state = PlayerState::jumping;
          obj.velocity.y += JUMP_FORCE;
        }
        break;
      }
      case PlayerState::running:
      {
        if (key == SDL_SCANCODE_K && keyDown)
        {
          obj.data.player.state = PlayerState::jumping;
          obj.velocity.y += JUMP_FORCE;
        }
        break;
      }
      case PlayerState::jumping:
      {
        break;
      }
    }
  }
}
