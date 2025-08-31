#pragma once

/**
 * @file        src/engine/services/NullServices.h
 * @brief       Null service implementations for engine service classes
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "common_definitions.h"

#include "engine/services/audio/IAudio.h"
#include "engine/services/net/INet.h"
#include "core/error.h"


namespace trezanik {
namespace engine {


/**
 * Default class for an audio interface implementation
 */
class TZK_ENGINE_API NullAudio : public IAudio
{
private:
protected:
public:
	/**
	 * Implementation of IAudio::CreateSound
	 */
	virtual std::shared_ptr<ALSound>
	CreateSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> TZK_UNUSED(res)
	) override
	{
		return nullptr;
	}


	/**
	 * Implementation of IAudio::FindSound
	 */
	virtual std::shared_ptr<ALSound>
	FindSound(
		std::shared_ptr<trezanik::engine::Resource_Audio> TZK_UNUSED(res)
	) override
	{
		return nullptr;
	}


	/**
	 * Implementation of IAudio::GetAllOutputDevices
	 */
	virtual std::vector<std::string>
	GetAllOutputDevices() const override
	{
		return std::vector<std::string>();
	}


	/**
	 * Implementation of IAudio::GetFiletype
	 */
	virtual AudioFileType
	GetFiletype(
		FILE* TZK_UNUSED(fp)
	) override
	{
		return AudioFileType::Invalid;
	}


	/**
	 * Implementation of IAudio::GetFiletype
	 */
	virtual AudioFileType
	GetFiletype(
		const std::string& TZK_UNUSED(fpath)
	) override
	{
		return AudioFileType::Invalid;
	}


	/**
	 * Implementation of IAudio::GlobalPause
	 */
	virtual void
	GlobalPause() override
	{
	}


	/**
	 * Implementation of IAudio::GlobalResume
	 */
	virtual void
	GlobalResume() override
	{
	}


	/**
	 * Implementation of IAudio::GlobalStop
	 */
	virtual void
	GlobalStop() override
	{
	}


	/**
	 * Implementation of IAudio::Initialize
	 */
	virtual int
	Initialize() override
	{
		return ErrIMPL;
	}


	/**
	 * Implementation of IAudio::SetSoundGain
	 */
	virtual void
	SetSoundGain(
		float TZK_UNUSED(effects),
		float TZK_UNUSED(music)
	) override
	{
	}


	/**
	 * Implementation of IAudio::Update
	 */
	virtual void
	Update(
		float TZK_UNUSED(delta_time)
	) override
	{
	}


	/**
	 * Implementation of IAudio::UseSound
	 */
	virtual int
	UseSound(
		std::shared_ptr<AudioComponent> TZK_UNUSED(emitter),
		std::shared_ptr<ALSound> TZK_UNUSED(sound),
		uint8_t TZK_UNUSED(priority)
	) override
	{
		return 0;
	}
};


/**
 * Default class for a network interface implementation
 */
class NullNet : public net::INet
{
private:
protected:
public:
	
	virtual std::shared_ptr<net::HTTPSession>
	CreateHTTPSession(
		net::URI TZK_UNUSED(uri)
	) override
	{
		return nullptr;
	}


	virtual std::shared_ptr<net::HTTPRequest>
	GetHTTPRequest(
		trezanik::core::UUID& TZK_UNUSED(id)
	) override
	{
		return nullptr;
	}


	virtual std::shared_ptr<net::HTTPResponse>
	GetHTTPResponse(
		trezanik::core::UUID& TZK_UNUSED(id)
	) override
	{
		return nullptr;
	}


	virtual std::shared_ptr<net::HTTPSession>
	GetHTTPSession(
		trezanik::core::UUID& TZK_UNUSED(id)
	) override
	{
		return nullptr;
	}


	virtual int
	Initialize() override
	{
		return ErrIMPL;
	}


	virtual std::shared_ptr<net::HTTPResponse>
	IssueHTTPRequest(
		std::shared_ptr<net::HTTPSession> TZK_UNUSED(session),
		std::shared_ptr<net::HTTPRequest> TZK_UNUSED(request)
	) override
	{
		return nullptr;
	}


	virtual void
	Terminate() override
	{
	}
};


} // namespace engine
} // namespace trezanik
