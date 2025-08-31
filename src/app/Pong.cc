/**
 * @file        src/app/Pong.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/Pong.h"

#include "engine/services/event/EngineEvent.h"
#include "engine/services/ServiceLocator.h"

#include "core/services/event/EventDispatcher.h"
#include "core/services/log/Log.h"
#include "core/util/time.h"

#if TZK_USING_SDLIMAGE
#   include <SDL_image.h>
#endif


namespace trezanik {
namespace app {
namespace pong {


Pong::Pong(
	SDL_Renderer* renderer,
	TTF_Font* font,
	uint32_t height,
	uint32_t width
)
: my_renderer(renderer)
, my_ball(Vector2D((width / 2.f) - (ball_width / 2.f), (height / 2.f) - (ball_height / 2.f)), Vector2D(0.f, 0.f))
, my_paddle1(Vector2D(paddle_offset, (height / 2.f) - (paddle_height / 2.f)), Vector2D(0.f, 0.f))
, my_paddle2(Vector2D(width - paddle_offset, (height / 2.f) - (paddle_height / 2.f)), Vector2D(0.f, 0.f))
, my_state(PongState::Idle)
, my_height(height)
, my_width(width)
, my_game_start_time(0)
, my_last_speed_increase(0)
, my_score_p1(Vector2D(static_cast<float>(width / 4), text_offset), renderer, font)
, my_score_p2(Vector2D(static_cast<float>(3 * (width / 4)), text_offset), renderer, font)
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::key_char>>(uuid_keychar, std::bind(&Pong::HandleKeyboardChar, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::key_press>>(uuid_keydown, std::bind(&Pong::HandleKeyboardPress, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::key_press>>(uuid_keyup, std::bind(&Pong::HandleKeyboardRelease, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::mouse_button>>(uuid_mousedown, std::bind(&Pong::HandleMouseButtonDown, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::mouse_button>>(uuid_mouseup, std::bind(&Pong::HandleMouseButtonUp, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::mouse_move>>(uuid_mousemove, std::bind(&Pong::HandleMouseMove, this, std::placeholders::_1))));
		my_reg_ids.emplace(evtmgr->Register(std::make_shared<core::Event<engine::EventData::window_size>>(uuid_windowsize, std::bind(&Pong::HandleWindowSize, this, std::placeholders::_1))));


		// pong will not perform text processing; suppress KeyChar events. To move with better evt mgmt
		SDL_StopTextInput();

		engine::Context::GetSingleton().AddUpdateListener(this);
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


Pong::~Pong()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		engine::Context::GetSingleton().RemoveUpdateListener(this);

		SDL_StartTextInput();
	
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


contact
Pong::CheckPaddleCollision(
	Ball const& ball,
	Paddle const& paddle
)
{
	float ball_left = ball.position.x;
	float ball_right = ball.position.x + ball_width;
	float ball_top = ball.position.y;
	float ball_bottom = ball.position.y + ball_height;

	float paddle_left = paddle.position.x;
	float paddle_right = paddle.position.x + paddle_width;
	float paddle_top = paddle.position.y;
	float paddle_bottom = paddle.position.y + paddle_height;

	contact  ball_contact{};

	if ( ball_left >= paddle_right )
	{
		return ball_contact;
	}

	if ( ball_right <= paddle_left )
	{
		return ball_contact;
	}

	if ( ball_top >= paddle_bottom )
	{
		return ball_contact;
	}

	if ( ball_bottom <= paddle_top )
	{
		return ball_contact;
	}

	float paddle_range_upper = paddle_bottom - (2.0f * paddle_height / 3.0f);
	float paddle_range_middle = paddle_bottom - (paddle_height / 3.0f);

	if ( ball.velocity.x < 0 )
	{
		// left paddle
		ball_contact.penetration = paddle_right - ball_left;
	}
	else if ( ball.velocity.x > 0 )
	{
		// right paddle
		ball_contact.penetration = paddle_left - ball_right;
	}

	if ( (ball_bottom > paddle_top)
		&& (ball_bottom < paddle_range_upper) )
	{
		ball_contact.type = CollisionType::Top;
	}
	else if ( (ball_bottom > paddle_range_upper)
		&& (ball_bottom < paddle_range_middle) )
	{
		ball_contact.type = CollisionType::Middle;
	}
	else
	{
		ball_contact.type = CollisionType::Bottom;
	}

	return ball_contact;
}


contact
Pong::CheckWallCollision(
	Ball const& ball
)
{
	float ball_left = ball.position.x;
	float ball_right = ball.position.x + ball_width;
	float ball_top = ball.position.y;
	float ball_bottom = ball.position.y + ball_height;

	contact ball_contact{};

	if ( ball_left < 0.0f )
	{
		ball_contact.type = CollisionType::Left;
	}
	else if ( ball_right > my_width )
	{
		ball_contact.type = CollisionType::Right;
	}
	else if ( ball_top < 0.0f )
	{
		ball_contact.type = CollisionType::Top;
		ball_contact.penetration = -ball_top;
	}
	else if ( ball_bottom > my_height )
	{
		ball_contact.type = CollisionType::Bottom;
		ball_contact.penetration = my_height - ball_bottom;
	}

	return ball_contact;
}


void
Pong::HandleKeyboardChar(
	trezanik::engine::EventData::key_char TZK_UNUSED(inkc)
)
{
	using namespace trezanik::core;

	//TZK_LOG_FORMAT(LogLevel::Trace, "KeyChar: %s", inkc->utf8);
}


void
Pong::HandleKeyboardPress(
	trezanik::engine::EventData::key_press ink
)
{
	using namespace trezanik::engine;
	
	if ( ink.key == Key::Key_W )
		my_state |= PongState::UpPressedL;
	if ( ink.key == Key::Key_S )
		my_state |= PongState::DownPressedL;

	if ( ink.key == Key::Key_UpArrow )
		my_state |= PongState::UpPressedR;
	if ( ink.key == Key::Key_DownArrow )
		my_state |= PongState::DownPressedR;

	if ( ink.key == Key::Key_R )
	{
		// reset ball position, retaining velocity
		my_ball.position.x = (my_width / 2.f) - (ball_width / 2.f);
		my_ball.position.y = (my_height / 2.f) - (ball_height / 2.f);
	}
	if ( ink.key == Key::Key_C )
	{
		// reset ball position, with velocity center-on to player 2
		my_ball.position.x = (my_width / 2.f) - (ball_width / 2.f);
		my_ball.position.y = (my_height / 2.f) - (ball_height / 2.f);
		my_ball.velocity.x = ball_speed_start;
		my_ball.velocity.y = 0.f;
	}

	if ( ink.key == Key::Key_Space && my_ball.velocity.x == 0 && my_ball.velocity.y == 0 )
	{
		// start
		my_state = PongState::Active;
		my_game_start_time = core::aux::get_ms_since_epoch();
		my_last_speed_increase = my_game_start_time;
#if 0
		// boring straight to the right
		my_ball.velocity.x = ball_speed;
#else
		// random left<->right and up<->down
		my_ball.velocity.x = (rand() % 2 >= 1) ? ball_speed_start : -ball_speed_start;
		// take off 0.5f leaving a start of -0.5f through +0.5f
		my_ball.velocity.y = ((float)(rand() % 10) / 10) - 0.5f;
#endif
	}
}


void
Pong::HandleKeyboardRelease(
	trezanik::engine::EventData::key_press ink
)
{
	using namespace trezanik::engine;

	if ( ink.key == Key::Key_W )
		my_state &= ~PongState::UpPressedL;
	if ( ink.key == Key::Key_S )
		my_state &= ~PongState::DownPressedL;

	if ( ink.key == Key::Key_UpArrow )
		my_state &= ~PongState::UpPressedR;
	if ( ink.key == Key::Key_DownArrow )
		my_state &= ~PongState::DownPressedR;
}


void
Pong::HandleMouseButtonDown(
	trezanik::engine::EventData::mouse_button TZK_UNUSED(input)
)
{
	using namespace trezanik::core;

	//TZK_LOG_FORMAT(LogLevel::Trace, "MButton Down: %u", input.button);
}


void
Pong::HandleMouseButtonUp(
	trezanik::engine::EventData::mouse_button TZK_UNUSED(input)
)
{
	using namespace trezanik::core;

	//TZK_LOG_FORMAT(LogLevel::Trace, "MButton Up: %u", input.button);
}


void
Pong::HandleMouseMove(
	trezanik::engine::EventData::mouse_move input
)
{
	using namespace trezanik::core;

#if TZK_MOUSEMOVE_LOGS
	TZK_LOG_FORMAT(LogLevel::Trace,
		"MouseMove Pos=%d,%d, Rel=%d,%d", 
		input.pos_x, input.pos_y, input.rel_x, input.rel_y
	);
#endif

	TZK_CC_DISABLE_WARNING(-Wunused-value)

	// handle as needed
	input;

	TZK_CC_RESTORE_WARNING
}


void
Pong::HandleWindowSize(
	trezanik::engine::EventData::window_size wndsz
)
{
	using namespace trezanik::core;

	// remember, this is the *entire* window size, not just our 'allocation'
	TZK_LOG_FORMAT(LogLevel::Trace, "New Window Size: %ux%u", wndsz.width, wndsz.height);

	my_height = wndsz.height;
	my_width = wndsz.width;
}


void
Pong::PostBegin()
{
	// no-op
}


void
Pong::PostEnd()
{
	Render();
}


bool
Pong::PreBegin()
{
	// always permit
	return true;
}


void
Pong::PreEnd()
{
	// no-op
}


void
Pong::Render()
{
	// net
	{
		// use white
		SDL_SetRenderDrawColor(my_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		// gapped points from centerpoint top to bottom
		for ( unsigned int y = 0; y < my_height; ++y )
		{
			if ( y % 5 )
			{
				SDL_RenderDrawPoint(my_renderer, my_width / 2, y);
				// use renderdrawpoints() to save calculating each frame
			}
		}
	}
	// ball
	{
		my_ball.Draw(my_renderer);
	}
	// paddles
	{
		my_paddle1.Draw(my_renderer);
		my_paddle2.Draw(my_renderer);
	}
	// scores
	{
		my_score_p1.Draw();
		my_score_p2.Draw();
	}
}


void
Pong::Update(
	float delta_time
)
{
	using namespace trezanik::core;

	pong_update  pu;
	Vector2D     adj{};

	pu.delta_time = delta_time;
	pu.window_height = my_height;
	pu.window_width = my_width;

	if ( my_state & PongState::BallGoalL ||
		 my_state & PongState::BallGoalR )
	{
		// reset ball and paddles
		my_ball.position.x = (my_width / 2.f) - (ball_width / 2.f);
		my_ball.position.y = (my_height / 2.f) - (ball_height / 2.f);
		my_ball.velocity.x = 0.f;
		my_ball.velocity.y = 0.f;
		my_paddle1.position.x = paddle_offset;
		my_paddle1.position.y = (my_height / 2.f) - (paddle_height / 2.f);
		my_paddle1.velocity.x = 0.f;
		my_paddle1.velocity.y = 0.f;
		my_paddle2.position.x = my_width - paddle_offset;
		my_paddle2.position.y = (my_height / 2.f) - (paddle_height / 2.f);
		my_paddle2.velocity.x = 0.f;
		my_paddle2.velocity.y = 0.f;

		// normal state, await activation keypress
		my_state = PongState::Idle;
	}

	{
		if ( my_state & PongState::UpPressedL )
			my_paddle1.velocity.y = -paddle_speed;
		else if ( my_state & PongState::DownPressedL )
			my_paddle1.velocity.y = paddle_speed;
		else
			my_paddle1.velocity.y = 0;

		if ( my_state & PongState::UpPressedR )
			my_paddle2.velocity.y = -paddle_speed;
		else if ( my_state & PongState::DownPressedR )
			my_paddle2.velocity.y = paddle_speed;
		else
			my_paddle2.velocity.y = 0;
	}

	my_paddle1.Update(&pu);
	my_paddle2.Update(&pu);
	my_ball.Update(&pu);

	contact ball_contact = CheckPaddleCollision(my_ball, my_paddle1);

	if ( ball_contact.type != CollisionType::None )
	{
		my_ball.CollideWithPaddle(ball_contact);
	}
	else
	{
		ball_contact = CheckPaddleCollision(my_ball, my_paddle2);
	
		if ( ball_contact.type != CollisionType::None )
		{
			my_ball.CollideWithPaddle(ball_contact);
		}
	}
	
	ball_contact = CheckWallCollision(my_ball);

	if ( ball_contact.type != CollisionType::None )
	{
		my_ball.CollideWithWall(ball_contact);

		if ( ball_contact.type == CollisionType::Left )
		{
			TZK_LOG(LogLevel::Info, "Player 2 scored");

			my_state = PongState::BallGoalL;
			my_ball.velocity.x = 0.f;
			my_ball.velocity.y = 0.f;
			my_score_p2.Scored();
		}
		else if ( ball_contact.type == CollisionType::Right )
		{
			TZK_LOG(LogLevel::Info, "Player 1 scored");

			my_state = PongState::BallGoalR;
			my_ball.velocity.x = 0.f;
			my_ball.velocity.y = 0.f;
			my_score_p1.Scored();
		}
	}


	// challenge bump, increase ball speed
	if ( my_state & PongState::Active )
	{
		uint64_t  cur_time = aux::get_ms_since_epoch();
		uint64_t  diff = (cur_time - my_last_speed_increase);

		// every 15 seconds, increase the ball speed
		if ( diff > 15000 )
		{
			TZK_LOG(LogLevel::Info, "Increasing ball speed");
			my_ball.IncreaseSpeed();
			my_last_speed_increase = cur_time;
		}
	}
}


} // namespace pong
} // namespace app
} // namespace treanik
