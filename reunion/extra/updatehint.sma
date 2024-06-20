/*  AMXModX Script
*
*   Title:    Update Client Hint
*   Author:   Lev/Crock
*
*   Changelog:
*
*   23.03.2010
*   - Added HLTV recognition
*
*   04.08.2010
*   - Added new emulators support (AVSMP, SC2009)
*
*   19.10.2010
*   - Fixed AVSMP and SteamClient2009 support
*
*   07.08.2013
*   - Added sXeI support
*   - Added RevEmu2013 support
*
*/

#pragma semicolon 1
#pragma ctrlchar '\'

#include <amxmodx>
#include <amxmisc>
#include <amxconst>
#include <fun>
#include <regex>

#define DP_AUTH_NONE 		0	// "N/A" - slot is free
#define DP_AUTH_DPROTO		1	// dproto
#define DP_AUTH_STEAM		2	// Native Steam
#define DP_AUTH_STEAMEMU	3	// SteamEmu
#define DP_AUTH_REVEMU		4	// RevEmu
#define DP_AUTH_OLDREVEMU	5	// Old RevEmu
#define DP_AUTH_HLTV		6	// HLTV
#define DP_AUTH_SC2009		7	// SteamClient 2009
#define DP_AUTH_AVSMP		8	// AVSMP
#define DP_AUTH_SXEI		9	// sXe Injected
#define DP_AUTH_REVEMU2013	10	// RevEmu 2013

new const PLUGIN[]  = "UpdateHint";
new const VERSION[] = "1.3";
new const AUTHOR[]  = "Lev";

const BASE_TASK_ID_HINT = 3677;		// random number
const BASE_TASK_ID_KICK = 6724;		// random number
const MIN_SHOW_INTERVAL = 20;		// minimum constrain for hint show interval
const MAX_URL_LENGTH = 70;		// max length of the URL

new bool:playerPutOrAuth[33];		// Player was put in server or auth.

new pcvar_uh_url;
new pcvar_uh_interval;
new pcvar_uh_kickinterval;
new pcvar_dp_r_protocol;
new pcvar_dp_r_id_provider;

public plugin_init()
{
	register_plugin(PLUGIN, VERSION, AUTHOR);
	register_cvar("updatehint", VERSION, FCVAR_SERVER | FCVAR_SPONLY | FCVAR_UNLOGGED);

	register_dictionary("updatehint.txt");

	pcvar_uh_url = register_cvar("uh_url", "http://some.addr/somefile");	// URL where player can goto to download new client.
	pcvar_uh_interval = register_cvar("uh_interval", "60.0");		// Interval between hint shows.
	pcvar_uh_kickinterval = register_cvar("uh_kickinterval", "0");		// Interval bwfoew kick client.
	pcvar_dp_r_protocol = get_cvar_pointer ("dp_r_protocol");		// Reunion interface.
	pcvar_dp_r_id_provider = get_cvar_pointer ("dp_r_id_provider");		// Reunion interface.
}

public client_connect(id)
{
	playerPutOrAuth[id] = false;
}

public client_authorized(id)
{
	if (playerPutOrAuth[id])
	{
		return check_client_type(id);
	}
	playerPutOrAuth[id] = true;
	return PLUGIN_CONTINUE;
}

public client_putinserver(id)
{
	if (playerPutOrAuth[id])
	{
		return check_client_type(id);
	}
	playerPutOrAuth[id] = true;
	return PLUGIN_CONTINUE;
}

stock NeedShowUpdateMsg(proto, authprov)
{
	if (authprov == DP_AUTH_HLTV)
		return false;

	if (proto < 48)
		return true;

	if (authprov == DP_AUTH_STEAM ||
		authprov == DP_AUTH_REVEMU ||
		authprov == DP_AUTH_SC2009 ||
		authprov == DP_AUTH_AVSMP ||
		authprov == DP_AUTH_SXEI ||
		authprov == DP_AUTH_REVEMU2013)
		return false;

	return true;
}

check_client_type(id)
{
	if (!pcvar_dp_r_protocol || !pcvar_dp_r_id_provider)
		return PLUGIN_CONTINUE;

	server_cmd("dp_clientinfo %d", id);
	server_exec();

	new proto = get_pcvar_num(pcvar_dp_r_protocol);
	new authprov = get_pcvar_num(pcvar_dp_r_id_provider);

	switch (authprov)
	{
		case DP_AUTH_DPROTO:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "DPROTO");
		case DP_AUTH_STEAM:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "STEAM");
		case DP_AUTH_REVEMU:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "REVEMU");
		case DP_AUTH_STEAMEMU:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "STEAMEMU");
		case DP_AUTH_OLDREVEMU:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "OLDREVEMU");
		case DP_AUTH_HLTV:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "HLTV");
		case DP_AUTH_SC2009:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "SteamClient2009/revEmu");
		case DP_AUTH_AVSMP:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "AVSMP");
		case DP_AUTH_SXEI:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "SXEI");
		case DP_AUTH_REVEMU2013:
			console_print(0, "Protocol: %d, authprovider: %s", proto, "REVEMU2013");
	}

	if (NeedShowUpdateMsg(proto, authprov))
	{
		set_task(get_uh_interval(), "show_update_hint", BASE_TASK_ID_HINT + id, _, _, "b");
		new kick_interval = get_pcvar_num(pcvar_uh_kickinterval);
		if (kick_interval > 0)
			set_task(float(kick_interval), "kick_client", BASE_TASK_ID_KICK + id);
	}

	return PLUGIN_CONTINUE;
}

public client_disconnect(id)
{
	remove_task(BASE_TASK_ID_HINT + id);
	remove_task(BASE_TASK_ID_KICK + id);
}

Float:get_uh_interval()
{
	new interval = get_pcvar_num(pcvar_uh_interval);
	// Check to be no less then minimum value
	return float((interval < MIN_SHOW_INTERVAL ) ? MIN_SHOW_INTERVAL : interval);
}

public show_update_hint(id)
{
	id -= BASE_TASK_ID_HINT;

	if (0 > id || id > 31)
		return;

	new url[MAX_URL_LENGTH];
	get_pcvar_string(pcvar_uh_url, url, charsmax(url));
	set_hudmessage(255, 100, 100, -1.0, 0.35, 0, 3.0, 5.0, 0.1, 0.1, 4);
	show_hudmessage(id, "%L", id, "HUDHINT");
	client_print(id, print_chat, "%L", id, "CHATHINT", url);
}

public kick_client(id)
{
	id -= BASE_TASK_ID_KICK;

	if (0 > id || id > 31)
		return;

	new url[MAX_URL_LENGTH];
	get_pcvar_string(pcvar_uh_url, url, charsmax(url));
	client_print(id, print_chat, "%L", id, "CHATHINT", url);
	new userid = get_user_userid(id);
	server_cmd("kick #%d \"%L\"", userid, id, "HUDHINT");
}
