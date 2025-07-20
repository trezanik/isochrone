#pragma once

/**
 * @file        src/secfuncs/Message.h
 * @brief       Messaging
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "definitions.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <Windows.h>


//#if SECFUNCS_LINK_PUGIXML


/*
 * Messaging system
 * 
 * Communication between isochrone and secfuncs deployed clients employs a
 * messaging structure to relay commands and responses.
 * 
 * Many commands are as simple as 'perform x' with a response 'success', however
 * this needs to scale for responses covering multiple megabytes.
 * 
 * Our lightweight-dependency stance remains in force, thus we have a custom
 * handler for executing commands and providing response notifications. This may
 * be subject to change based on in-the-wild results and discoveries. For now,
 * data transfer is compressed then base-64 encoded. Non-data transfer is raw.
 * 
 * Each message has a header and footer, and can not exceed 4096 bytes (including
 * the head and foot).
 * 
 * We also use named pipes (limiting general usage to Windows) so we don't need
 * to link and handle the winsock libraries. All we need is the Server service
 * to be running (in a default environment).
 */



constexpr size_t  message_size_max = 4096;
constexpr DWORD   pipe_timeout = 632;


constexpr uint32_t CT_CRC32_TABLE[] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
	0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
	0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
	0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};


constexpr uint32_t
ct_crc32_s4(
	char c,
	uint32_t h
)
{
	return (h >> 4) ^ CT_CRC32_TABLE[(h & 0xF) ^ c];
}


constexpr uint32_t
ct_crc32(
	const char* s,
	uint32_t h = ~0
)
{
	return !*s ? ~h : ct_crc32(s + 1, ct_crc32_s4(*s >> 4, ct_crc32_s4(*s & 0xF, h)));
}


constexpr uint32_t
compile_time_hash(
	const char* str
)
{
	return ct_crc32(str);
}




/*
 * All commands are hashes of the entries stated in DllWrapper, plus a few
 * special internal entries. Beyond these, always keep names in sync!
 */
const uint32_t  cmd_GetAutostarts = compile_time_hash("GetAutostarts");
const uint32_t  cmd_GetEvidenceOfExecution = compile_time_hash("GetEvidenceOfExecution");
const uint32_t  cmd_GetPowerShellInvokedCommandsForAll = compile_time_hash("GetPowerShellInvokedCommandsForAll");
const uint32_t  cmd_GetPowerShellInvokedCommandsForUser = compile_time_hash("GetPowerShellInvokedCommandsForUser");
const uint32_t  cmd_ReadAmCache = compile_time_hash("ReadAmCache");
const uint32_t  cmd_ReadAppCompatFlags = compile_time_hash("ReadAppCompatFlags");
const uint32_t  cmd_ReadBAM = compile_time_hash("ReadBAM");
const uint32_t  cmd_ReadChromiumDataForAll = compile_time_hash("ReadChromiumDataForAll");
const uint32_t  cmd_ReadChromiumDataForUser = compile_time_hash("ReadChromiumDataForUser");
const uint32_t  cmd_ReadPrefetch = compile_time_hash("ReadPrefetch");
const uint32_t  cmd_ReadUserAssist = compile_time_hash("ReadUserAssist");
// specials
const uint32_t  cmd_Restart = compile_time_hash("Restart");
const uint32_t  cmd_Shutdown = compile_time_hash("Shutdown");
const uint32_t  cmd_Logoff = compile_time_hash("Logoff");
const uint32_t  cmd_Tasklist = compile_time_hash("Tasklist");
const uint32_t  cmd_Kill = compile_time_hash("Kill");

const unsigned char  msg_sig_h[] = { 'M', 'S', 'G' };
const unsigned char  msg_sig_f[] = { 'E', 'M', 'S', 'G' };


// 8 bytes; 3 byte signature, 1 byte flags, 4 bytes offset
struct message_header
{
	uint8_t   b1 = 'M';
	uint8_t   b2 = 'S';
	uint8_t   b3 = 'G';
	unsigned  b4b1_command : 1;  // command: 1 = command, 0 = response
	unsigned  b4b2_state : 1;  // state: 1 = success, 0 = failure
	unsigned  b4b3_params : 1;  // message class: - 1 = custom, 0 = plain
	unsigned  b4b4_custom : 1;  // reserved; must be 0
	unsigned  b4b5 : 1;  // reserved; must be 0
	unsigned  b4b6 : 1;  // reserved; must be 0
	unsigned  b4b7 : 1;  // reserved; must be 0
	unsigned  b4b8 : 1;  // reserved; must be 0
	// uint8_t  b5_version;
	uint32_t  end_offset; // number of bytes to the footer, starting from the next byte (message start)
};

// 8 bytes; 4 byte reference, 4 byte signature
struct message_footer
{
	// uint32_t  hash;
	uint32_t  reference; // messaging reference for maintaining state
	uint32_t  sig;
#if 0
	uint8_t  b1 = 'E';
	uint8_t  b2 = 'M';
	uint8_t  b3 = 'S';
	uint8_t  b4 = 'G';
#endif
};


class CommandMessage;
class ResponseMessage;
class Message;

class MessageHandler
{
private:

	std::vector<std::shared_ptr<CommandMessage>>  my_cmd_messages;
	std::vector<std::shared_ptr<ResponseMessage>>  my_rsp_messages;

public:

	int
	ReceiveCommand(
		unsigned char* buf,
		size_t buf_size,
		std::shared_ptr<CommandMessage>& msg
	);
};


enum class MessageState
{
	Inactive,
	CommandSent,
	DataIncoming,
	DataReceived,
	Failed
};



class Message
{
private:

	message_header  header;
	message_footer  footer;
	SYSTEMTIME  systime;

protected:

public:
	Message operator = (Message const&) = delete;
	Message(Message const&) = delete;
	Message operator = (Message &&) = delete;
	Message(Message &&) = delete;

	Message(
		message_header* hdr,
		message_footer* ftr
	)
	{
		memcpy(&header, &hdr, sizeof(header));
		memcpy(&footer, &ftr, sizeof(footer));
		::GetSystemTime(&systime);
	}

	bool
	IsCommand()
	{
		return header.b4b1_command == 1;
	}

	bool
	IsResponse()
	{
		return header.b4b1_command == 0;
	}

	SYSTEMTIME
	ProcessTime()
	{
		return systime;
	}

	uint32_t
	Reference()
	{
		return footer.reference;
	}
};

class CommandMessage : public Message
{
private:

	uint32_t  my_command_hash;
	std::wstring  my_params;

public:
	CommandMessage operator = (CommandMessage const&) = delete;
	CommandMessage(CommandMessage const&) = delete;
	CommandMessage operator = (CommandMessage&&) = delete;
	CommandMessage(CommandMessage&&) = delete;

	CommandMessage(
		message_header* hdr,
		message_footer* ftr,
		uint32_t command_hash,
		std::wstring& params
	);

	uint32_t
	Command()
	{
		return my_command_hash;
	}

	std::wstring
	Parameters()
	{
		return my_params;
	}
};

class ResponseMessage : public Message
{
private:

	unsigned char  my_response[4096 - sizeof(message_header) - sizeof(message_footer)];
	size_t  my_response_size;

public:
	ResponseMessage operator = (ResponseMessage const&) = delete;
	ResponseMessage(ResponseMessage const&) = delete;
	ResponseMessage operator = (ResponseMessage&&) = delete;
	ResponseMessage(ResponseMessage&&) = delete;

	ResponseMessage(
		message_header* hdr,
		message_footer* ftr,
		unsigned char* response,
		size_t response_size
	);

	unsigned char*
	Response()
	{
		return my_response;
	}

	size_t
	ResponseSize()
	{
		return my_response_size;
	}
};


//#endif // SECFUNCS_LINK_PUGIXML
