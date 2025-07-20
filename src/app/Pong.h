#pragma once

/**
 * @file        src/app/Pong.h
 * @brief       Basic version of Pong
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "engine/services/event/IEventListener.h"
#include "engine/services/event/EventData.h"
#include "engine/Context.h"
#include "engine/IFrameListener.h"

#include "core/services/log/LogLevel.h"
#include "core/util/filesystem/Path.h"
#include "core/util/SingularInstance.h"
#include "core/UUID.h"

#include <SDL.h>
#include <SDL_ttf.h>

#if TZK_IS_LINUX
#   include "app/undefs.h"
#endif


namespace trezanik {
namespace app {
namespace pong {


const uint8_t  ball_width = 15;
const uint8_t  ball_height = 15;
const float    ball_speed_start = 0.5f;
const float    ball_speed_max = 1.5f;
const uint8_t  paddle_width = 15;
const uint8_t  paddle_height = 120;
const float    paddle_speed = 1.f;
const float    paddle_offset = 50.f;
const float    text_offset = 20.f;


enum class CollisionType
{
	None,
	Top,
	Middle,
	Bottom,
	Left,
	Right
};

struct contact
{
	CollisionType type;
	float penetration;
};

struct pong_update
{
	float  delta_time;
	unsigned int  window_width;
	unsigned int  window_height;
};


class CollisionRect
{
public:
	float  left, top, right, bottom;


	CollisionRect() : left(0.f), top(0.f), right(0.f), bottom(0.f)
	{
	}

	bool
	Overlaps( //OrTouches
		CollisionRect& rhs
	)
	{
		if ( left <= rhs.right &&
		     right >= rhs.left &&
		     top >= rhs.bottom &&
		     bottom <= rhs.top )
		{
			return true;
		}

		return false;
	}
};


class Vector2D
{
public:
	Vector2D() : x(0.0f), y(0.0f)
	{
	}

	Vector2D(
		float x,
		float y
	)
	: x(x), y(y)
	{
	}

	Vector2D operator+(
		Vector2D const& rhs
	)
	{
		return Vector2D(x + rhs.x, y + rhs.y);
	}

	Vector2D& operator+=(
		Vector2D const& rhs
	)
	{
		x += rhs.x;
		y += rhs.y;

		return *this;
	}

	Vector2D operator*(float rhs)
	{
		return Vector2D(x * rhs, y * rhs);
	}

	float x, y;
};


class PlayerScore
{
public:
	PlayerScore(
		Vector2D position,
		SDL_Renderer* renderer,
		TTF_Font* font
	)
	: position(position)
	, score(0)
	, renderer(renderer)
	, font(font)
	, surface(nullptr)
	, texture(nullptr)
	{
		surface = TTF_RenderText_Solid(font, std::to_string(score).c_str(), { 0xFF, 0xFF, 0xFF, 0xFF });
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = width;
		rect.h = height;
	}

	~PlayerScore()
	{
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);
	}

	void
	Draw()
	{
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
	}

	void
	Scored()
	{
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);

		texture = nullptr;
		surface = nullptr;

		surface = TTF_RenderText_Solid(font, std::to_string(++score).c_str(), { 0xFF, 0xFF, 0xFF, 0xFF });
		texture = SDL_CreateTextureFromSurface(renderer, surface);

		int width, height;
		SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);

		rect.w = width;
		rect.h = height;
	}

	Vector2D  position;
	uint32_t  score;
	std::string  text;
	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Surface* surface;
	SDL_Texture* texture;
	SDL_Rect rect{};
};


class Paddle
{
public:
	Paddle(
		Vector2D position,
		Vector2D velocity
	)
	: position(position)
	, velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = paddle_width;
		rect.h = paddle_height;

		crect.left   = position.x;
		crect.top    = position.y;
		crect.right  = position.x + paddle_width;
		crect.bottom = position.y + paddle_height;
	}

	void
	Draw(
		SDL_Renderer* renderer
	)
	{
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	void
	Update(
		pong_update* update
	)
	{
		position += velocity * update->delta_time;

		if ( position.y < 0 )
		{
			// Restrict to top of the screen
			position.y = 0;
		}
		else if ( position.y > (update->window_height - paddle_height) )
		{
			// Restrict to bottom of the screen
			position.y = static_cast<float>(update->window_height - paddle_height);
		}

		// update collision
		crect.left   = position.x;
		crect.top    = position.y;
		crect.right  = position.x + paddle_width;
		crect.bottom = position.y + paddle_height;
	}

	Vector2D  position;
	Vector2D  velocity;
	CollisionRect  crect;
	SDL_Rect  rect{};
};


class Ball
{
public:
	Ball(
		Vector2D position,
		Vector2D velocity
	)
	: position(position)
	, velocity(velocity)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);
		rect.w = ball_width;
		rect.h = ball_height;

		speed = ball_speed_start;

		crect.left   = position.x;
		crect.top    = position.y;
		crect.right  = position.x + ball_width;
		crect.bottom = position.y + ball_height;
	}

	void
	Draw(
		SDL_Renderer* renderer
	)
	{
		rect.x = static_cast<int>(position.x);
		rect.y = static_cast<int>(position.y);

		SDL_RenderFillRect(renderer, &rect);
	}

	void
	CollideWithPaddle(
		contact const& contact
	)
	{
		position.x += contact.penetration;
		velocity.x = -velocity.x;

		if ( contact.type == CollisionType::Top )
		{
			float  randomness = -0.75f + (((float)(rand() % 3) / 3) - 0.8f);
			velocity.y = randomness * speed;
			//velocity.y = -0.75f * speed;
		}
		else if ( contact.type == CollisionType::Bottom )
		{
			float  randomness = 0.75f + (((float)(rand() % 3) / 3) - 0.8f);
			velocity.y = randomness * speed;
			//velocity.y = 0.75f * speed;
		}
	}

	void
	CollideWithWall(
		contact const& contact
	)
	{
		if ( (contact.type == CollisionType::Top)
			|| (contact.type == CollisionType::Bottom) )
		{
			// bounce off
			position.y += contact.penetration;
			velocity.y = -velocity.y;
		}
	}

	void
	IncreaseSpeed()
	{
		if ( speed >= ball_speed_max )
			return;

		speed += .1f;
		velocity.x += velocity.x > 0 ? .1f : -.1f;
		velocity.y += velocity.y > 0 ? .1f : -.1f;
	}


#if 0 // crap
	void
	AdjustForPaddleCollision(
		Paddle& pad_l,
		Paddle& pad_r,
		Vector2D& adjustment
	)
	{
		adjustment = Vector2D(0.f, 0.f);

		float paddle_split  = paddle_height / 3;
		float paddle_upper  = paddle_split * 1;
		float paddle_center = paddle_split * 2;
		float paddle_lower  = paddle_split * 3;

		if ( crect.Overlaps(pad_l.crect) )
		{
			// impact from the right
			if ( pad_l.crect.right > crect.left )
			{
				if ( (pad_l.crect.top + paddle_split) <= position.y )
				{
					// upper hit
					adjustment.x = -velocity.x;
					adjustment.y = -velocity.y;
				}
				else if ( (pad_l.crect.bottom - paddle_split) >= position.y )
				{
					// lower hit
					adjustment.x = -velocity.x;
					adjustment.y = velocity.y;
				}
				else
				{
					// central hit
					adjustment.x = -velocity.x;
					adjustment.y = 0.f;
				}
			}
		}
		else if ( crect.Overlaps(pad_r.crect) )
		{
			// impact from the left
			if ( pad_r.crect.left > crect.right )
			{
				if ( (pad_r.crect.top + paddle_split) <= position.y )
				{
					// upper hit
					adjustment.x = -velocity.x;
					adjustment.y = -velocity.y;
				}
				else if ( (pad_r.crect.bottom - paddle_split) >= position.y )
				{
					// lower hit
					adjustment.x = -velocity.x;
					adjustment.y = velocity.y;
				}
				else
				{
					// central hit
					adjustment.x = -velocity.x;
					adjustment.y = 0.f;
				}
			}
		}

		if ( adjustment.x != 0.f )
			velocity.x = adjustment.x;
		if ( adjustment.y != 0.f )
			velocity.y = adjustment.y;
	}
#endif

	void
	Update(
		pong_update* update
	)
	{
		position += velocity * update->delta_time;

		// update collision
		crect.left   = position.x;
		crect.top    = position.y;
		crect.right  = position.x + ball_width;
		crect.bottom = position.y + ball_height;
	}

	float     speed;
	Vector2D  position;
	Vector2D  velocity;
	CollisionRect  crect;
	SDL_Rect  rect{};
};


enum PongState : unsigned
{
	Idle = 0,    // ball is not in motion
	UpPressedL   = (1 << 0),
	UpPressedR   = (1 << 1),
	DownPressedL = (1 << 2),
	DownPressedR = (1 << 3),
	BallGoalL    = (1 << 4),
	BallGoalR    = (1 << 5),
	BallCollideL = (1 << 6),
	BallCollideR = (1 << 7),
	Active       = 1 << 8
};



/**
 * An implementation of the classic game, Pong
 *
 * Does actually serve a purpose here; we're using this as the first trial for
 * confirming SDL is functioning and rendering correctly, while also doing the
 * same for our Workspace concept.
 *
 * Once done, this is of course redundant, but the code will be left behind as
 * a future reminder aid, whilst also serving as a simple example.
 */
class Pong
	: private trezanik::core::SingularInstance<Pong>
	, public trezanik::engine::IEventListener
	, public trezanik::engine::IFrameListener
	, public trezanik::engine::IContextUpdate
{
	TZK_NO_CLASS_ASSIGNMENT(Pong);
	TZK_NO_CLASS_COPY(Pong);
	TZK_NO_CLASS_MOVEASSIGNMENT(Pong);
	TZK_NO_CLASS_MOVECOPY(Pong);

private:

	SDL_Renderer*  my_renderer;

	Ball    my_ball;
	Paddle  my_paddle1; // left
	Paddle  my_paddle2; // right

	unsigned int  my_state;

	uint32_t  my_height;  // available height
	uint32_t  my_width;   // available width

	uint64_t  my_game_start_time;  // used to increase ball speed between goals
	uint64_t  my_last_speed_increase;

	PlayerScore  my_score_p1;
	PlayerScore  my_score_p2;


	contact
	CheckPaddleCollision(
		Ball const& ball,
		Paddle const& paddle
	);


	contact
	CheckWallCollision(
		Ball const& ball
	);


	void
	HandleKeyboardChar(
		trezanik::engine::EventData::Input_KeyChar* inkc
	);


	void
	HandleKeyboardPress(
		trezanik::engine::EventData::Input_Key* ink
	);


	void
	HandleKeyboardRelease(
		trezanik::engine::EventData::Input_Key* ink
	);


	void
	HandleMouseButtonDown(
		trezanik::engine::EventData::Input_MouseButton* mbutton
	);


	void
	HandleMouseButtonUp(
		trezanik::engine::EventData::Input_MouseButton* mbutton
	);


	void
	HandleMouseMove(
		trezanik::engine::EventData::Input_MouseMove* input
	);


	void
	HandleWindowSize(
		trezanik::engine::EventData::System_WindowSize* wndsz
	);


	/**
	 * Implementation of IEventListener::ProcessEvent
	 */
	virtual int
	ProcessEvent(
		trezanik::engine::IEvent* event
	) override;


	/**
	 * Sends data to the SDL renderer for presentation
	 */
	void
	Render();

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] renderer
	 *  The SDL renderer that will present our content
	 * @param[in] font
	 *  The SDL (ttf) font for text rendering - the score
	 * @param[in] height
	 *  The available height of the game window
	 * @param[in] width
	 *  The available width of the game window
	 */
	Pong(
		SDL_Renderer* renderer,
		TTF_Font* font,
		uint32_t height,
		uint32_t width
	);


	/**
	 * Standard destructor
	 */
	virtual ~Pong();


	/**
	 * Implementation of IFrameListener::PostBegin
	 */
	virtual void
	PostBegin() override;


	/**
	 * Implementation of IFrameListener::PostEnd
	 */
	virtual void
	PostEnd() override;


	/**
	 * Implementation of IFrameListener::PreBegin
	 */
	virtual bool
	PreBegin() override;


	/**
	 * Implementation of IFrameListener::PreEnd
	 */
	virtual void
	PreEnd() override;


	/**
	 * @Implementation of IContextUpdate::Update
	 */
	virtual void
	Update(
		float delta_time
	) override;
};


} // namespace pong
} // namespace app
} // namespace trezanik
