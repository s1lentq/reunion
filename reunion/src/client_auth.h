#pragma once

struct client_auth_context_t {
	int protocol					= 0;
	bool authentificatedInSteam		= false;
	bool unauthenticatedConnection	= false;
	unsigned char* authTicket		= nullptr;
	unsigned int authTicketLen		= 0;
	char* userinfo					= nullptr;
	char* protinfo					= nullptr;
	netadr_t* adr					= nullptr;
	uint32 ipaddr					= 0;
	int* pAuthProto					= nullptr;
};

extern client_auth_context_t* g_CurrentAuthContext;
extern bool Reunion_Auth_Init();

extern uint64 Steam_GSGetSteamID();
