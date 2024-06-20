/*
	This sample plugin shows how to get information about client's protocol and Steam ID. (plugin will write this info when client connecting)
	
*/

#include <amxmodx>
#include <amxmisc>


#define DP_AUTH_NONE 		0
#define DP_AUTH_DPROTO		1
#define DP_AUTH_STEAM		2
#define DP_AUTH_STEAMEMU	3
#define DP_AUTH_REVEMU		4
#define DP_AUTH_OLDREVEMU	5
#define DP_AUTH_HLTV		6
#define DP_AUTH_SC2009		7
#define DP_AUTH_AVSMP		8
#define DP_AUTH_SXEI		9
#define DP_AUTH_REVEMU2013	10


//
// pointers to dp_r_protocol and dp_r_id_provider cvars
// reunion will store information in these cvars

new pcv_dp_r_protocol
new pcv_dp_r_id_provider

public plugin_init()
{
	register_plugin("reunion testing", "1", "")

	//
	// Initialize cvar pointers
	//
	pcv_dp_r_protocol = get_cvar_pointer ("dp_r_protocol")
	pcv_dp_r_id_provider = get_cvar_pointer ("dp_r_id_provider")
	
}

public client_connect(id)
{
	if (!pcv_dp_r_protocol || !pcv_dp_r_id_provider)
	{
		log_amx ("cant find dp_r_protocol or dp_r_id_provider cvars")  
		return PLUGIN_HANDLED
	}

	/*
		The "dp_clientinfo" command are exported by reunion. The syntax is:
			dp_clientinfo <id>
		where id is slot index (1 to 32)
		After executing this command reunion will set dp_r_protocol and dp_r_id_provider cvars.
		The dp_r_protocol keeps client's protocol
		The dp_r_id_provider cvar will be set to:
			1:  if client's steam id assigned by reunion (player uses no-steam client without emulator)
			2:  if client's steam id assigned by native steam library (or by another soft that emulates this library, server-side revEmu for example)
			3:  if client's steam id assigned by reunion's SteamEmu emulator
			4:  if client's steam id assigned by reunion's revEmu emulator
			5:  if client's steam id assigned by reunion's old revEmu emulator
			6:  if client is HLTV
			7:  if client's steam id assigned by reunion's SC2009 emulator
			8:  if client's steam id assigned by reunion's AVSMP emulator
			9:  if client's steam id assigned by SXEI's *HID userinfo field
			10:  if client's steam id assigned by reunion's revEmu2013 emulator

		If slot is empty, both dp_r_protocol and dp_r_id_provider cvars will be set to 0
		If slot is invalid (id < 1 or  id > max players), both dp_r_protocol and dp_r_id_provider cvars will be set to -1
	*/

	//
	// add command to queue
	//
	server_cmd("dp_clientinfo %d", id)

	//
	// make server to execute all queued commands
	//
	server_exec()

	//
	// now parse cvar values
	// 
	new proto = get_pcvar_num(pcv_dp_r_protocol)
	new authprov = get_pcvar_num(pcv_dp_r_id_provider)
	new auth_prov_str[32]
	new user_name[33]

	switch (authprov)
	{
		case DP_AUTH_NONE: copy(auth_prov_str, 32, "N/A") //slot is free
		case DP_AUTH_DPROTO: copy(auth_prov_str, 32, "dproto")
		case DP_AUTH_STEAM: copy(auth_prov_str, 32, "Steam(Native)")
		case DP_AUTH_STEAMEMU: copy(auth_prov_str, 32, "SteamEmu")
		case DP_AUTH_REVEMU: copy(auth_prov_str, 32, "revEmu")
		case DP_AUTH_OLDREVEMU: copy(auth_prov_str, 32, "old revEmu")
		case DP_AUTH_HLTV: copy(auth_prov_str, 32, "HLTV")	
		case DP_AUTH_SC2009: copy(auth_prov_str, 32, "SteamClient2009")
		case DP_AUTH_AVSMP: copy(auth_prov_str, 32, "AVSMP")
		case DP_AUTH_SXEI: copy(auth_prov_str, 32, "SXEI")
		case DP_AUTH_REVEMU2013: copy(auth_prov_str, 32, "RevEmu2013")
		default: copy(auth_prov_str, 32, "Erroneous") //-1 if slot id is invalid
	}

	get_user_name (id, user_name, 33) 

	server_print("User %s (%d) uses protocol %d; SteamID assigned by %s", user_name, id, proto, auth_prov_str)

	return PLUGIN_HANDLED
}
