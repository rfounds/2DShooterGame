#include <SDL.h>
#include <vector>

struct Projectile {
    SDL_Rect rect;
    int dx, dy;
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("WASD + Shoot",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          640, 480,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool running = true;
    SDL_Event event;

    SDL_Rect player = { 300, 220, 40, 40 };
    const int speed = 5;

    int dirX = 0, dirY = -1;

    std::vector<Projectile> projectiles;

    while (running) {
        const Uint8* keystates = SDL_GetKeyboardState(NULL);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                Projectile p;
                p.rect.x = player.x + player.w / 2 - 5;
                p.rect.y = player.y + player.h / 2 - 5;
                p.rect.w = 10;
                p.rect.h = 10;
                p.dx = dirX * 10;
                p.dy = dirY * 10;
                projectiles.push_back(p);
            }
        }

        // Movement and direction tracking
        if (keystates[SDL_SCANCODE_W]) { player.y -= speed; dirX = 0; dirY = -1; }
        if (keystates[SDL_SCANCODE_S]) { player.y += speed; dirX = 0; dirY = 1; }
        if (keystates[SDL_SCANCODE_A]) { player.x -= speed; dirX = -1; dirY = 0; }
        if (keystates[SDL_SCANCODE_D]) { player.x += speed; dirX = 1; dirY = 0; }

        // Update projectiles
        for (auto& p : projectiles) {
            p.rect.x += p.dx;
            p.rect.y += p.dy;
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderClear(renderer);

        // Draw player
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &player);

        // Draw projectiles
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (auto& p : projectiles) {
            SDL_RenderFillRect(renderer, &p.rect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

