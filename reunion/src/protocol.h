#pragma once

#define	STEAM_MAX_PACKET_SIZE		1400
#define NETWORK_HEADERS_SIZE		42 // ip header + udp header

#define GAMESERVER_STEAMID			0x01	// not NULL

// Extra Data Flags
// https://developer.valvesoftware.com/wiki/Server_queries#Response_Format
#define EDF_FLAG_PORT				0x80	// The server's game port number
#define EDF_FLAG_STEAMID			0x10	// Server's SteamID
#define EDF_FLAG_SOURCE_TV			0x40	// Spectator port number and Name of spectator server for SourceTV
#define EDF_FLAG_GAME_TAGS			0x20	// Tags that describe the game according to the server
#define EDF_FLAG_GAMEID				0x1		// The server's 64-bit GameID. If this is present, a more accurate AppID is present in the low 24 bits.
											// The earlier AppID could have been truncated as it was forced into 16-bit storage.

#define CONNECTIONLESS_HEADER		-1
#define MULTIPACKET_HEADER			-2

//
#define PORT_MODMASTER				27011	// Default mod-master port, UDP
#define PORT_CLIENT					27005	// Default client port, UDP
#define PORT_SERVER					27015	// Default server port, UDP
#define PORT_RCON					27015	// Default RCON port, UDP

#define PROTOCOL_AUTHCERTIFICATE	0x01	// Connection from client is using a WON authenticated certificate
#define PROTOCOL_HASHEDCDKEY		0x02	// Connection from client is using hashed CD key because WON comm. channel was unreachable
#define PROTOCOL_STEAM				0x03	// Steam certificates
#define PROTOCOL_UNKNOWN			0x04	// Unknown protocol
#define PROTOCOL_LASTVALID			0x04	// Last valid protocol

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

// Client connection is initiated by requesting a challenge value
//  the server sends this value back
#define S2C_CHALLENGE			'A'	// + challenge value

// Response to source query
#define S2A_INFO				'I'

// Send a userid, client remote address, is this server secure and engine build number
#define S2C_CONNECTION			'B'

// HLMaster rejected a server's connection because the server needs to be updated
#define M2S_REQUESTRESTART		'O'

// Response details about each player on the server
#define S2A_PLAYERS				'D'

// Number of rules + string key and string value pairs
#define S2A_RULES				'E'

// info request
#define S2A_INFO_OLD			'C' // deprecated goldsrc response

// New Query protocol, returns dedicated or not, + other performance info.
#define S2A_INFO_DETAILED		'm'

// Send a log event as key value
#define S2A_LOGSTRING			'R'

// Send a log string
#define S2A_LOGKEY				'S'

// Basic information about the server
#define A2S_INFO				'T'

// Details about each player on the server
#define A2S_PLAYER				'U'

// The rules the server is using
#define A2S_RULES				'V'

// Another user is requesting a challenge value from this machine
#define A2A_GETCHALLENGE		'W'	// Request challenge # from another machine

// Generic Ping Request
#define A2A_PING				'i'	// respond with an A2A_ACK

// Generic Ack
#define A2A_ACK					'j'	// general acknowledgement without info

// Print to client console
#define A2A_PRINT				'l' // print a message on client

// Challenge response from master
#define M2A_CHALLENGE			's'	// + challenge value

typedef enum svc_commands_e
{
	svc_bad,
	svc_nop,
	svc_disconnect,
	svc_event,
	svc_version,
	svc_setview,
	svc_sound,
	svc_time,
	svc_print,
	svc_stufftext,
	svc_setangle,
	svc_serverinfo,
	svc_lightstyle,
	svc_updateuserinfo,
	svc_deltadescription,
	svc_clientdata,
	svc_stopsound,
	svc_pings,
	svc_particle,
	svc_damage,
	svc_spawnstatic,
	svc_event_reliable,
	svc_spawnbaseline,
	svc_temp_entity,
	svc_setpause,
	svc_signonnum,
	svc_centerprint,
	svc_killedmonster,
	svc_foundsecret,
	svc_spawnstaticsound,
	svc_intermission,
	svc_finale,
	svc_cdtrack,
	svc_restore,
	svc_cutscene,
	svc_weaponanim,
	svc_decalname,
	svc_roomtype,
	svc_addangle,
	svc_newusermsg,
	svc_packetentities,
	svc_deltapacketentities,
	svc_choke,
	svc_resourcelist,
	svc_newmovevars,
	svc_resourcerequest,
	svc_customization,
	svc_crosshairangle,
	svc_soundfade,
	svc_filetxferfailed,
	svc_hltv,
	svc_director,
	svc_voiceinit,
	svc_voicedata,
	svc_sendextrainfo,
	svc_timescale,
	svc_resourcelocation,
	svc_sendcvarvalue,
	svc_sendcvarvalue2,
	svc_startofusermessages = svc_sendcvarvalue2,
	svc_endoflist = 255,
} svc_commands_t;
