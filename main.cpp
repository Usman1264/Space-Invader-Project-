#include "raylib.h"
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

const int MAX_BULLETS = 200;
const int MAX_ALIENS = 200;

enum GameState {
    PLAYING,
    GAME_OVER,
    VICTORY,
    LEVEL_TRANSITION
};

struct Player {
    float x, y;
    float width, height;
    float speed;
    Color color;
};

struct Bullet {
    float x, y;
    float speed;
    float radius;
    bool active;
};

struct Alien {
    float x, y;
    float width, height;
    float speed;
    Color color;
    bool active;
};

struct GameData {
    GameState state;
    int score;

    int level;
    int lives;

    int aliensToSpawn;
    int aliensSpawned;
    int activeAliens;
    float lastAlienSpawn;
    float alienSpawnRate;

    Player player;

    Bullet bullets[MAX_BULLETS];
    int bulletCount;

    Alien aliens[MAX_ALIENS];
    int alienCount;

    float lastBulletTime;
    float bulletCooldown;
};

void InitPlayer(Player* p) {
    p->x = SCREEN_WIDTH / 2 - 25;
    p->y = SCREEN_HEIGHT - 60;
    p->width = 50;
    p->height = 50;
    p->speed = 5;
    p->color = GREEN;
}

void InitGame(GameData* g) {
    g->state = PLAYING;
    g->score = 0;

    g->level = 1;
    g->lives = 2;

    g->aliensToSpawn = 10;
    g->aliensSpawned = 0;
    g->activeAliens = 0;

    g->lastAlienSpawn = 0;
    g->alienSpawnRate = 2.0f;

    g->lastBulletTime = 0;
    g->bulletCooldown = 0.2f;

    InitPlayer(&g->player);

    g->bulletCount = 0;
    g->alienCount = 0;

    srand(time(NULL));
}

void ResetArrays(GameData* g) {
    g->bulletCount = 0;
    g->alienCount = 0;
}

void StartNextLevel(GameData* g) {
    ResetArrays(g);

    g->aliensSpawned = 0;
    g->activeAliens = 0;

    g->aliensToSpawn = 10 + g->level * 5;
    g->alienSpawnRate = 2.0f - g->level * 0.3f;
    if (g->alienSpawnRate < 0.8f) g->alienSpawnRate = 0.8f;

    g->lastAlienSpawn = GetTime();

    g->state = PLAYING;
}

bool CheckCollisionRect(float x1, float y1, float w1, float h1,
    float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool CheckCollisionCircleRect(float cx, float cy, float r,
    float rx, float ry, float rw, float rh) {
    float closestX = fmaxf(rx, fminf(cx, rx + rw));
    float closestY = fmaxf(ry, fminf(cy, ry + rh));
    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx * dx + dy * dy) < (r * r);
}

void UpdatePlayer(GameData* g) {
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        g->player.x -= g->player.speed;
        if (g->player.x < 0) g->player.x = 0;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        g->player.x += g->player.speed;
        if (g->player.x > SCREEN_WIDTH - g->player.width)
            g->player.x = SCREEN_WIDTH - g->player.width;
    }
}

void FireBullet(GameData* g) {
    float t = GetTime();
    if (t - g->lastBulletTime < g->bulletCooldown) return;

    if (g->bulletCount < MAX_BULLETS) {
        Bullet* b = &g->bullets[g->bulletCount++];
        b->x = g->player.x + g->player.width / 2;
        b->y = g->player.y;
        b->speed = 7;
        b->radius = 5;
        b->active = true;
    }

    g->lastBulletTime = t;
}

void UpdateBullets(GameData* g) {
    for (int i = 0; i < g->bulletCount; i++) {
        if (g->bullets[i].active) {
            g->bullets[i].y -= g->bullets[i].speed;
            if (g->bullets[i].y < 0)
                g->bullets[i].active = false;
        }
    }

    // compact array (manual remove_if)
    int write = 0;
    for (int i = 0; i < g->bulletCount; i++) {
        if (g->bullets[i].active)
            g->bullets[write++] = g->bullets[i];
    }
    g->bulletCount = write;
}

void CreateAlien(GameData* g) {
    if (g->alienCount >= MAX_ALIENS) return;

    Alien* a = &g->aliens[g->alienCount++];
    a->x = rand() % (SCREEN_WIDTH - 40);
    a->y = -40;
    a->width = 40;
    a->height = 40;
    a->speed = 1.0f + g->level * 0.8f;
    a->color = RED;
    a->active = true;

    g->aliensSpawned++;
    g->activeAliens++;
}

void SpawnAliens(GameData* g) {
    float t = GetTime();
    if (t - g->lastAlienSpawn >= g->alienSpawnRate &&
        g->aliensSpawned < g->aliensToSpawn) {

        CreateAlien(g);
        g->lastAlienSpawn = t;
    }
}

void UpdateAliens(GameData* g) {
    for (int i = 0; i < g->alienCount; i++) {
        Alien* a = &g->aliens[i];
        if (a->active) {
            a->y += a->speed;

            if (a->y > SCREEN_HEIGHT) {
                g->lives--;
                if (g->lives <= 0) g->state = GAME_OVER;
                else StartNextLevel(g);
                return;
            }

            if (CheckCollisionRect(a->x, a->y, a->width, a->height,
                g->player.x, g->player.y,
                g->player.width, g->player.height)) {
                g->lives--;
                if (g->lives <= 0) g->state = GAME_OVER;
                else StartNextLevel(g);
                return;
            }
        }
    }
}

void CheckBulletCollisions(GameData* g) {
    for (int i = 0; i < g->bulletCount; i++) {
        for (int j = 0; j < g->alienCount; j++) {
            if (!g->aliens[j].active) continue;
            if (!g->bullets[i].active) continue;

            if (CheckCollisionCircleRect(
                g->bullets[i].x, g->bullets[i].y, g->bullets[i].radius,
                g->aliens[j].x, g->aliens[j].y,
                g->aliens[j].width, g->aliens[j].height)) {

                g->bullets[i].active = false;
                g->aliens[j].active = false;

                g->score += 10;
                g->activeAliens--;
            }
        }
    }

    // compact alien array
    int write = 0;
    for (int i = 0; i < g->alienCount; i++) {
        if (g->aliens[i].active)
            g->aliens[write++] = g->aliens[i];
    }
    g->alienCount = write;

    // level done?
    if (g->activeAliens == 0 && g->aliensSpawned >= g->aliensToSpawn) {
        g->level++;
        if (g->level > 3)
            g->state = VICTORY;
        else
            g->state = LEVEL_TRANSITION;
    }
}

void DrawPlayer(const Player* p) {
    Vector2 v1 = { p->x + p->width / 2, p->y };
    Vector2 v2 = { p->x, p->y + p->height };
    Vector2 v3 = { p->x + p->width, p->y + p->height };
    DrawTriangle(v1, v2, v3, p->color);
    DrawTriangleLines(v1, v2, v3, YELLOW);
}

void DrawBullets(const GameData* g) {
    for (int i = 0; i < g->bulletCount; i++) {
        if (g->bullets[i].active)
            DrawCircle(g->bullets[i].x, g->bullets[i].y, g->bullets[i].radius, YELLOW);
    }
}

void DrawAliens(const GameData* g) {
    for (int i = 0; i < g->alienCount; i++) {
        Alien a = g->aliens[i];
        DrawRectangle(a.x, a.y, a.width, a.height, a.color);
        DrawRectangleLines(a.x, a.y, a.width, a.height, MAROON);
    }
}

void DrawStars() {
    for (int i = 0; i < 50; i++) {
        int x = (i * 37) % SCREEN_WIDTH;
        int y = ((int)(i * 53 + GetTime() * 10)) % SCREEN_HEIGHT;
        DrawPixel(x, y, WHITE);
    }
}

void DrawGameOver(int score) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    DrawText("GAME OVER", 300, 230, 48, RED);
}

void DrawVictory(int score) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.8f));
    DrawText("VICTORY!", 300, 230, 48, GREEN);
}

void DrawLevelTransition(int lvl) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.85f));
    char msg[64];
    sprintf_s(msg, "LEVEL %d STARTING...", lvl);
    DrawText(msg, 250, 260, 30, YELLOW);
}

void DrawHUD(const GameData* g) {
    char t[64];
    sprintf_s(t, "Score: %d", g->score);
    DrawText(t, 10, 10, 20, GREEN);

    sprintf_s(t, "Lives: %d", g->lives);
    DrawText(t, 10, 40, 20, RED);

    sprintf_s(t, "Level: %d", g->level);
    DrawText(t, 10, 70, 20, YELLOW);
}

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Invaders NO VECTOR");
    SetTargetFPS(60);

    GameData g;
    InitGame(&g);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R) && (g.state == GAME_OVER || g.state == VICTORY))
            InitGame(&g);

        if (g.state == LEVEL_TRANSITION && IsKeyPressed(KEY_SPACE))
            StartNextLevel(&g);

        if (g.state == PLAYING) {
            UpdatePlayer(&g);
            if (IsKeyDown(KEY_SPACE)) FireBullet(&g);
            UpdateBullets(&g);
            UpdateAliens(&g);
            CheckBulletCollisions(&g);
            SpawnAliens(&g);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawStars();
        DrawPlayer(&g.player);
        DrawBullets(&g);
        DrawAliens(&g);
        DrawHUD(&g);

        if (g.state == GAME_OVER) DrawGameOver(g.score);
        if (g.state == VICTORY) DrawVictory(g.score);
        if (g.state == LEVEL_TRANSITION) DrawLevelTransition(g.level);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
