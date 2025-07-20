/**
 * @file        src/secfuncs/Message.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "Message.h"
#include "DllWrapper.h"



int
MessageHandler::ReceiveCommand(
	unsigned char* buf,
	size_t buf_size,
	std::shared_ptr<CommandMessage>& msg
)
{
	if ( memcmp(buf, msg_sig_h, sizeof(msg_sig_h)) != 0 )
	{
		return -1;
	}

	message_header*  hdr = (message_header*)&buf;

	if ( hdr->b4b1_command == 0 )
	{
		// command required
		return -1;
	}
	if ( hdr->b4b5 != 0 || hdr->b4b6 != 0 || hdr->b4b7 != 0 || hdr->b4b8 != 0 )
	{
		// unused bits must be 0
		return -1;
	}
	if ( hdr->end_offset > (buf_size - sizeof(message_footer)) || hdr->end_offset == 0 )
	{
		// invalid offset
		return -1;
	}

	message_footer*  ftr = (message_footer*)&buf + hdr->end_offset;

	if ( memcmp(&ftr->sig, msg_sig_f, sizeof(msg_sig_f)) != 0 )
	{
		return -1;
	}

	/* 
	 * If we reach here, we have a dataset with a valid header and footer in
	 * the right locations, in a thus far decent format.
	 */

	uint32_t  reference = 0;
	uint32_t  command_hash = 0;
	std::wstring  params;

	memcpy(&command_hash, &buf + sizeof(hdr), sizeof(command_hash));
	memcpy(&reference, &ftr->reference, sizeof(reference));

	if ( reference == 0 )
	{
		// client must supply a reference we can sync with
		return -1;
	}

	if ( hdr->b4b3_params == 1 )
	{
		// params present - unicode string, UTF-16, nul-terminated
		//std::vector<wchar_t>  wchars;
		unsigned char*  start = buf + sizeof(hdr) + sizeof(command_hash);
		unsigned char*  end = (unsigned char*)ftr - 1;

		if ( end >= (unsigned char*)ftr || start == end )
		{
			// invalid
		}

		params.assign((wchar_t*)start, end - start);
	}


	// finally, this is valid if we have a matching command
	switch ( command_hash )
	{
	case cmd_GetAutostarts:
	case cmd_GetEvidenceOfExecution:
	case cmd_GetPowerShellInvokedCommandsForAll:
	case cmd_GetPowerShellInvokedCommandsForUser:
	case cmd_Kill:
	case cmd_Logoff:
	case cmd_ReadAmCache:
	case cmd_ReadAppCompatFlags:
	case cmd_ReadBAM:
	case cmd_ReadChromiumDataForAll:
	case cmd_ReadChromiumDataForUser:
	case cmd_ReadPrefetch:
	case cmd_ReadUserAssist:
	case cmd_Restart:
	case cmd_Shutdown:
	case cmd_Tasklist:
		break;
	default:
		::DebugBreak();
		return -1;
	}


	std::shared_ptr<CommandMessage>  cmdmsg = std::make_shared<CommandMessage>(
		hdr, ftr, command_hash, params
	);

	my_cmd_messages.push_back(cmdmsg);
	msg = cmdmsg;

	return 0;
}





CommandMessage::CommandMessage(
	message_header* hdr,
	message_footer* ftr,
	uint32_t command_hash,
	std::wstring& params
)
: Message(hdr, ftr)
, my_command_hash(command_hash)
, my_params(params)
{
	
}


ResponseMessage::ResponseMessage(
	message_header* hdr,
	message_footer* ftr,
	unsigned char* response,
	size_t response_size
)
: Message(hdr, ftr)
, my_response_size(response_size)
{
	memcpy(&my_response, response, response_size);
}
