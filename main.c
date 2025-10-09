#include "raylib.h"
#include <stdlib.h>
#include <time.h>

#define MAX_BOTS 10

typedef struct {
    Vector2 pos;
    int health;
    int alive;
} Entity;

int main(void) {
    InitWindow(800, 600, "2D Battle Royale");
    SetTargetFPS(60);

    Entity player = {{400, 300}, 100, 1};
    Entity bots[MAX_BOTS];

    srand(time(NULL));
    for (int i = 0; i < MAX_BOTS; i++) {
        bots[i].pos = (Vector2){rand()%800, rand()%600};
        bots[i].health = 100;
        bots[i].alive = 1;
    }

    float safeRadius = 300.0f;
    Vector2 safeCenter = {400, 300};

    while (!WindowShouldClose()) {
        // --- Player movement ---
        if (IsKeyDown(KEY_W)) player.pos.y -= 2;
        if (IsKeyDown(KEY_S)) player.pos.y += 2;
        if (IsKeyDown(KEY_A)) player.pos.x -= 2;
        if (IsKeyDown(KEY_D)) player.pos.x += 2;

        // --- Safe zone shrink ---
        safeRadius -= 0.01f;

        // --- Damage outside safe zone ---
        if (Vector2Distance(player.pos, safeCenter) > safeRadius) player.health--;
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bots[i].alive && Vector2Distance(bots[i].pos, safeCenter) > safeRadius)
                bots[i].health--;
            if (bots[i].health <= 0) bots[i].alive = 0;
        }

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawCircleLines(safeCenter.x, safeCenter.y, safeRadius, GREEN);

        if (player.alive) DrawCircleV(player.pos, 10, BLUE);
        for (int i = 0; i < MAX_BOTS; i++) {
            if (bots[i].alive) DrawCircleV(bots[i].pos, 10, RED);
        }

        DrawText(TextFormat("Health: %d", player.health), 10, 10, 20, BLACK);

        EndDrawing();

        if (player.health <= 0) player.alive = 0;
    }

    CloseWindow();
    return 0;
}
