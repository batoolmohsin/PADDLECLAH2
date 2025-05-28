#include <raylib.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>

// Colors
Color Red = { 200, 30, 30, 255 };
Color DarkRed = { 100, 10, 10, 255 };
Color LightRed = { 255, 100, 100, 255 };
Color Yellow = { 255, 255, 100, 255 };
Color White = { 255, 255, 255, 255 };
Color Gray = { 180, 180, 180, 255 };

// Scores
int player_score = 0, opponent_score = 0;
int high_player_score = 0, high_opponent_score = 0;
const char* score_file = "scores.txt";
const float SPEED_INCREMENT = 0.5f;

// Audio
Sound paddle_sfx, score_sfx;
Music bg_music;

// Font
Font arcade_font;

// UI
Rectangle playRect, highScoreRect;
bool playHover = false, highHover = false;

// Ball class for gameplay
class Ball {
public:
    float x{}, y{}, speed_x{}, speed_y{}, radius{};

    void Draw() { DrawCircle((int)x, (int)y, radius, Yellow); }

    void Update() {
        x += speed_x;
        y += speed_y;

        if (y + radius >= GetScreenHeight() || y - radius <= 0)
            speed_y *= -1;

        if (x + radius >= GetScreenWidth()) {
            opponent_score++;
            PlaySound(score_sfx);
            ResetBall();
        }

        if (x - radius <= 0) {
            player_score++;
            PlaySound(score_sfx);
            ResetBall();
        }
    }

    void ResetBall() {
        x = GetScreenWidth() / 2.0f;
        y = GetScreenHeight() / 2.0f;
        int dir[] = { -1, 1 };
        speed_x = 7.0f * dir[GetRandomValue(0, 1)];
        speed_y = 7.0f * dir[GetRandomValue(0, 1)];
    }
};

// Animated balls for front interface only
struct AnimatedBall {
    float x, y;
    float speed_x, speed_y;
    float radius;
    Color color;
};

std::vector<AnimatedBall> menuBalls;

void InitMenuBalls(int count, int screenWidth, int screenHeight) {
    menuBalls.clear();
    for (int i = 0; i < count; ++i) {
        AnimatedBall ball;
        ball.x = GetRandomValue(100, screenWidth - 100);
        ball.y = GetRandomValue(100, screenHeight - 100);
        ball.radius = 20;
        ball.speed_x = GetRandomValue(2, 5) * ((i % 2 == 0) ? 1 : -1);
        ball.speed_y = GetRandomValue(2, 5) * ((i % 2 == 0) ? -1 : 1);
        ball.color = Yellow;
        menuBalls.push_back(ball);
    }
}

void UpdateMenuBalls(int screenWidth, int screenHeight) {
    for (auto& b : menuBalls) {
        b.x += b.speed_x;
        b.y += b.speed_y;

        if (b.x <= b.radius || b.x >= screenWidth - b.radius) b.speed_x *= -1;
        if (b.y <= b.radius || b.y >= screenHeight - b.radius) b.speed_y *= -1;
    }
}

void DrawMenuBalls() {
    for (const auto& b : menuBalls) {
        DrawCircle((int)b.x, (int)b.y, b.radius, b.color);
    }
}

// Paddle base class
class PaddleBase {
public:
    float x, y, width, height, speed;

    PaddleBase(float x, float y, float width, float height, float speed)
        : x(x), y(y), width(width), height(height), speed(speed) {}

    virtual void Update() = 0;
    virtual void Draw() {
        DrawRectangleRounded({ x, y, width, height }, 0.8f, 0, White);
    }

    virtual void Limit() {
        if (y < 0) y = 0;
        if (y + height > GetScreenHeight()) y = GetScreenHeight() - height;
    }

    virtual ~PaddleBase() = default;
};

// Player paddle
class PlayerPaddle : public PaddleBase {
public:
    int keyUp, keyDown;

    PlayerPaddle(float x, float y, float width, float height, float speed, int keyUp, int keyDown)
        : PaddleBase(x, y, width, height, speed), keyUp(keyUp), keyDown(keyDown) {}

    void Update() override {
        if (IsKeyDown(keyUp)) y -= speed;
        if (IsKeyDown(keyDown)) y += speed;
        Limit();
    }
};

// CPU paddle
class CpuPaddle : public PaddleBase {
public:
    Ball* ball;

    CpuPaddle(float x, float y, float width, float height, float speed, Ball* ball)
        : PaddleBase(x, y, width, height, speed), ball(ball) {}

    void Update() override {
        if (y + height / 2 > ball->y) y -= speed;
        if (y + height / 2 < ball->y) y += speed;
        Limit();
    }
};

Ball ball;
PaddleBase* left_paddle = nullptr;
PaddleBase* right_paddle = nullptr;

enum GameState { MENU, SELECT_MODE, PLAYING, PAUSED };
GameState state = MENU;
bool vs_cpu = true;

void ResetGame(int width, int height) {
    player_score = opponent_score = 0;
    ball.radius = 20;
    ball.ResetBall();

    float paddle_w = 25.0f, paddle_h = 120.0f, paddle_speed = 6.0f;

    delete left_paddle;
    delete right_paddle;

    right_paddle = new PlayerPaddle(width - 35.0f, height / 2 - paddle_h / 2,
        paddle_w, paddle_h, paddle_speed, KEY_UP, KEY_DOWN);

    if (vs_cpu)
        left_paddle = new CpuPaddle(10.0f, height / 2 - paddle_h / 2,
            paddle_w, paddle_h, paddle_speed, &ball);
    else
        left_paddle = new PlayerPaddle(10.0f, height / 2 - paddle_h / 2,
            paddle_w, paddle_h, paddle_speed, KEY_W, KEY_S);
}

void SaveScores() {
    std::ofstream file(score_file);
    if (file.is_open()) {
        if (player_score > high_player_score) high_player_score = player_score;
        if (opponent_score > high_opponent_score) high_opponent_score = opponent_score;
        file << high_player_score << " " << high_opponent_score;
        file.close();
    }
}

void LoadScores() {
    std::ifstream file(score_file);
    if (file.is_open()) {
        file >> high_player_score >> high_opponent_score;
        file.close();
    }
}

void LogSession() {
    std::ofstream log_file("log.txt", std::ios::app);
    if (log_file.is_open()) {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);
        log_file << "Game session at " << dt;
        log_file << "Player Score: " << player_score << ", Opponent Score: " << opponent_score << "\n\n";
        log_file.close();
    }
}

int main() {
    LoadScores();
    const int screen_width = 1280, screen_height = 800;
    InitWindow(screen_width, screen_height, "Paddle Clash - Red Edition");
    ToggleFullscreen();
    InitAudioDevice();

    paddle_sfx = LoadSound("hit.wav");
    score_sfx = LoadSound("score.wav");
    bg_music = LoadMusicStream("bg_music.mp3");
    arcade_font = LoadFont("arcade_font.ttf");

    PlayMusicStream(bg_music);
    SetTargetFPS(60);
    ResetGame(screen_width, screen_height);

    InitMenuBalls(6, screen_width, screen_height); // Add 6 animated balls

    float titleScale = 1.0f;
    bool growing = true;

    while (!WindowShouldClose()) {
        UpdateMusicStream(bg_music);
        Vector2 mouse = GetMousePosition();
        titleScale += (growing ? 0.005f : -0.005f);
        if (titleScale >= 1.05f || titleScale <= 0.95f) growing = !growing;

        if (state == MENU) {
            UpdateMenuBalls(screen_width, screen_height);
        }

        BeginDrawing();
        ClearBackground(DarkRed);

        if (state == MENU) {
            DrawMenuBalls();

            const char* title = "PADDLE CLASH";
            Vector2 titleSize = MeasureTextEx(arcade_font, title, 80 * titleScale, 4);
            DrawTextEx(arcade_font, title, { screen_width / 4 - titleSize.x / 4, 100 }, 80 * titleScale, 4, LightRed);

            playRect = { screen_width / 2 - 150, 300, 300, 60 };
            highScoreRect = { screen_width / 2 - 150, 380, 300, 60 };
            playHover = CheckCollisionPointRec(mouse, playRect);
            highHover = CheckCollisionPointRec(mouse, highScoreRect);

            DrawRectangleRounded(playRect, 0.3f, 10, playHover ? White : Gray);
            DrawRectangleRounded(highScoreRect, 0.3f, 10, highHover ? White : Gray);

            const char* playText = "PLAY GAME";
            Vector2 playTextSize = MeasureTextEx(arcade_font, playText, 30, 4);
            DrawTextEx(arcade_font, playText, { playRect.x + (playRect.width - playTextSize.x) / 4, playRect.y + 15 }, 30, 4, DarkRed);

            const char* highText = "HIGH SCORES";
            Vector2 highTextSize = MeasureTextEx(arcade_font, highText, 25, 4);
            DrawTextEx(arcade_font, highText, { highScoreRect.x + (highScoreRect.width - highTextSize.x) / 4, highScoreRect.y + 15 }, 25, 4, DarkRed);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (playHover) state = SELECT_MODE;
                if (highHover) {
                    bool show = true;
                    while (show && !WindowShouldClose()) {
                        BeginDrawing();
                        ClearBackground(DarkRed);
                        const char* hsTitle = "HIGH SCORES";
                        Vector2 hsTitleSize = MeasureTextEx(arcade_font, hsTitle, 40, 2);
                        DrawTextEx(arcade_font, hsTitle, { screen_width / 2 - hsTitleSize.x / 2, 250 }, 40, 2, Yellow);
                        DrawText(TextFormat("Player: %i", high_player_score), screen_width / 2 - 100, 320, 30, White);
                        DrawText(TextFormat("Opponent: %i", high_opponent_score), screen_width / 2 - 100, 360, 30, White);
                        DrawText("Press ENTER to return", screen_width / 2 - 130, 450, 20, GRAY);
                        if (IsKeyPressed(KEY_ENTER)) show = false;
                        EndDrawing();
                    }
                }
            }
        }

        else if (state == SELECT_MODE) {
            DrawTextEx(arcade_font, "CHOOSE MODE", { screen_width / 2 - 180, 200 }, 40, 2, Yellow);
            DrawText("1. Player vs CPU", screen_width / 2 - 100, 300, 30, White);
            DrawText("2. Player vs Player", screen_width / 2 - 100, 350, 30, White);
            if (IsKeyPressed(KEY_ONE)) { vs_cpu = true; ResetGame(screen_width, screen_height); state = PLAYING; }
            if (IsKeyPressed(KEY_TWO)) { vs_cpu = false; ResetGame(screen_width, screen_height); state = PLAYING; }
        }

        else if (state == PLAYING) {
            if (IsKeyPressed(KEY_P)) state = PAUSED;

            ball.Update();
            left_paddle->Update();
            right_paddle->Update();

            Rectangle ball_rect = { ball.x - ball.radius, ball.y - ball.radius, ball.radius * 2, ball.radius * 2 };
            Rectangle player_rect = { right_paddle->x, right_paddle->y, right_paddle->width, right_paddle->height };
            Rectangle opp_rect = { left_paddle->x, left_paddle->y, left_paddle->width, left_paddle->height };

            if (CheckCollisionCircleRec({ ball.x, ball.y }, ball.radius, player_rect) ||
                CheckCollisionCircleRec({ ball.x, ball.y }, ball.radius, opp_rect)) {
                ball.speed_x *= -1;
                ball.speed_x += (ball.speed_x > 0) ? SPEED_INCREMENT : -SPEED_INCREMENT;
                ball.speed_y += (ball.speed_y > 0) ? SPEED_INCREMENT : -SPEED_INCREMENT;
                PlaySound(paddle_sfx);
            }

            ClearBackground(Red);
            DrawRectangle(screen_width / 2, 0, screen_width / 2, screen_height, LightRed);
            DrawLine(screen_width / 2, 0, screen_width / 2, screen_height, White);
            DrawCircle(screen_width / 2, screen_height / 2, 150, DarkRed);

            ball.Draw();
            left_paddle->Draw();
            right_paddle->Draw();

            DrawText(TextFormat("%i", opponent_score), screen_width / 4 - 20, 20, 80, White);
            DrawText(TextFormat("%i", player_score), 3 * screen_width / 4 - 20, 20, 80, White);
        }

        else if (state == PAUSED) {
            DrawText("GAME PAUSED", screen_width / 2 - 150, screen_height / 2 - 50, 50, YELLOW);
            DrawText("Press P to Resume", screen_width / 2 - 140, screen_height / 2 + 20, 30, GRAY);
            if (IsKeyPressed(KEY_P)) state = PLAYING;
        }

        EndDrawing();
    }

    SaveScores();
    LogSession();

    delete left_paddle;
    delete right_paddle;

    UnloadSound(paddle_sfx);
    UnloadSound(score_sfx);
    UnloadMusicStream(bg_music);
    UnloadFont(arcade_font);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
