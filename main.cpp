#ifdef __EMSCRIPTEN__
#include "web/sfml_web.hpp"
#include <emscripten.h>
#include <functional>
#else
#include <SFML/Graphics.hpp>
#endif
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <deque>
#include <cmath>
#include <algorithm>

#ifdef __EMSCRIPTEN__
// Browser build: the board size is picked at startup to suit the screen (see main).
int width = 32;
int height = 24;
#else
const int width = 32;
const int height = 24;
#endif
const int blockSize = 24;
const float frameTime = 0.09f;

#ifdef __EMSCRIPTEN__
// Browser build: the font is bundled, and the high score lives in a folder
// that the page keeps in the visitor's browser (IndexedDB).
const char *const kFontPath = "/DejaVuSans-Bold.ttf";
const char *const kHighScoreFile = "/persist/highscore.txt";
#else
const char *const kFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
const char *const kHighScoreFile = "highscore.txt";
#endif

enum Direction
{
    STOP = 0,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

struct Segment
{
    int x, y;
};

bool gameOver = false;
bool paused = false;
int score = 0;
int highScore = 0;
Direction dir = STOP;
Direction nextDir = STOP;

int foodX, foodY;
bool bonusFood = false;
float bonusTimer = 0.f;

std::deque<Segment> snake;

void LoadHighScore()
{
    std::ifstream file(kHighScoreFile);
    if (file.is_open())
    {
        file >> highScore;
        file.close();
    }
}

void SaveHighScore()
{
    if (score > highScore)
    {
        highScore = score;
        std::ofstream file(kHighScoreFile, std::ios::trunc);
        if (file.is_open())
        {
            file << highScore;
            file.close();
        }
#ifdef __EMSCRIPTEN__
        EM_ASM({
            try
            {
                FS.syncfs(false, function(err){});
            }
            catch (e)
            {
            }
        });
#endif
    }
}

void SpawnFood()
{
    bool onSnake;
    do
    {
        onSnake = false;
        foodX = rand() % width;
        foodY = rand() % height;
        for (auto &s : snake)
            if (s.x == foodX && s.y == foodY)
            {
                onSnake = true;
                break;
            }
    } while (onSnake);

    bonusFood = (rand() % 5 == 0); // 1 in 5 chance of bonus food
    bonusTimer = 5.f;
}

void Setup()
{
    gameOver = false;
    paused = false;
    dir = STOP;
    nextDir = STOP;
    score = 0;
    snake.clear();
    snake.push_back({width / 2, height / 2});
    SpawnFood();
}

void Logic(float dt)
{
    if (bonusFood)
    {
        bonusTimer -= dt;
        if (bonusTimer <= 0.f)
            SpawnFood();
    }

    dir = nextDir;
    if (dir == STOP)
        return;

    Segment head = snake.front();
    switch (dir)
    {
    case LEFT:
        head.x--;
        break;
    case RIGHT:
        head.x++;
        break;
    case UP:
        head.y--;
        break;
    case DOWN:
        head.y++;
        break;
    default:
        break;
    }

    if (head.x < 0 || head.x >= width || head.y < 0 || head.y >= height)
    {
        gameOver = true;
        return;
    }
    for (auto &s : snake)
    {
        if (s.x == head.x && s.y == head.y)
        {
            gameOver = true;
            return;
        }
    }

    snake.push_front(head);

    if (head.x == foodX && head.y == foodY)
    {
        score += bonusFood ? 5 : 1;
        SpawnFood();
    }
    else
        snake.pop_back();
}

int main()
{
#ifdef __EMSCRIPTEN__
    // On a small screen (a phone), use a smaller board so the cells stay big enough to play.
    // The page keeps 84 pixels under the board for the buttons and the hint,
    // and the score strip inside the canvas takes 50 more.
    int screenW = EM_ASM_INT({ return window.innerWidth; });
    int screenH = EM_ASM_INT({ return window.innerHeight; });
    if (screenW < 700 || screenH < 560)
    {
        width = std::max(10, std::min(32, screenW / blockSize));
        height = std::max(10, std::min(24, (screenH - 84 - 50) / blockSize));
    }
#endif

    srand((unsigned)time(0));
    LoadHighScore();
    Setup();

    sf::RenderWindow window(sf::VideoMode(width * blockSize, height * blockSize + 50), "Snake - Enhanced");
    window.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = font.loadFromFile(kFontPath);

    sf::Text hud;
    if (hasFont)
    {
        hud.setFont(font);
        hud.setCharacterSize(20);
        hud.setFillColor(sf::Color::White);
        hud.setPosition(10, height * blockSize + 10);
    }

    sf::Text bigMsg;
    if (hasFont)
    {
        bigMsg.setFont(font);
        bigMsg.setCharacterSize(36);
        bigMsg.setFillColor(sf::Color::White);
        bigMsg.setStyle(sf::Text::Bold);
    }

    sf::Clock clock;
    sf::Clock deltaClock;

    // One frame of the game. The desktop build calls it in a while loop.
    // The browser build hands it to the browser's frame callback.
    auto frame = [&]()
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed)
            {
                if (!gameOver)
                {
                    if ((event.key.code == sf::Keyboard::A || event.key.code == sf::Keyboard::Left) && dir != RIGHT)
                        nextDir = LEFT;

                    else if ((event.key.code == sf::Keyboard::D || event.key.code == sf::Keyboard::Right) && dir != LEFT)
                        nextDir = RIGHT;

                    else if ((event.key.code == sf::Keyboard::W || event.key.code == sf::Keyboard::Up) && dir != DOWN)
                        nextDir = UP;

                    else if ((event.key.code == sf::Keyboard::S || event.key.code == sf::Keyboard::Down) && dir != UP)
                        nextDir = DOWN;

                    else if (event.key.code == sf::Keyboard::P)
                        paused = !paused;
                }
                else if (event.key.code == sf::Keyboard::Enter)
                    Setup();
            }
        }

        float dt = deltaClock.restart().asSeconds();
        (void)dt;

        if (!gameOver && !paused && clock.getElapsedTime().asSeconds() >= frameTime)
        {
            Logic(frameTime);
            if (gameOver)
                SaveHighScore();
            clock.restart();
        }

        window.clear(sf::Color(15, 15, 20));

        // grid background
        for (int gx = 0; gx <= width; gx++)
        {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(gx * blockSize, 0), sf::Color(30, 30, 38)),
                sf::Vertex(sf::Vector2f(gx * blockSize, height * blockSize), sf::Color(30, 30, 38))};
            window.draw(line, 2, sf::Lines);
        }
        for (int gy = 0; gy <= height; gy++)
        {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(0, gy * blockSize), sf::Color(30, 30, 38)),
                sf::Vertex(sf::Vector2f(width * blockSize, gy * blockSize), sf::Color(30, 30, 38))};
            window.draw(line, 2, sf::Lines);
        }

        // food (pulsing glow)
        float pulse = 3.f * std::sin(deltaClock.getElapsedTime().asSeconds() * 6.f);
        sf::CircleShape food(blockSize / 2.2f + pulse * 0.3f);
        food.setOrigin(food.getRadius(), food.getRadius());
        food.setPosition(foodX * blockSize + blockSize / 2.f, foodY * blockSize + blockSize / 2.f);
        food.setFillColor(bonusFood ? sf::Color(255, 215, 0) : sf::Color(220, 50, 50));
        food.setOutlineThickness(2.f);
        food.setOutlineColor(sf::Color(255, 255, 255, 120));
        window.draw(food);

        // snake (color shifts from head to tail, rounded look)
        for (size_t i = 0; i < snake.size(); i++)
        {
            float t = (snake.size() <= 1) ? 0 : (float)i / (snake.size() - 1);
            sf::Color c(
                (sf::Uint8)(60 + t * 20),
                (sf::Uint8)(200 - t * 90),
                (sf::Uint8)(100 + t * 40));
            sf::RectangleShape seg(sf::Vector2f(blockSize - 2, blockSize - 2));
            seg.setPosition(snake[i].x * blockSize + 1, snake[i].y * blockSize + 1);
            seg.setFillColor(i == 0 ? sf::Color(255, 220, 60) : c);
            window.draw(seg);
        }

        // eyes on head
        if (!snake.empty())
        {
            sf::CircleShape eye(2.5f);
            eye.setFillColor(sf::Color::Black);
            float hx = snake[0].x * blockSize + blockSize / 2.f;
            float hy = snake[0].y * blockSize + blockSize / 2.f;
            float ox = 0, oy = 0;
            if (dir == LEFT)
                ox = -5;
            else if (dir == RIGHT)
                ox = 5;
            else if (dir == UP)
                oy = -5;
            else if (dir == DOWN)
                oy = 5;
            eye.setPosition(hx + ox - 6, hy + oy - 3);
            window.draw(eye);
            eye.setPosition(hx + ox + 3, hy + oy - 3);
            window.draw(eye);
        }

        // HUD
        if (hasFont)
        {
            // A narrow board (a phone) gets shorter text so everything fits.
            bool narrow = width * blockSize < 600;

            hud.setCharacterSize(narrow ? 16 : 20);
            if (narrow)
                hud.setString("Score: " + std::to_string(score) + "  High: " + std::to_string(highScore) + (paused ? "  [PAUSED]" : ""));
            else
                hud.setString("Score: " + std::to_string(score) + "   High Score: " + std::to_string(highScore) + (paused ? "   [PAUSED]" : ""));
            window.draw(hud);

            if (gameOver)
            {
                if (narrow)
                {
                    bigMsg.setCharacterSize(36);
                    bigMsg.setString("GAME OVER");
                    bigMsg.setPosition((width * blockSize - bigMsg.getLocalBounds().width) / 2.f, height * blockSize / 2.f - 40);
                    window.draw(bigMsg);

                    bigMsg.setCharacterSize(18);
                    bigMsg.setString("Press Enter or Restart");
                    bigMsg.setPosition((width * blockSize - bigMsg.getLocalBounds().width) / 2.f, height * blockSize / 2.f + 8);
                    window.draw(bigMsg);
                }
                else
                {
                    bigMsg.setCharacterSize(36);
                    bigMsg.setString("GAME OVER - Press Enter to Restart");
                    bigMsg.setPosition((width * blockSize - bigMsg.getLocalBounds().width) / 2.f, height * blockSize / 2.f - 20);
                    window.draw(bigMsg);
                }
            }
        }

        window.display();
    };

#ifdef __EMSCRIPTEN__
    static std::function<void()> webFrame = frame;
    emscripten_set_main_loop([]()
                             { webFrame(); }, 0, 1);
#else
    while (window.isOpen())
        frame();
#endif

    return 0;
}