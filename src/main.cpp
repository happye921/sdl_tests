#include "SDL3/SDL_init.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
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
  // Player animations
  const int ANIM_PLAYER_IDLE = 0;
  const int ANIM_PLAYER_RUN = 1;
  const int ANIM_PLAYER_SLIDE = 2;
  const int ANIM_PLAYER_SHOOT = 3;
  const int ANIM_PLAYER_SLIDE_SHOOT = 4;
  std::vector<Animation> playerAnims;

  // Bullet animations
  const int ANIM_BULLET_MOVING = 0;
  const int ANIM_BULLET_HIT = 1;
  std::vector<Animation> bulletAnims;

  // Enemy animations
  const int ANIM_ENEMY = 0;
  const int ANIM_ENEMY_HIT = 1;
  const int ANIM_ENEMY_DIE = 2;
  std::vector<Animation> enemyAnims;

  std::vector<SDL_Texture *> textures;

  // Player textures
  SDL_Texture *texIdle, *texRun, *texSlide, *texShoot, *texRunShoot, *texSlideShoot;

  // Backgroud textures
  SDL_Texture *texBg1, *texBg2, *texBg3, *texBg4;

  // Tiles textures
  SDL_Texture *texBrick, *texGrass, *texGround, *texPanel;

  // Bullet tiles
  SDL_Texture *texBullet, *texBulletHit;

  // Enemy textures
  SDL_Texture *texEnemy, *texEnemyHit, *texEnemyDie;

  SDL_Texture *loadTexture(SDL_Renderer *renderer, const std::string &filepath)
  {
    SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    textures.push_back(tex);
    return tex;
  }

  void load(EngineState &es)
  {
    // Load player animations
    playerAnims.resize(5);
    playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
    playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
    playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 5.0f);
    playerAnims[ANIM_PLAYER_SHOOT] = Animation(4, 0.5f);
    playerAnims[ANIM_PLAYER_SLIDE_SHOOT] = Animation(4, 0.5f);

    // Load bullet animations
    bulletAnims.resize(2);
    bulletAnims[ANIM_BULLET_MOVING] = Animation(4, 0.05f);
    bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f);

    // Load enemy animations
    enemyAnims.resize(3);
    enemyAnims[ANIM_ENEMY] = Animation(8, 1.0f);
    enemyAnims[ANIM_ENEMY_HIT] = Animation(8, 1.0f);
    enemyAnims[ANIM_ENEMY_DIE] = Animation(18, 2.0f);

    // Load player textures
    texIdle = loadTexture(es.renderer, "resources/idle.png");
    texRun = loadTexture(es.renderer, "resources/run.png");
    texSlide = loadTexture(es.renderer, "resources/slide.png");
    texShoot = loadTexture(es.renderer, "resources/shoot.png");
    texRunShoot = loadTexture(es.renderer, "resources/shoot_run.png");
    texSlideShoot = loadTexture(es.renderer, "resources/slide_shoot.png");

    // Load tiles textures
    texBrick = loadTexture(es.renderer, "resources/tiles/brick.png");
    texGrass = loadTexture(es.renderer, "resources/tiles/grass.png");
    texGround = loadTexture(es.renderer, "resources/tiles/ground.png");
    texPanel = loadTexture(es.renderer, "resources/tiles/panel.png");

    // Load background textures
    texBg1 = loadTexture(es.renderer, "resources/bg/bg_layer1.png");
    texBg2 = loadTexture(es.renderer, "resources/bg/bg_layer2.png");
    texBg3 = loadTexture(es.renderer, "resources/bg/bg_layer3.png");
    texBg4 = loadTexture(es.renderer, "resources/bg/bg_layer4.png");

    // Load bullet textures
    texBullet = loadTexture(es.renderer, "resources/bullet.png");
    texBulletHit = loadTexture(es.renderer, "resources/bullet_hit.png");

    // Load enemy textures
    texEnemy = loadTexture(es.renderer, "resources/enemy.png");
    texEnemyHit = loadTexture(es.renderer, "resources/enemy_hit.png");
    texEnemyDie = loadTexture(es.renderer, "resources/enemy_die.png");
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
  std::array<std::vector<GameObject>, 2> layers;
  std::vector<GameObject> backgroundTiles;
  std::vector<GameObject> foregroundTiles;
  std::vector<GameObject> bullets;
  int playerIndex;
  SDL_FRect mapViewport;
  float bg2scroll, bg3scroll, bg4scroll;
  bool debugMode;

  GameState()
  {
    playerIndex = -1;
    debugMode = false;
  }

  void SetupViewport(const EngineState &es)
  {
    mapViewport = {
        .x = 0,
        .y = 0,
        .w = static_cast<float>(es.logicalWidth),
        .h = static_cast<float>(es.logicalHeight),
    };

    bg2scroll = bg3scroll = bg4scroll = 0;
  }

  GameObject &player()
  {
    return layers[LAYER_IDX_CHARACTERS][playerIndex];
  }

} typedef GameState;

// Forward decls
void update(const EngineState &es, GameObject &obj, float deltaTime);
void drawObject(const EngineState &es, GameObject &obj, float width, float height, float deltaTime);
void checkCollision(const EngineState &es, GameObject &a, GameObject &b, float deltaTime);
void handleKeyInput(const EngineState &es, GameObject &obj, SDL_Scancode key, bool keyDown);
void cleanupResources();
void createTiles(const EngineState &es);
void drawParalaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime);

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
  SDL_SetRenderLogicalPresentation(es->renderer, es->logicalWidth, es->logicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  // Setup viewport
  gameState.SetupViewport(*es);

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
    if (event->key.scancode == SDL_SCANCODE_Q)
    {
      return SDL_APP_SUCCESS;
    }

    handleKeyInput(*es, gameState.player(), event->key.scancode, true);
    return SDL_APP_CONTINUE;
  };

  if (event->type == SDL_EVENT_KEY_UP)
  {
    handleKeyInput(*es, gameState.player(), event->key.scancode, false);
    if (event->key.scancode == SDL_SCANCODE_F12)
    {
      gameState.debugMode = !gameState.debugMode;
    }
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
    }
  }

  // Update Bullets
  for (GameObject &bullet : gameState.bullets)
  {
    update(*es, bullet, deltaTime);
    if (bullet.currentAnimation != -1)
    {
      bullet.animations[bullet.currentAnimation].step(deltaTime);
    }
  }

  gameState.mapViewport.x = (gameState.player().position.x + TILE_SIZE / 2) - gameState.mapViewport.w / 2;

  // Render options
  SDL_SetRenderDrawColor(es->renderer, 20, 10, 30, 255);
  SDL_RenderClear(es->renderer);

  // Draw background images
  SDL_RenderTexture(es->renderer, res.texBg1, nullptr, nullptr);
  drawParalaxBackground(es->renderer, res.texBg4, gameState.player().velocity.x, gameState.bg4scroll, 0.075f, deltaTime);
  drawParalaxBackground(es->renderer, res.texBg3, gameState.player().velocity.x, gameState.bg3scroll, 0.15f, deltaTime);
  drawParalaxBackground(es->renderer, res.texBg2, gameState.player().velocity.x, gameState.bg2scroll, 0.3f, deltaTime);

  // Draw background tiles
  for (GameObject &obj : gameState.backgroundTiles)
  {
    SDL_FRect dst{
        .x = obj.position.x - gameState.mapViewport.x,
        .y = obj.position.y,
        .w = static_cast<float>(obj.texture->w),
        .h = static_cast<float>(obj.texture->h),
    };

    SDL_RenderTexture(es->renderer, obj.texture, nullptr, &dst);
  }

  // Draw objects
  for (auto &layer : gameState.layers)
  {
    for (GameObject &obj : layer)
    {
      drawObject(*es, obj, TILE_SIZE, TILE_SIZE, deltaTime);
    }
  }

  // Draw bullets
  for (GameObject &bullet : gameState.bullets)
  {
    if (bullet.data.bullet.state == BulletState::inactive)
    {
      continue;
    }

    drawObject(*es, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
  }

  // Draw foreground tiles
  for (GameObject &obj : gameState.foregroundTiles)
  {
    SDL_FRect dst{
        .x = obj.position.x - gameState.mapViewport.x,
        .y = obj.position.y,
        .w = static_cast<float>(obj.texture->w),
        .h = static_cast<float>(obj.texture->h),
    };

    SDL_RenderTexture(es->renderer, obj.texture, nullptr, &dst);
  }

  // Render Debug info
  if (gameState.debugMode)
  {
    SDL_SetRenderDrawColor(es->renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(es->renderer, 5, 5,
                        std::format("State: {} | Grounded: {} | Bullets: {}", static_cast<int>(gameState.player().data.player.state),
                                    gameState.player().grounded, gameState.bullets.size())
                            .c_str());
    SDL_RenderDebugText(es->renderer, 5, 15,
                        std::format("Position: {} | Velocity: {}",
                                    std::format("{:.0f},{:.0f}", gameState.player().position.x, gameState.player().position.y),
                                    std::format("{:.0f},{:.0f}", gameState.player().velocity.x, gameState.player().velocity.y))
                            .c_str());
  }

  // Swap buffers and present
  SDL_RenderPresent(es->renderer);

  // SDL_Delay(100);

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

void drawObject(const EngineState &es, GameObject &obj, float width, float height, float deltaTime)
{
  float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * width : (obj.spriteFrame - 1) * width;

  SDL_FRect src{
      .x = srcX,
      .y = 0,
      .w = width,
      .h = height,
  };

  SDL_FRect dst{
      .x = obj.position.x - gameState.mapViewport.x,
      .y = obj.position.y,
      .w = width,
      .h = height,
  };

  SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  if (!obj.shouldFlash)
  {
    SDL_RenderTextureRotated(es.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
  }
  else
  {
    SDL_SetTextureColorModFloat(obj.texture, 2.5f, 1.0f, 1.0f);
    SDL_RenderTextureRotated(es.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
    SDL_SetTextureColorModFloat(obj.texture, 1.0f, 1.0f, 1.0f);

    if (obj.flashTimer.step(deltaTime))
    {
      obj.shouldFlash = false;
    }
  }

  if (gameState.debugMode)
  {
    SDL_FRect debugColliderRect{
        .x = obj.position.x + obj.collider.x - gameState.mapViewport.x,
        .y = obj.position.y + obj.collider.y,
        .w = obj.collider.w,
        .h = obj.collider.h,
    };

    SDL_SetRenderDrawBlendMode(es.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(es.renderer, 255, 0, 0, 150);
    SDL_RenderFillRect(es.renderer, &debugColliderRect);
    SDL_SetRenderDrawBlendMode(es.renderer, SDL_BLENDMODE_NONE);

    SDL_FRect debugColliderSensorRect{
        .x = obj.position.x + obj.collider.x - gameState.mapViewport.x,
        .y = obj.position.y + obj.collider.y + obj.collider.h,
        .w = obj.collider.w,
        .h = 1,
    };

    SDL_SetRenderDrawColor(es.renderer, 0, 0, 255, 255);
    SDL_RenderFillRect(es.renderer, &debugColliderSensorRect);
  }
}

void update(const EngineState &es, GameObject &obj, float deltaTime)
{
  // Update animation
  if (obj.currentAnimation != -1)
  {
    obj.animations[obj.currentAnimation].step(deltaTime);
  }

  // Apply gravity
  if (obj.dynamic)
  {
    obj.velocity += glm::vec2(0, 500) * deltaTime;
  }

  float currentDirection = 0;

  if (obj.type == ObjectType::player)
  {
    if (es.keys[SDL_SCANCODE_A])
    {
      currentDirection += -1;
    }
    if (es.keys[SDL_SCANCODE_D])
    {
      currentDirection += 1;
    }

    Timer &weaponTimer = obj.data.player.weaponTimer;
    weaponTimer.step(deltaTime);

    const auto handleShooting = [&es, &obj, &weaponTimer](SDL_Texture *tex, SDL_Texture *shootTex, int animIndex, int shootAnimIndex)
    {
      if (es.keys[SDL_SCANCODE_J])
      {
        // set shooting tex/anim
        obj.texture = shootTex;
        obj.currentAnimation = shootAnimIndex;
        if (weaponTimer.isTimeout())
        {
          weaponTimer.reset();
          GameObject bullet;
          bullet.data.bullet = BulletData();
          bullet.type = ObjectType::bullet;
          bullet.direction = gameState.player().direction;
          bullet.texture = res.texBullet;
          bullet.currentAnimation = res.ANIM_BULLET_MOVING;
          bullet.collider = SDL_FRect{
              .x = 0,
              .y = 0,
              .w = static_cast<float>(res.texBullet->h),
              .h = static_cast<float>(res.texBullet->h),
          };
          const int yVariation = 40;
          const float yVelocity = SDL_rand(yVariation) - yVariation / 2.0f;
          bullet.velocity = glm::vec2(obj.velocity.x + 600.0f, yVelocity) * bullet.direction;
          bullet.animations = res.bulletAnims;

          const float left = 4.0f;
          const float right = 24.0f;
          const float t = (obj.direction + 1) / 2.0f;
          const float xOffset = left + right * t;
          bullet.position = glm::vec2(obj.position.x + xOffset, obj.position.y + TILE_SIZE / 2 + 1);
          bullet.maxSpeedX = 1000.0f;

          // Look for inactive bullets before spawning a new one
          bool foundInactive = false;
          for (size_t i = 0; i < gameState.bullets.size() && !foundInactive; i++)
          {
            if (gameState.bullets[i].data.bullet.state == BulletState::inactive)
            {
              foundInactive = true;
              gameState.bullets[i] = bullet;
            }
          }

          if (!foundInactive)
          {
            gameState.bullets.push_back(bullet);
          }
        }
      }
      else
      {
        obj.texture = tex;
        obj.currentAnimation = animIndex;
      }
    };

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

        handleShooting(res.texIdle, res.texShoot, res.ANIM_PLAYER_IDLE, res.ANIM_PLAYER_SHOOT);
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
          handleShooting(res.texSlide, res.texSlideShoot, res.ANIM_PLAYER_SLIDE, res.ANIM_PLAYER_SLIDE_SHOOT);
        }
        else
        {
          handleShooting(res.texRun, res.texRunShoot, res.ANIM_PLAYER_RUN, res.ANIM_PLAYER_RUN);
        }
        break;
      }
      case PlayerState::jumping:
      {
        handleShooting(res.texRun, res.texRunShoot, res.ANIM_PLAYER_RUN, res.ANIM_PLAYER_RUN);
        obj.texture = res.texRun;
        obj.currentAnimation = res.ANIM_PLAYER_RUN;
        break;
      }
    }
  }
  else if (obj.type == ObjectType::bullet)
  {
    switch (obj.data.bullet.state)
    {
      case BulletState::moving:
      {
        if (obj.position.x - gameState.mapViewport.x < 0 || obj.position.x - gameState.mapViewport.x > es.logicalWidth ||
            obj.position.y - gameState.mapViewport.y < 0 || obj.position.y - gameState.mapViewport.y > es.logicalHeight)
        {
          obj.data.bullet.state = BulletState::inactive;
        }
        break;
      }
      case BulletState::colliding:
        if (obj.animations[obj.currentAnimation].isDone())
        {
          obj.data.bullet.state = BulletState::inactive;
        }
        break;
      case BulletState::inactive:
        break;
    }
  }
  else if (obj.type == ObjectType::enemy)
  {
    switch (obj.data.enemy.state)
    {
      case EnemyState::idle:
        break;
      case EnemyState::damaged:
      {
        if (obj.data.enemy.damagedTimer.step(deltaTime))
        {
          obj.data.enemy.state = EnemyState::idle;
          obj.texture = res.texEnemy;
          obj.currentAnimation = res.ANIM_ENEMY;
        }
        break;
      }
      case EnemyState::dead:
      {
        if (obj.currentAnimation != -1 && obj.animations[obj.currentAnimation].isDone())
        {
          // Remove animation and set to last frame
          obj.currentAnimation = -1;
          obj.spriteFrame = 18;
        }
        break;
      }
    }
  }

  if (currentDirection)
  {
    obj.direction = currentDirection;
  }

  // Add acceleration to velocity
  obj.velocity += currentDirection * obj.acceleration * deltaTime;
  if (std::abs(obj.velocity.x) > obj.maxSpeedX)
  {
    obj.velocity.x = obj.maxSpeedX * currentDirection;
  }

  // Add velocity to position
  obj.position += obj.velocity * deltaTime;

  // handle collision detection
  bool foundGround = false;
  for (auto &layer : gameState.layers)
  {
    for (GameObject &objB : layer)
    {
      if (&obj == &objB)
      {
        continue;
      }

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

      SDL_FRect rectC{0, 0, 0, 0};
      if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC))
      {
        foundGround = true;
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

void collisionResponse(const EngineState &es, const SDL_FRect &rectA, const SDL_FRect &rectB, const SDL_FRect &rectC, GameObject &objA,
                       GameObject &objB, float deltaTime)
{
  const auto genericResponse = [&]()
  {
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
  };

  if (objA.type == ObjectType::player)
  {
    switch (objB.type)
    {
      case ObjectType::player:
        break;
      case ObjectType::level:
        genericResponse();
        break;
      case ObjectType::enemy:
        break;
      case ObjectType::bullet:
        break;
    }
  }
  else if (objA.type == ObjectType::bullet)
  {
    bool passthrough = false;
    switch (objA.data.bullet.state)
    {
      case BulletState::moving:
      {
        switch (objB.type)
        {
          case ObjectType::player:
            break;
          case ObjectType::level:
            break;
          case ObjectType::enemy:
          {
            EnemyData &d = objB.data.enemy;
            if (d.state != EnemyState::dead)
            {
              objB.direction = -objA.direction;
              objB.shouldFlash = true;
              objB.flashTimer.reset();
              objB.texture = res.texEnemyHit;
              objB.currentAnimation = res.ANIM_ENEMY_HIT;
              d.state = EnemyState::damaged;
              d.healthPoints -= 10;
              if (d.healthPoints <= 0)
              {
                d.state = EnemyState::dead;
                objB.texture = res.texEnemyDie;
                objB.currentAnimation = res.ANIM_ENEMY_DIE;
              }
            }
            else
            {
              passthrough = true;
            }
            break;
          }
          case ObjectType::bullet:
            break;
        }

        if (!passthrough)
        {
          genericResponse();
          objA.velocity *= 0;
          objA.data.bullet.state = BulletState::colliding;
          objA.texture = res.texBulletHit;
          objA.currentAnimation = res.ANIM_BULLET_HIT;
        }
        break;
      }
      case BulletState::colliding:
        break;
      case BulletState::inactive:
        break;
    }
  }
  else if (objA.type == ObjectType::enemy)
  {
    genericResponse();
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
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {4, 0, 0, 3, 0, 0, 3, 0, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };

  short foreground[MAP_ROWS][MAP_COLS] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {5, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };

  short background[MAP_ROWS][MAP_COLS] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 0, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };
  // clang-format on

  const auto loadMap = [&es](short layer[MAP_ROWS][MAP_COLS])
  {
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
        switch (layer[r][c])
        {
          case 1: // ground
          {
            GameObject ground = createObject(r, c, res.texGround, ObjectType::level);
            gameState.layers[LAYER_IDX_LEVEL].push_back(ground);
            break;
          }

          case 2: // back panel
          {
            GameObject panel = createObject(r, c, res.texPanel, ObjectType::level);
            gameState.layers[LAYER_IDX_LEVEL].push_back(panel);
            break;
          }

          case 3:
          {
            GameObject enemy = createObject(r, c, res.texEnemy, ObjectType::enemy);
            enemy.data.enemy = EnemyData();
            enemy.currentAnimation = res.ANIM_ENEMY;
            enemy.animations = res.enemyAnims;
            enemy.collider = SDL_FRect{
                .x = 10,
                .y = 4,
                .w = 12,
                .h = 28,
            };
            enemy.dynamic = true;
            gameState.layers[LAYER_IDX_CHARACTERS].push_back(enemy);
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

          case 5: // Grass
          {
            GameObject grass = createObject(r, c, res.texGrass, ObjectType::level);
            gameState.foregroundTiles.push_back(grass);
            break;
          }

          case 6: // Brick
          {
            GameObject brick = createObject(r, c, res.texBrick, ObjectType::level);
            gameState.backgroundTiles.push_back(brick);
            break;
          }

          default:
            break;
        }
      }
    }
  };

  loadMap(map);
  loadMap(background);
  loadMap(foreground);

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

void drawParalaxBackground(SDL_Renderer *renderer, SDL_Texture *texture, float xVelocity, float &scrollPos, float scrollFactor, float deltaTime)
{
  scrollPos -= xVelocity * scrollFactor * deltaTime;
  if (scrollPos <= -texture->w)
  {
    scrollPos = 0;
  }

  SDL_FRect dst{
      .x = scrollPos,
      .y = 30,
      .w = texture->w * 2.0f,
      .h = static_cast<float>(texture->h),
  };

  SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}
