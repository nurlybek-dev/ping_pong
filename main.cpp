#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#define TICK_INTERVAL 30
#define WIDTH 1024 
#define HEIGHT 768
#define PADDLE_SPEED 10
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 100
#define BALL_SPEED 10
#define BALL_ACCELERATED_SPEED 0.1
#define BALL_WIDTH 10
#define BALL_HEIGHT 10

static Uint32 next_time;
Uint32 time_left(void)
{
    Uint32 now;

    now = SDL_GetTicks();
    if(next_time <= now)
        return 0;
    else
        return next_time - now;
}

struct Vector2 {
    int x;
    int y;
};

enum class CollisionType
{
	None,
	Top,
	Middle,
	Bottom
};

struct Contact
{
	CollisionType type;
	float penetration;
};

SDL_Window *gWindow;
SDL_Renderer *gRenderer;
TTF_Font *gFont;

SDL_Texture *gPlayerOneScoreTexture;
SDL_Texture *gPlayerTwoScoreTexture;

SDL_Rect gPaddleOnePos = {10, HEIGHT/2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
SDL_Rect gPaddleTwoPos = {WIDTH - PADDLE_WIDTH - 10, HEIGHT/2 - PADDLE_HEIGHT / 2, PADDLE_WIDTH, PADDLE_HEIGHT};
SDL_Rect gBallPos = {WIDTH / 2 + BALL_WIDTH / 2, HEIGHT / 2 - BALL_HEIGHT / 2, BALL_WIDTH, BALL_HEIGHT};
SDL_Rect gPlayerOneScorePos = {WIDTH / 4, 20, 100, 64};
SDL_Rect gPlayerTwoScorePos = {3 * WIDTH / 4, 20, 0, 0};

Vector2 gPaddleOneDir;
Vector2 gPaddleTwoDir;
Vector2 gBallDir;

Mix_Chunk *gPaddleSound;
Mix_Chunk *gWallSound;

const SDL_Color WHITE = {255, 255, 255, 255};

int gPlayerOneScore = 0;
int gPlayerTwoScore = 0;

float gBallSpeed = BALL_SPEED;

void ClampPaddle(SDL_Rect &rect);
void ClampBall(SDL_Rect &rect);
Contact CheckPaddleCollision(SDL_Rect ball, SDL_Rect paddle);
void CollideWithPaddle(Contact const& contact);
void UpdateScore();

int main(int, char**)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT,2,2048);

    gWindow = SDL_CreateWindow("Ping Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

    gFont = TTF_OpenFont("assets/DejaVuSans.ttf", 40);

    gPaddleSound = Mix_LoadWAV("assets/paddle.wav");
    gWallSound = Mix_LoadWAV("assets/wall.wav");

    bool running = true;
    SDL_Event event;
    gBallDir.x = gBallSpeed;
    gBallDir.y = 0;
    UpdateScore();
    while(running)
    {
        while(SDL_PollEvent(&event) != 0)
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
            else if(event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_w:
                    gPaddleOneDir.y = -PADDLE_SPEED;
                    break;
                case SDLK_s:
                    gPaddleOneDir.y = PADDLE_SPEED;
                    break;
                case SDLK_UP:
                    gPaddleTwoDir.y = -PADDLE_SPEED;
                    break;
                case SDLK_DOWN:
                    gPaddleTwoDir.y = PADDLE_SPEED;
                    break;
                }
            }
            else if(event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_w:
                    if (gPaddleOneDir.y < 0) gPaddleOneDir.y = 0;
                    break;
                case SDLK_s:
                    if (gPaddleOneDir.y > 0) gPaddleOneDir.y = 0;
                    break;
                case SDLK_UP:
                    if (gPaddleTwoDir.y < 0) gPaddleTwoDir.y = 0;
                    break;
                case SDLK_DOWN:
                    if (gPaddleTwoDir.y > 0) gPaddleTwoDir.y = 0;
                    break;
                }
            }
        }

        gPaddleOnePos.y += gPaddleOneDir.y;
        ClampPaddle(gPaddleOnePos);

        gPaddleTwoPos.y += gPaddleTwoDir.y;
        ClampPaddle(gPaddleTwoPos);

        gBallPos.x += gBallDir.x;
        gBallPos.y += gBallDir.y;
        ClampBall(gBallPos);

        if (Contact contact = CheckPaddleCollision(gBallPos, gPaddleOnePos); contact.type != CollisionType::None)
        {
            CollideWithPaddle(contact);
        }
        else if (Contact contact = CheckPaddleCollision(gBallPos, gPaddleTwoPos); contact.type != CollisionType::None)
        {
            CollideWithPaddle(contact);
        }

        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
        SDL_RenderClear(gRenderer);

        SDL_RenderCopy(gRenderer, gPlayerOneScoreTexture, NULL, &gPlayerOneScorePos);
        SDL_RenderCopy(gRenderer, gPlayerTwoScoreTexture, NULL, &gPlayerTwoScorePos);

        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
        SDL_RenderFillRect(gRenderer, &gPaddleOnePos);
        SDL_RenderFillRect(gRenderer, &gPaddleTwoPos);
        SDL_RenderFillRect(gRenderer, &gBallPos);

        for (int y = 0; y < HEIGHT; ++y)
        {
            if (y % 3)
            {
                SDL_RenderDrawPoint(gRenderer, WIDTH / 2, y);
            }
        }

        SDL_RenderPresent(gRenderer);

        SDL_Delay(time_left());
        next_time += TICK_INTERVAL;
    }

    SDL_DestroyTexture(gPlayerTwoScoreTexture);
    SDL_DestroyTexture(gPlayerTwoScoreTexture);
    TTF_CloseFont(gFont);
    Mix_CloseAudio();
    Mix_FreeChunk(gPaddleSound);
    Mix_FreeChunk(gWallSound);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    return 0;
}

void ClampPaddle(SDL_Rect &rect)
{
    if(rect.x < 0) rect.x = 0;
    if(rect.x - rect.w > WIDTH) rect.x = WIDTH - rect.w;
    if(rect.y < 0) rect.y = 0;
    if(rect.y + rect.h > HEIGHT) rect.y = HEIGHT - rect.h;
}

void ClampBall(SDL_Rect &rect)
{
    if(rect.x < 0) {
        rect.x = 0;
        gBallSpeed = BALL_SPEED;
        gBallDir.x = gBallDir.x < 0 ? gBallSpeed : -gBallSpeed;
        gBallDir.y = 0;
        gBallPos.x = WIDTH / 2 + BALL_WIDTH / 2;
        gBallPos.y = HEIGHT / 2 - BALL_HEIGHT / 2;
        gPlayerTwoScore += 1;
        UpdateScore();
    }
    if(rect.x - rect.w > WIDTH) {
        rect.x = WIDTH - rect.w;
        gBallSpeed = BALL_SPEED;
        gBallDir.x = gBallDir.x < 0 ? gBallSpeed : -gBallSpeed;
        gBallDir.y = 0;
        gBallPos.x = WIDTH / 2 + BALL_WIDTH / 2;
        gBallPos.y = HEIGHT / 2 - BALL_HEIGHT / 2;
        gPlayerOneScore += 1;
        UpdateScore();
    }
    if(rect.y < 0) {
        rect.y = 0;
        gBallDir.y *= -1;
        Mix_PlayChannel(-1, gWallSound, 0);
    }
    if(rect.y + rect.h > HEIGHT) {
        rect.y = HEIGHT - rect.h;
        gBallDir.y *= -1;
        Mix_PlayChannel(-1, gWallSound, 0);
    }
}

Contact CheckPaddleCollision(SDL_Rect ball, SDL_Rect paddle)
{
	float ballLeft = ball.x;
	float ballRight = ball.x + BALL_WIDTH;
	float ballTop = ball.y;
	float ballBottom = ball.y + BALL_HEIGHT;

	float paddleLeft = paddle.x;
	float paddleRight = paddle.x + PADDLE_WIDTH;
	float paddleTop = paddle.y;
	float paddleBottom = paddle.y + PADDLE_HEIGHT;

	Contact contact{};

	if (ballLeft >= paddleRight)
	{
		return contact;
	}

	if (ballRight <= paddleLeft)
	{
		return contact;
	}

	if (ballTop >= paddleBottom)
	{
		return contact;
	}

	if (ballBottom <= paddleTop)
	{
		return contact;
	}

	float paddleRangeUpper = paddleBottom - (2.0f * PADDLE_HEIGHT / 3.0f);
	float paddleRangeMiddle = paddleBottom - (PADDLE_HEIGHT / 3.0f);

	if (gBallDir.x < 0)
	{
		// Left paddle
		contact.penetration = paddleRight - ballLeft;
	}
	else if (gBallDir.x > 0)
	{
		// Right paddle
		contact.penetration = paddleLeft - ballRight;
	}

	if ((ballBottom > paddleTop)
	    && (ballBottom < paddleRangeUpper))
	{
		contact.type = CollisionType::Top;
	}
	else if ((ballBottom > paddleRangeUpper)
	     && (ballBottom < paddleRangeMiddle))
	{
		contact.type = CollisionType::Middle;
	}
	else
	{
		contact.type = CollisionType::Bottom;
	}

	return contact;
}

void CollideWithPaddle(Contact const& contact)
{
    Mix_PlayChannel(-1, gPaddleSound, 0);
    gBallSpeed += BALL_ACCELERATED_SPEED;
    gBallPos.x += contact.penetration;
    gBallDir.x = gBallDir.x < 0 ? gBallSpeed : -gBallSpeed;

    if (contact.type == CollisionType::Top)
    {
        gBallDir.y = -.75f * gBallSpeed;
    }
    else if (contact.type == CollisionType::Bottom)
    {
        gBallDir.y = 0.75f * gBallSpeed;
    }
}

void UpdateScore()
{
    std::string score = std::to_string(gPlayerOneScore);
    SDL_Surface *playerOneSurface = TTF_RenderText_Solid(gFont, score.c_str(), {0xFF, 0xFF, 0xFF, 0xFF});
    gPlayerOneScoreTexture = SDL_CreateTextureFromSurface(gRenderer, playerOneSurface);
    int width, height;
    SDL_QueryTexture(gPlayerOneScoreTexture, nullptr, nullptr, &width, &height);
    gPlayerOneScorePos.w = width;
    gPlayerOneScorePos.h = height;

    score = std::to_string(gPlayerTwoScore);
    SDL_Surface *playerTwoSurface = TTF_RenderText_Solid(gFont, score.c_str(), {0xFF, 0xFF, 0xFF, 0xFF});
    gPlayerTwoScoreTexture = SDL_CreateTextureFromSurface(gRenderer, playerTwoSurface);
    SDL_QueryTexture(gPlayerTwoScoreTexture, nullptr, nullptr, &width, &height);
    gPlayerTwoScorePos.w = width;
    gPlayerTwoScorePos.h = height;

    SDL_FreeSurface(playerOneSurface);
    SDL_FreeSurface(playerTwoSurface);
}
