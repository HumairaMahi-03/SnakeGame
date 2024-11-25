#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define SCREEN_WIDTH 700
#define SCREEN_HEIGHT 600
#define BLOCK_DIMENSION 20
#define INITIAL_SPEED 200

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position body[SCREEN_WIDTH * SCREEN_HEIGHT / (BLOCK_DIMENSION * BLOCK_DIMENSION)];
    int length;
    Position movement;
} SnakeGame;

typedef struct {
    Position location;
    int isActive;
} RegularFood;

typedef struct {
    Position location;
    int isActive;
    Uint32 spawnTime;  // To track how long the poisonous food has been active
} PoisonFood;

typedef struct {
    Position location;
    int isActive;
} BonusFood;

SDL_Texture *load_asset(SDL_Renderer *renderer, const char *filePath) {
    SDL_Surface *image = IMG_Load(filePath);
    if (!image) {
        printf("Image load failed: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, image);
    SDL_FreeSurface(image);
    return texture;
}

void display_text(SDL_Renderer *renderer, TTF_Font *font, const char *message, SDL_Color textColor, int x, int y) {
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, message, textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect renderQuad = {x, y, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void initialize_game(SnakeGame *snake, RegularFood *food, BonusFood *bonus, PoisonFood *poison, int *score, int *speed, int *foodConsumed) {
    snake->length = 2;
    snake->movement = (Position){1, 0};
    for (int i = 0; i < snake->length; i++) {
        snake->body[i] = (Position){snake->length - i - 1, 0};
    }
    food->location = (Position){rand() % (SCREEN_WIDTH / BLOCK_DIMENSION), rand() % (SCREEN_HEIGHT / BLOCK_DIMENSION)};
    food->isActive = 1;
    bonus->isActive = 0;
    poison->isActive = 0;
    *score = 0;
    *speed = INITIAL_SPEED;
    *foodConsumed = 0;
}

int check_self_collision(SnakeGame *snake) {
    for (int i = 1; i < snake->length; i++) {
        if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
            return 1;
        }
    }
    return 0;
}

int check_border_collision(SnakeGame *snake) {
    if (snake->body[0].x < 0 || snake->body[0].x >= SCREEN_WIDTH / BLOCK_DIMENSION || snake->body[0].y < 0 || snake->body[0].y >= SCREEN_HEIGHT / BLOCK_DIMENSION) {
        return 1;
    }
    return 0;
}

void update_snake(SnakeGame *snake) {
    for (int i = snake->length - 1; i > 0; i--) {
        snake->body[i] = snake->body[i - 1];
    }
    snake->body[0].x += snake->movement.x;
    snake->body[0].y += snake->movement.y;
}

int spawn_new_food(RegularFood *food, BonusFood *bonus, PoisonFood *poison, SnakeGame *snake, int *foodConsumed) {
    while (1) {
        food->location.x = rand() % (SCREEN_WIDTH / BLOCK_DIMENSION);
        food->location.y = rand() % (SCREEN_HEIGHT / BLOCK_DIMENSION);
        if (!check_self_collision(snake)) {
            break;
        }
    }

    if (*foodConsumed >= 4) {
        poison->location = (Position){rand() % (SCREEN_WIDTH / BLOCK_DIMENSION), rand() % (SCREEN_HEIGHT / BLOCK_DIMENSION)};
        poison->isActive = 1;
        poison->spawnTime = SDL_GetTicks();  // Set the spawn time for the poisonous food
       
    }

    if (*foodConsumed >= 5) {
        bonus->location = (Position){rand() % (SCREEN_WIDTH / BLOCK_DIMENSION), rand() % (SCREEN_HEIGHT / BLOCK_DIMENSION)};
        bonus->isActive = 1;
        *foodConsumed = 0;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || TTF_Init() == -1 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        return 1;
    }

    SDL_Window *gameWindow = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *gameRenderer = SDL_CreateRenderer(gameWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!gameWindow || !gameRenderer) {
        return 1;
    }

    TTF_Font *gameFont = TTF_OpenFont("arial.ttf", 30);
    Mix_Chunk *foodSound = Mix_LoadWAV("foodsound.mp3");
    Mix_Music *backgroundMusic = Mix_LoadMUS("snakesound.mp3");
    SDL_Texture *foodImage = load_asset(gameRenderer, "food.png");
    SDL_Texture *snakeImage = load_asset(gameRenderer, "snake.png");
    SDL_Texture *backgroundImage = load_asset(gameRenderer, "background4_0snake.png");
    SDL_Texture *bonusFoodImage = load_asset(gameRenderer, "BonusFood3.jpg");
    SDL_Texture *poisonFoodImage = load_asset(gameRenderer, "applebody.jpg");

    if (!gameFont || !foodSound || !backgroundMusic || !foodImage || !snakeImage || !backgroundImage || !bonusFoodImage || !poisonFoodImage) {
        return 1;
    }

    srand(time(NULL));

    SnakeGame snake;
    RegularFood regularFood;
    BonusFood bonusFood;
    PoisonFood poisonFood;
    int score = 0, gameSpeed = INITIAL_SPEED, foodCount = 0;

    initialize_game(&snake, &regularFood, &bonusFood, &poisonFood, &score, &gameSpeed, &foodCount);

    bool isRunning = 1, isGameOver = 0;
    SDL_Event gameEvent;

    Mix_PlayMusic(backgroundMusic, -1);

    while (isRunning) {
        while (SDL_PollEvent(&gameEvent)) {
            if (gameEvent.type == SDL_QUIT) {
                isRunning = 0;
            }
            if (gameEvent.type == SDL_KEYDOWN) {
                switch (gameEvent.key.keysym.sym) {
                    case SDLK_UP:
                        if (snake.movement.y == 0) snake.movement = (Position){0, -1};
                        break;
                    case SDLK_DOWN:
                        if (snake.movement.y == 0) snake.movement = (Position){0, 1};
                        break;
                    case SDLK_LEFT:
                        if (snake.movement.x == 0) snake.movement = (Position){-1, 0};
                        break;
                    case SDLK_RIGHT:
                        if (snake.movement.x == 0) snake.movement = (Position){1, 0};
                        break;
                    case SDLK_r:
                        if (isGameOver) {
                            initialize_game(&snake, &regularFood, &bonusFood, &poisonFood, &score, &gameSpeed, &foodCount);
                            isGameOver = 0;
                        }
                        break;
                }
            }
        }

        if (!isGameOver) {
            update_snake(&snake);

            if (check_border_collision(&snake) || check_self_collision(&snake)) {
                isGameOver = 1;
            }

            // Regular food consumption
            if (snake.body[0].x == regularFood.location.x && snake.body[0].y == regularFood.location.y) {
                snake.length++;
                score+=10;
                foodCount++;
                spawn_new_food(&regularFood, &bonusFood, &poisonFood, &snake, &foodCount);
                Mix_PlayChannel(-1, foodSound, 0);
                if (gameSpeed > 50) gameSpeed -= 5;
            }

            // Poisonous food consumption
            if (poisonFood.isActive && snake.body[0].x == poisonFood.location.x && snake.body[0].y == poisonFood.location.y) {
                score -= 10;
                if (score < 0) {
                    isGameOver = 1;
                }
                poisonFood.isActive = 0;
            }

            // Bonus food consumption
            if (bonusFood.isActive && snake.body[0].x == bonusFood.location.x && snake.body[0].y == bonusFood.location.y) {
                score += 50;
                bonusFood.isActive = 0;
            }

            // Check if poisonous food time is up (4 seconds)
            if (poisonFood.isActive && SDL_GetTicks() - poisonFood.spawnTime > 4000) {
                poisonFood.isActive = 0;
            }
        }

        SDL_SetRenderDrawColor(gameRenderer, 0, 0, 0, 255);
        SDL_RenderClear(gameRenderer);

        // Render background
        SDL_Rect backgroundRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderCopy(gameRenderer, backgroundImage, NULL, &backgroundRect);

        // Render regular food
        SDL_Rect foodRect = {regularFood.location.x * BLOCK_DIMENSION, regularFood.location.y * BLOCK_DIMENSION, BLOCK_DIMENSION, BLOCK_DIMENSION};
        SDL_RenderCopy(gameRenderer, foodImage, NULL, &foodRect);

        // Render poisonous food
        if (poisonFood.isActive) {
            SDL_Rect poisonFoodRect = {poisonFood.location.x * BLOCK_DIMENSION, poisonFood.location.y * BLOCK_DIMENSION, BLOCK_DIMENSION, BLOCK_DIMENSION};
            SDL_RenderCopy(gameRenderer, poisonFoodImage, NULL, &poisonFoodRect);
        }

        // Render bonus food
        if (bonusFood.isActive) {
            SDL_Rect bonusFoodRect = {bonusFood.location.x * BLOCK_DIMENSION, bonusFood.location.y * BLOCK_DIMENSION, BLOCK_DIMENSION, BLOCK_DIMENSION};
            SDL_RenderCopy(gameRenderer, bonusFoodImage, NULL, &bonusFoodRect);
        }

        // Render snake
        for (int i = 0; i < snake.length; i++) {
            SDL_Rect snakeRect = {snake.body[i].x * BLOCK_DIMENSION, snake.body[i].y * BLOCK_DIMENSION, BLOCK_DIMENSION, BLOCK_DIMENSION};
            SDL_RenderCopy(gameRenderer, snakeImage, NULL, &snakeRect);
        }

        // Render score
        char scoreText[32];
        sprintf(scoreText, "Score: %d", score);
        display_text(gameRenderer, gameFont, scoreText, (SDL_Color){255, 255, 255, 255}, 10, 10);

        // Game over screen
        if (isGameOver) {
            display_text(gameRenderer, gameFont, "Game Over!", (SDL_Color){255, 0, 0, 255}, SCREEN_WIDTH / 2 -30, SCREEN_HEIGHT / 2 -50);
            char finalScore[32];
            sprintf(finalScore, "Score: %d", score);
            display_text(gameRenderer, gameFont, finalScore, (SDL_Color){255, 255, 255, 255}, SCREEN_WIDTH / 2 - 35, SCREEN_HEIGHT / 2);
            display_text(gameRenderer, gameFont, "Press 'R' to Restart", (SDL_Color){255, 255, 255, 255}, SCREEN_WIDTH / 2 - 115, SCREEN_HEIGHT / 2 + 40);
        }

        SDL_RenderPresent(gameRenderer);
        SDL_Delay(isGameOver ? 300 : gameSpeed); // Delay to control speed
    }

    // Cleanup resources
    SDL_DestroyTexture(foodImage);
    SDL_DestroyTexture(snakeImage);
    SDL_DestroyTexture(backgroundImage);
    SDL_DestroyTexture(bonusFoodImage);
    SDL_DestroyTexture(poisonFoodImage);
    Mix_FreeChunk(foodSound);
    Mix_FreeMusic(backgroundMusic);
    TTF_CloseFont(gameFont);
    SDL_DestroyRenderer(gameRenderer);
    SDL_DestroyWindow(gameWindow);
    Mix_CloseAudio();
    SDL_Quit();
    TTF_Quit();
    return 0;
}