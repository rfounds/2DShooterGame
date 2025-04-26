#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <SDL2_gfxPrimitives.h>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <cmath>

enum GameState {
    MENU,
    PLAYING,
    GAME_OVER,
    WIN
};

struct Projectile {
    SDL_Rect rect;
    int dx, dy;
    bool isEnemy;
};

struct Enemy {
    SDL_Rect rect;
    Uint32 lastShot;
    Uint32 fireRate;
    double angle;
};

// Helper function to draw a filled triangle
void DrawFilledTriangle(SDL_Renderer* renderer, int x, int y, int size, double angle, 
                       Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    Sint16 vx[3];
    Sint16 vy[3];
    double radians = angle * M_PI / 180.0;
    
    // Calculate points for an isosceles triangle
    vx[0] = x + static_cast<int>(sin(radians) * size);  // Nose
    vy[0] = y - static_cast<int>(cos(radians) * size);
    
    vx[1] = x + static_cast<int>(sin(radians + 2.618) * size);  // Back right
    vy[1] = y - static_cast<int>(cos(radians + 2.618) * size);
    
    vx[2] = x + static_cast<int>(sin(radians - 2.618) * size);  // Back left
    vy[2] = y - static_cast<int>(cos(radians - 2.618) * size);

    filledPolygonRGBA(renderer, vx, vy, 3, r, g, b, a);
}

// Helper function to draw a rotated square
void DrawRotatedSquare(SDL_Renderer* renderer, const SDL_Rect& rect, double angle) {
    // Calculate the center point
    int centerX = rect.x + rect.w / 2;
    int centerY = rect.y + rect.h / 2;
    
    // Calculate the corners of the square
    Sint16 vx[4];
    Sint16 vy[4];
    double radians = angle * M_PI / 180.0;
    double halfWidth = rect.w / 2.0;
    double halfHeight = rect.h / 2.0;
    
    // Calculate rotated points
    vx[0] = centerX + static_cast<int>((cos(radians) * -halfWidth) - (sin(radians) * -halfHeight));
    vy[0] = centerY + static_cast<int>((sin(radians) * -halfWidth) + (cos(radians) * -halfHeight));
    
    vx[1] = centerX + static_cast<int>((cos(radians) * halfWidth) - (sin(radians) * -halfHeight));
    vy[1] = centerY + static_cast<int>((sin(radians) * halfWidth) + (cos(radians) * -halfHeight));
    
    vx[2] = centerX + static_cast<int>((cos(radians) * halfWidth) - (sin(radians) * halfHeight));
    vy[2] = centerY + static_cast<int>((sin(radians) * halfWidth) + (cos(radians) * halfHeight));
    
    vx[3] = centerX + static_cast<int>((cos(radians) * -halfWidth) - (sin(radians) * halfHeight));
    vy[3] = centerY + static_cast<int>((sin(radians) * -halfWidth) + (cos(radians) * halfHeight));

    filledPolygonRGBA(renderer, vx, vy, 4, 255, 165, 0, 255);
}

// Helper function to check if a point is inside a circle
bool CircleCollision(int circleX, int circleY, int radius, const SDL_Rect& rect) {
    // Find the closest point to the circle within the rectangle
    int closestX = std::max(rect.x, std::min(circleX, rect.x + rect.w));
    int closestY = std::max(rect.y, std::min(circleY, rect.y + rect.h));
    
    // Calculate the distance between the circle's center and the closest point
    int distanceX = circleX - closestX;
    int distanceY = circleY - closestY;
    
    // If the distance is less than the circle's radius, an intersection occurs
    return (distanceX * distanceX + distanceY * distanceY) < (radius * radius);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    Mix_Chunk* pew = Mix_LoadWAV("shot.wav");
    
    SDL_Window* window = SDL_CreateWindow("WASD + Shoot",
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        1280, 720,
                                        SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(0, 1280 - 40);
    std::uniform_int_distribution<> spawnTimeDist(1000, 3000);

    // Load font
    TTF_Font* font = TTF_OpenFont("font/Roboto-VariableFont_wdth,wght.ttf", 64);
    if (!font) {
        printf("TTF_OpenFont error: %s\n", TTF_GetError());
        return 1;
    }

    // Create text textures
    SDL_Color textColor = {255, 255, 255, 255};
    
    // Menu text
    SDL_Surface* menuSurface = TTF_RenderText_Solid(font, "Press SPACE to Play", textColor);
    SDL_Texture* menuTexture = SDL_CreateTextureFromSurface(renderer, menuSurface);
    SDL_Rect menuRect = {
        1280/2 - menuSurface->w/2,
        720/2 - menuSurface->h/2,
        menuSurface->w,
        menuSurface->h
    };
    SDL_FreeSurface(menuSurface);

    // Game Over text
    SDL_Surface* gameOverSurface = TTF_RenderText_Solid(font, "Game Over - Press R to Restart", textColor);
    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
    SDL_Rect gameOverRect = {
        1280/2 - gameOverSurface->w/2,
        720/2 - gameOverSurface->h/2,
        gameOverSurface->w,
        gameOverSurface->h
    };
    SDL_FreeSurface(gameOverSurface);

    // Win text
    SDL_Surface* winSurface = TTF_RenderText_Solid(font, "You Win! Press R to Play Again", textColor);
    SDL_Texture* winTexture = SDL_CreateTextureFromSurface(renderer, winSurface);
    SDL_Rect winRect = {
        1280/2 - winSurface->w/2,
        720/2 - winSurface->h/2,
        winSurface->w,
        winSurface->h
    };
    SDL_FreeSurface(winSurface);

    // Score text setup
    TTF_Font* scoreFont = TTF_OpenFont("font/Roboto-VariableFont_wdth,wght.ttf", 32);
    SDL_Texture* scoreTexture = nullptr;
    SDL_Rect scoreRect = {10, 10, 0, 0};

    // Game state variables
    const int playerSize = 40; // Size for the triangle
    SDL_Point playerPos = { 300, 220 }; // Center position of the player
    const int playerRadius = playerSize / 2; // For collision detection
    const int speed = 5;
    double playerAngle = 0; // Angle in degrees
    int dirX = 0, dirY = -1;
    int enemiesKilled = 0;
    const int enemiesRequiredToWin = 5;

    std::vector<Projectile> projectiles;
    std::vector<Enemy> enemies;

    Uint32 lastShot = 0;
    const Uint32 fireRate = 200;
    
    Uint32 lastEnemySpawn = SDL_GetTicks();
    Uint32 nextEnemySpawn = spawnTimeDist(gen);

    GameState gameState = MENU;
    bool running = true;
    SDL_Event event;

    auto resetGame = [&]() {
        playerPos = { 300, 220 };
        playerAngle = 0;
        dirX = 0;
        dirY = -1;
        projectiles.clear();
        enemies.clear();
        lastShot = 0;
        lastEnemySpawn = SDL_GetTicks();
        nextEnemySpawn = spawnTimeDist(gen);
        enemiesKilled = 0;
    };

    auto updateScoreDisplay = [&]() {
        if (scoreTexture) {
            SDL_DestroyTexture(scoreTexture);
        }
        std::string scoreText = "Enemies Killed: " + std::to_string(enemiesKilled) + "/" + std::to_string(enemiesRequiredToWin);
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(scoreFont, scoreText.c_str(), textColor);
        scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        scoreRect.w = scoreSurface->w;
        scoreRect.h = scoreSurface->h;
        SDL_FreeSurface(scoreSurface);
    };

    updateScoreDisplay(); // Initial score display

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            
            if (event.type == SDL_KEYDOWN) {
                if (gameState == MENU && event.key.keysym.sym == SDLK_SPACE) {
                    gameState = PLAYING;
                    resetGame();
                }
                else if ((gameState == GAME_OVER || gameState == WIN) && 
                         event.key.keysym.sym == SDLK_r) {
                    gameState = PLAYING;
                    resetGame();
                }
            }
        }

        if (gameState == PLAYING) {
            const Uint8* keystates = SDL_GetKeyboardState(NULL);
            Uint32 now = SDL_GetTicks();

            // Reset direction for this frame
            dirX = 0;
            dirY = 0;

            // Player movement and direction
            if (keystates[SDL_SCANCODE_W]) { 
                dirY = -1;
                playerPos.y -= speed;
            }
            if (keystates[SDL_SCANCODE_S]) { 
                dirY = 1;
                playerPos.y += speed;
            }
            if (keystates[SDL_SCANCODE_A]) { 
                dirX = -1;
                playerPos.x -= speed;
            }
            if (keystates[SDL_SCANCODE_D]) { 
                dirX = 1;
                playerPos.x += speed;
            }

            // Update player angle based on movement direction
            if (dirX != 0 || dirY != 0) {
                playerAngle = atan2(dirX, -dirY) * 180.0 / M_PI;
            }

            // Keep player in bounds
            playerPos.x = SDL_max(playerSize/2, SDL_min(playerPos.x, 1280 - playerSize/2));
            playerPos.y = SDL_max(playerSize/2, SDL_min(playerPos.y, 720 - playerSize/2));

            // Player shooting
            if (keystates[SDL_SCANCODE_SPACE] && now - lastShot >= fireRate) {
                Projectile p;
                p.rect = {playerPos.x - 5, playerPos.y - 5, 10, 10};
                
                // Calculate projectile velocity based on player direction
                float speed = 10.0f;
                if (dirX == 0 && dirY == 0) {
                    // If not moving, shoot in the last direction we were facing
                    float radians = playerAngle * M_PI / 180.0;
                    p.dx = static_cast<int>(sin(radians) * speed);
                    p.dy = static_cast<int>(-cos(radians) * speed);
                } else {
                    // Normalize the direction vector for consistent speed
                    float length = sqrt(dirX * dirX + dirY * dirY);
                    p.dx = static_cast<int>((dirX / length) * speed);
                    p.dy = static_cast<int>((dirY / length) * speed);
                }
                
                p.isEnemy = false;
                projectiles.push_back(p);
                Mix_PlayChannel(-1, pew, 0);
                lastShot = now;
            }

            // Spawn enemies
            if (now - lastEnemySpawn >= nextEnemySpawn) {
                Enemy enemy;
                enemy.rect = {xDist(gen), 0, 40, 40};
                enemy.lastShot = now;
                enemy.fireRate = 1500;
                enemy.angle = 0;
                enemies.push_back(enemy);
                lastEnemySpawn = now;
                nextEnemySpawn = spawnTimeDist(gen);
            }

            // Update enemy rotation
            for (auto& enemy : enemies) {
                enemy.angle += 10.0; // Rotate 10 degrees per frame
                if (enemy.angle >= 360.0) {
                    enemy.angle -= 360.0;
                }
                enemy.rect.y += 2;
                
                // Shoot at player
                if (now - enemy.lastShot >= enemy.fireRate) {
                    Projectile p;
                    p.rect = {enemy.rect.x + enemy.rect.w/2 - 5, enemy.rect.y + enemy.rect.h/2 - 5, 10, 10};
                    
                    // Calculate direction to player
                    float dx = playerPos.x - enemy.rect.x;
                    float dy = playerPos.y - enemy.rect.y;
                    float len = sqrt(dx*dx + dy*dy);
                    
                    // Normalize and set slower speed
                    p.dx = (dx/len) * 5;
                    p.dy = (dy/len) * 5;
                    p.isEnemy = true;
                    
                    projectiles.push_back(p);
                    enemy.lastShot = now;
                }
            }

            // Update projectiles
            for (auto& p : projectiles) {
                p.rect.x += p.dx;
                p.rect.y += p.dy;
            }

            // Collision detection
            // Remove off-screen projectiles and enemies
            projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(),
                    [](const Projectile& p) {
                        return p.rect.x < 0 || p.rect.x > 1280 || 
                               p.rect.y < 0 || p.rect.y > 720;
                    }
                ),
                projectiles.end()
            );

            enemies.erase(
                std::remove_if(enemies.begin(), enemies.end(),
                    [](const Enemy& e) { return e.rect.y > 720; }
                ),
                enemies.end()
            );

            // Check enemy projectile collision with player using circle collision
            for (const auto& p : projectiles) {
                if (p.isEnemy && CircleCollision(playerPos.x, playerPos.y, playerRadius, p.rect)) {
                    gameState = GAME_OVER;
                    break;
                }
            }

            // Check player projectile collision with enemies
            for (const auto& p : projectiles) {
                if (!p.isEnemy) {
                    auto beforeSize = enemies.size();
                    enemies.erase(
                        std::remove_if(enemies.begin(), enemies.end(),
                            [&p](const Enemy& e) {
                                return SDL_HasIntersection(&p.rect, &e.rect);
                            }
                        ),
                        enemies.end()
                    );
                    auto enemiesDestroyedThisFrame = beforeSize - enemies.size();
                    if (enemiesDestroyedThisFrame > 0) {
                        enemiesKilled += enemiesDestroyedThisFrame;
                        updateScoreDisplay();
                        
                        if (enemiesKilled >= enemiesRequiredToWin) {
                            gameState = WIN;
                        }
                    }
                }
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Changed background to black
        SDL_RenderClear(renderer);

        if (gameState == MENU) {
            SDL_RenderCopy(renderer, menuTexture, NULL, &menuRect);
        }
        else if (gameState == PLAYING) {
            // Render player as blue triangle
            DrawFilledTriangle(renderer, playerPos.x, playerPos.y, playerSize/2, playerAngle, 
                              0, 0, 255, 255); // Blue color (R=0, G=0, B=255)

            // Render enemies
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
            for (const auto& enemy : enemies) {
                DrawRotatedSquare(renderer, enemy.rect, enemy.angle);
            }

            // Render projectiles
            for (const auto& p : projectiles) {
                if (p.isEnemy) {
                    SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                }
                SDL_RenderFillRect(renderer, &p.rect);
            }

            // Render score
            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        }
        else if (gameState == GAME_OVER) {
            SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
        }
        else if (gameState == WIN) {
            SDL_RenderCopy(renderer, winTexture, NULL, &winRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(menuTexture);
    SDL_DestroyTexture(gameOverTexture);
    SDL_DestroyTexture(winTexture);
    SDL_DestroyTexture(scoreTexture);
    TTF_CloseFont(font);
    TTF_CloseFont(scoreFont);
    Mix_FreeChunk(pew);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

