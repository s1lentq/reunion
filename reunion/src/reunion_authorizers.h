#pragma once

#include "reunion_shared.h"

const int MAX_CLIENT_AUTHORIZERS = 32;

struct authdata_t
{
	// in
	uint8_t*		authTicket;
	size_t			ticketLen;

	int				protocol;
	char*			userinfo;
	char*			protinfo;
	uint32_t		ipaddr;
	uint32_t		port;

	// out
	int				idtype;
	uint32_t		steamId;

	auth_key_kind	authKeyKind;
	size_t			authKeyLen;
	char			authKey[MAX_AUTHKEY_LEN + 1];
};

class IClientAuthorizer {
public:
	virtual ~IClientAuthorizer() = default;
	virtual client_auth_kind authorize(authdata_t* authdata) = 0;
};

extern IClientAuthorizer* g_ClientAuthorizers[MAX_CLIENT_AUTHORIZERS];
extern int g_NumClientAuthorizers;

class CRevEmu2013Authorizer : public IClientAuthorizer {
public:
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CSteamClient2009Authorizer : public IClientAuthorizer {
public:
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CRevEmuAuthorizer : public IClientAuthorizer {
public:
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CSettiAuthorizer : public IClientAuthorizer {
public:
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CAVSMPAuthorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class COldRevEmuAuthorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CSteamEmuAuthorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CSXEIAuthorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CHLTVAuthorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CNoSteam47Authorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

class CNoSteam48Authorizer : public IClientAuthorizer {
	client_auth_kind authorize(authdata_t* authdata) override;
};

extern void Reunion_Init_Authorizers();
extern client_auth_kind Reunion_Authorize_Client(authdata_t* authdata);
extern const char* Reunion_GetAuthorizerName(client_auth_kind authKind);
