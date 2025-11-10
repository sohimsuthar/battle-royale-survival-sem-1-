#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_ENEMIES 50
#define MAX_BULLETS 200
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800
#define WORLD_WIDTH 3000
#define WORLD_HEIGHT 3000
#define PLAYER_SPEED 300.0f
#define BULLET_SPEED 700.0f
#define ENEMY_SPEED 70.0f
#define ENEMY_SPAWN_RADIUS 900
#define SPAWN_DELAY 0.7f
#define WAVE_ENEMIES 12
#define MAX_TREES 150

typedef struct
{
    Vector2 position;
    float radius;
    Color color;
} Tree;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float speed;
    bool active;
    Color color;
} Enemy;

typedef struct
{
    Vector2 position;
    Vector2 direction;
    bool active;
    float timer; // Prevent spam
} Bullet;

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float health;
    float maxHealth;
    float radius;
    Vector2 shootDir;
} Player;

int main(void)
{
    srand(time(NULL));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Survival Game - Fixed Visuals");
    SetTargetFPS(60);

    // Game state
    Player player = {0};
    player.position = (Vector2){WORLD_WIDTH / 2, WORLD_HEIGHT / 2};
    player.velocity = (Vector2){0, 0};
    player.health = 100;
    player.maxHealth = 100;
    player.radius = 18;

    Enemy enemies[MAX_ENEMIES] = {0};
    Bullet bullets[MAX_BULLETS] = {0};

    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.zoom = 1.0f;

    // Generate FIXED trees that NEVER move (true motion tracking reference)
    Tree trees[MAX_TREES];
    for (int i = 0; i < MAX_TREES; i++)
    {
        trees[i].position = (Vector2){
            (float)(rand() % WORLD_WIDTH),
            (float)(rand() % WORLD_HEIGHT)};
        trees[i].radius = 25 + (rand() % 30);
        trees[i].color = (Color){20 + rand() % 40, 60 + rand() % 60, 20, 255};
    }

    // Game variables
    int wave = 1;
    int enemiesKilled = 0;
    int enemiesToKill = WAVE_ENEMIES;
    float spawnTimer = 0;
    bool paused = false;
    bool gameOver = false;
    bool canShoot = true; // One bullet per click

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        if (!paused && !gameOver)
        {
            // Input
            Vector2 moveDir = {0, 0};
            if (IsKeyDown(KEY_W))
                moveDir.y -= 1;
            if (IsKeyDown(KEY_S))
                moveDir.y += 1;
            if (IsKeyDown(KEY_A))
                moveDir.x -= 1;
            if (IsKeyDown(KEY_D))
                moveDir.x += 1;

            if (Vector2Length(moveDir) > 0)
            {
                moveDir = Vector2Normalize(moveDir);
                player.velocity = Vector2Scale(moveDir, PLAYER_SPEED);
            }
            else
            {
                player.velocity = Vector2Lerp(player.velocity, (Vector2){0, 0}, 12 * deltaTime);
            }

            player.position.x += player.velocity.x * deltaTime;
            player.position.y += player.velocity.y * deltaTime;

            // World bounds
            player.position.x = Clamp(player.position.x, player.radius, WORLD_WIDTH - player.radius);
            player.position.y = Clamp(player.position.y, player.radius, WORLD_HEIGHT - player.radius);

            // Mouse aiming
            Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
            player.shootDir = Vector2Normalize(Vector2Subtract(mouseWorld, player.position));

            // ONE BULLET PER CLICK (not hold)
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && canShoot)
            {
                canShoot = false;
                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].position = Vector2Add(player.position, Vector2Scale(player.shootDir, player.radius));
                        bullets[i].direction = player.shootDir;
                        bullets[i].active = true;
                        bullets[i].timer = 0;
                        break;
                    }
                }
            }
            if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                canShoot = true;
            }

            // Update bullets
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active)
                {
                    bullets[i].position = Vector2Add(bullets[i].position,
                                                     Vector2Scale(bullets[i].direction, BULLET_SPEED * deltaTime));
                    bullets[i].timer += deltaTime;

                    // Remove after 2 seconds or out of bounds
                    if (bullets[i].timer > 2.0f ||
                        bullets[i].position.x < 0 || bullets[i].position.x > WORLD_WIDTH ||
                        bullets[i].position.y < 0 || bullets[i].position.y > WORLD_HEIGHT)
                    {
                        bullets[i].active = false;
                    }
                }
            }

            // Spawn enemies
            spawnTimer += deltaTime;
            if (spawnTimer >= SPAWN_DELAY && enemiesKilled < enemiesToKill)
            {
                spawnTimer = 0;

                for (int i = 0; i < MAX_ENEMIES; i++)
                {
                    if (!enemies[i].active)
                    {
                        // Random spawn around player
                        float angle = (float)rand() / RAND_MAX * 2 * PI;
                        float dist = ENEMY_SPAWN_RADIUS + (rand() % 300);
                        enemies[i].position = Vector2Add(player.position,
                                                         (Vector2){cosf(angle) * dist, sinf(angle) * dist});

                        Vector2 dirToPlayer = Vector2Normalize(Vector2Subtract(player.position, enemies[i].position));
                        enemies[i].velocity = Vector2Scale(dirToPlayer, ENEMY_SPEED);
                        enemies[i].speed = ENEMY_SPEED;
                        enemies[i].active = true;
                        enemies[i].color = (Color){180 + rand() % 70, 60 + rand() % 40, 60, 255};
                        break;
                    }
                }
            }

            // Update enemies
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (enemies[i].active)
                {
                    // Chase player
                    Vector2 dirToPlayer = Vector2Normalize(Vector2Subtract(player.position, enemies[i].position));
                    enemies[i].velocity = Vector2Lerp(enemies[i].velocity,
                                                      Vector2Scale(dirToPlayer, enemies[i].speed), 3 * deltaTime);
                    enemies[i].position = Vector2Add(enemies[i].position,
                                                     Vector2Scale(enemies[i].velocity, deltaTime));

                    // ENEMY TOUCH = -20 HEALTH (instant)
                    if (Vector2Distance(enemies[i].position, player.position) < player.radius + 16)
                    {
                        player.health -= 20;
                        if (player.health <= 0)
                        {
                            gameOver = true;
                        }
                        // Knockback enemy
                        Vector2 knockback = Vector2Scale(Vector2Normalize(Vector2Subtract(enemies[i].position, player.position)), 200);
                        enemies[i].position = Vector2Add(enemies[i].position, knockback);
                    }
                }
            }

            // Bullet-enemy collisions (one-shot)
            for (int b = 0; b < MAX_BULLETS; b++)
            {
                if (bullets[b].active)
                {
                    for (int e = 0; e < MAX_ENEMIES; e++)
                    {
                        if (enemies[e].active)
                        {
                            if (Vector2Distance(bullets[b].position, enemies[e].position) < 16)
                            {
                                enemies[e].active = false;
                                bullets[b].active = false;
                                enemiesKilled++;
                                if (enemiesKilled >= enemiesToKill)
                                {
                                    wave++;
                                    enemiesToKill = WAVE_ENEMIES * wave;
                                    enemiesKilled = 0;
                                    spawnTimer = 0;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Smooth camera
            camera.target = Vector2Lerp(camera.target, player.position, 8 * deltaTime);
        }

        // Pause toggle
        if (IsKeyPressed(KEY_P))
            paused = !paused;

        BeginDrawing();
        ClearBackground((Color){15, 35, 25, 255}); // Nice dark green

        BeginMode2D(camera);

        // FIXED TREES (never move - perfect motion reference!)
        for (int i = 0; i < MAX_TREES; i++)
        {
            Vector2 pos = trees[i].position;

            // Tree shadow
            DrawCircleV((Vector2){pos.x + 8, pos.y + 8}, trees[i].radius + 5, Fade((Color){0, 0, 0, 80}, 0.4f));

            // Tree trunk
            DrawRectangle(pos.x - 8, pos.y - trees[i].radius / 2, 16, trees[i].radius, (Color){90, 60, 30, 255});

            // Tree canopy (multiple layers for depth)
            DrawCircleV(pos, trees[i].radius, Fade(trees[i].color, 0.9f));
            DrawCircleV((Vector2){pos.x - 10, pos.y - 8}, trees[i].radius * 0.7f, Fade((Color){10, 70, 20}, 1.0f));
            DrawCircleV((Vector2){pos.x + 12, pos.y - 5}, trees[i].radius * 0.6f, Fade((Color){20, 80, 25}, 0.9f));
        }

        // Ground texture (subtle dots/grass for depth)
        for (int x = 0; x < WORLD_WIDTH; x += 35)
        {
            for (int y = 0; y < WORLD_HEIGHT; y += 35)
            {
                if ((x + y) % 70 == 0)
                {
                    DrawPixelV((Vector2){x + (rand() % 10 - 5), y + (rand() % 10 - 5)}, DARKGREEN);
                }
            }
        }

        // Draw enemies (improved visuals)
        for (int i = 0; i < MAX_ENEMIES; i++)
        {
            if (enemies[i].active)
            {
                Vector2 pos = enemies[i].position;
                // Enemy body
                DrawCircleV(pos, 16, enemies[i].color);
                // Enemy eye
                DrawCircleV(Vector2Add(pos, (Vector2){-5, -5}), 4, Fade(WHITE, 0.9f));
                DrawCircleV(Vector2Add(pos, (Vector2){-5, -5}), 2, RED);
                // Direction line
                DrawLineEx(pos,
                           Vector2Add(pos, Vector2Scale(enemies[i].velocity, 25)), 3, DARKGRAY);
            }
        }

        // Bullets (glowing trail)
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (bullets[i].active)
            {
                DrawCircleV(bullets[i].position, 5, YELLOW);
                DrawCircleV(bullets[i].position, 10, Fade(YELLOW, 0.3f));
            }
        }

        // Player (improved)
        Vector2 ppos = player.position;
        DrawCircleV(ppos, player.radius, (Color){80, 150, 255, 255});
        DrawCircleV(ppos, 14, (Color){120, 180, 255, 255});
        // Player eye
        DrawCircleV(Vector2Add(ppos, player.shootDir), 5, WHITE);
        DrawCircleV(Vector2Add(ppos, player.shootDir), 3, (Color){50, 100, 200});
        // Shoot direction
        DrawLineEx(ppos,
                   Vector2Add(ppos, Vector2Scale(player.shootDir, 50)), 4, ORANGE);

        EndMode2D();

        // UI
        if (paused)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 150});
            DrawText("PAUSED", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 - 50, 72, WHITE);
            DrawText("Press P to resume", SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT / 2 + 30, 36, LIGHTGRAY);
        }

        if (gameOver)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){200, 50, 50, 180});
            DrawText("GAME OVER", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 - 50, 72, BLACK);
            DrawText(TextFormat("Wave Reached: %d", wave - 1), SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 30, 36, BLACK);
        }

        // Wave display
        DrawText(TextFormat("WAVE %d", wave), SCREEN_WIDTH / 2 - 100, 25, 52, (Color){255, 220, 100, 255});

        // Pause button
        Rectangle pauseBtn = {25, 25, 90, 50};
        DrawRectangleRec(pauseBtn, paused ? (Color){220, 60, 60} : (Color){100, 100, 100});
        DrawRectangleLinesEx(pauseBtn, 4, BLACK);
        DrawText("PAUSE", 45, 42, 22, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), pauseBtn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            paused = !paused;
        }

        // Health bar (improved)
        Rectangle healthBg = {SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT - 70, 320, 35};
        Rectangle healthFg = {healthBg.x + 5, healthBg.y + 5, (player.health / player.maxHealth) * (310), 25};
        DrawRectangleRec(healthBg, (Color){60, 60, 60, 255});
        DrawRectangleRec(healthFg, player.health > 40 ? GREEN : player.health > 15 ? ORANGE
                                                                                   : RED);
        DrawRectangleLinesEx(healthBg, 5, WHITE);
        DrawText("HEALTH", SCREEN_WIDTH / 2 - 45, SCREEN_HEIGHT - 65, 24, WHITE);

        // Guns button (placeholder)
        Rectangle gunBtn = {SCREEN_WIDTH - 130, SCREEN_HEIGHT - 70, 110, 40};
        DrawRectangleRec(gunBtn, (Color){220, 220, 220, 255});
        DrawRectangleLinesEx(gunBtn, 4, BLACK);
        DrawText("PISTOL", SCREEN_WIDTH - 118, SCREEN_HEIGHT - 55, 22, BLACK);

        // Instructions
        DrawText("WASD: Move | CLICK: Shoot | Mouse: Aim", 20, SCREEN_HEIGHT - 40, 22, (Color){200, 200, 200, 255});

        EndDrawing();
    }

    CloseWindow();
    return 0;
}