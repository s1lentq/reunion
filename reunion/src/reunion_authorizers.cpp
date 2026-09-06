#include "precompiled.h"

#define IDREVHEADER	('r' << 16) + ('e' << 8) + 'v' // little-endian "rev"

enum
{
	REVEMU_SIGNATURE = IDREVHEADER,
	REVEMU_SIGNATURE2 = 0
};

enum
{
	REVEMU_KEY_LEN = 32,
	REVEMU_BLOCK_SIZE = 32
};

const char *g_AuthorizerName[] =
{
	"UNKNOWN",
	"HLTV",
	"NoSteam47",			// No auth data was provided by client
	"NoSteam48",			// No auth data was provided by client
	"Setti",				// Setti bot
	"Steam",				// Authorized by steam
	"Steam_PENDING",		// Authorized by steam, but ID is STEAM_ID_PENDING
	"SteamEmu",				// SteamEmu authorization (authorization by serial number of first HDD volume in the system)
	"OldRevemu",			// Old RevEmu (authorization by serial number of first HDD volume in the system)
	"RevEmu",				// RevEmu (authorization by serial number of first HDD in the system)
	"SC2009",				// SteamClient (alias for revemu) (authorization by serial number of first HDD in the system)
	"RevEmu2013",			// RevEmu (authorization by serial number of first HDD in the system)
	"AVSMP",				// Steam emulator, transmits steamid from Steam without any security/encryption, so it may be easily spoofed
	"sXe",					// Authorization by sXe Injected anticheat ID
	"SSE3",					// SmartSteamEmu
};

IClientAuthorizer* g_ClientAuthorizers[MAX_CLIENT_AUTHORIZERS];
int g_NumClientAuthorizers = 0;
const char *g_RevEmuCryptKey = "_YOU_SERIOUSLY_NEED_TO_GET_LAID_";
const uint32_t g_SteamEmuHashKey = 0xC9710266;

static uint32_t revHash(const char* str, int n = -1)
{
	uint32_t hash = 0x4E67C6A7;

	for (int cc = *str; cc && n != 0; cc = *++str, --n) {
		hash ^= (hash >> 2) + cc + 32 * hash;
	}

	return hash;
}

size_t Reunion_AuthKeyMaxLen(authdata_t* authdata)
{
	// deprecated auth version reunion2015/reunion2018 has a truncated ticket buffer
	const uint32_t MAX_RAWAUTHDATA_2015_TRUNCATED = 16;
	const uint32_t MAX_RAWAUTHDATA_2018_TRUNCATED = 31;

	switch (g_ReunionConfig->getAuthVersion())
	{
	case av_reunion2015:
		return min(authdata->authKeyLen, MAX_RAWAUTHDATA_2015_TRUNCATED);
	case av_reunion2018:
		return min(authdata->authKeyLen, MAX_RAWAUTHDATA_2018_TRUNCATED);
	default:
		break;
	}

	return authdata->authKeyLen;
}

void RevEmuFinishAuthorization(authdata_t* authdata, const char* authStr, size_t authKeyMaxLen, bool stripSpecialChars)
{
	uint32_t volumeId;
	char hddsn[256];

	if (IsHddsnNumber(authStr)) {
		authdata->authKeyKind = AK_HDDSN;

		if (stripSpecialChars) {
			authdata->authKeyLen = strecpy(hddsn, authStr, authKeyMaxLen, " \\/-");
			authStr = hddsn;
		}
		else
			authdata->authKeyLen = min(strlen(authStr), authKeyMaxLen);

		LCPrintf(false, "RevEmu raw auth string: '%s' (HDDSN)%s\n", authStr,
			IsValidHddsnNumber(authStr, authdata->authKeyLen) ? "" : " (INVALID)"
		);
	}
	else {
		authdata->authKeyKind = AK_VOLUMEID;
		LCPrintf(false, "RevEmu raw auth string: '%s' (VolumeID)\n", authStr);

		if (g_ReunionConfig->getAuthVersion() >= av_reunion2018) {
			volumeId = strtoul(authStr, nullptr, 10) & 0x7FFFFFFF;
			authdata->authKeyLen = volumeId ? sizeof(volumeId) : 0; // can't be zero
			authStr = (char *)&volumeId;
		}
		else
			authdata->authKeyLen = min(strlen(authStr), authKeyMaxLen);
	}

	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (authdata->authKeyLen) {
		memcpy(authdata->authKey, authStr, authdata->authKeyLen);
		authdata->authKey[authdata->authKeyLen] = '\0';

		authdata->steamId = revHash(authdata->authKey) << 1;

		if (authStr == (char *)&volumeId)
			LCPrintf(false, "RevEmu auth key: '%u' steamid: %u\n", volumeId, authdata->steamId);
		else
			LCPrintf(false, "RevEmu auth key: '%s' steamid: %u\n", authStr, authdata->steamId);
	}
	else {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		LCPrintf(false, "RevEmu authorized with pending id\n");
	}
}

template<typename ticket_t>
bool RevEmu2009to2013Authorize(ticket_t* ticket, authdata_t* authdata)
{
	enum
	{
		VERSION = 0x53,
	};

	if (ticket->Version != VERSION || ticket->RevSignature != REVEMU_SIGNATURE || ticket->Signature2 != REVEMU_SIGNATURE2)
		return CA_UNKNOWN;

	CRijndael hCrypt1;
	hCrypt1.MakeKey(g_RevEmuCryptKey, CRijndael::sm_chain0, REVEMU_KEY_LEN, REVEMU_BLOCK_SIZE);

	char DecryptedAuthKey[REVEMU_KEY_LEN];
	if (!hCrypt1.DecryptBlock(ticket->EncryptedKey, DecryptedAuthKey))
		return false;

	CRijndael hCrypt2;
	hCrypt2.MakeKey(DecryptedAuthKey, CRijndael::sm_chain0, REVEMU_KEY_LEN, REVEMU_BLOCK_SIZE);

	char DecryptedAuthData[REVEMU_BLOCK_SIZE + 1];
	if (!hCrypt2.DecryptBlock(ticket->EncryptedData, DecryptedAuthData))
		return false;

	sha2 hSha;
	hSha.Init(sha2::enuSHA256);
	hSha.Update((sha_byte *)DecryptedAuthData, SHA256_DIGESTC_LENGTH);
	hSha.End();

	DecryptedAuthData[REVEMU_BLOCK_SIZE] = '\0';

	int shaLen;
	const char *cDigest = hSha.RawHash(shaLen);
	if (memcmp(cDigest, ticket->Digest, SHA256_DIGESTC_LENGTH) != 0)
		return false;

	uint32_t hash = revHash(DecryptedAuthData);
	uint32_t accid = ticket->SteamID.GetAccountID();

	if (hash != ticket->Hash || hash << 1 != accid) {
		return false;
	}

	RevEmuFinishAuthorization(authdata, DecryptedAuthData, sizeof DecryptedAuthData - 1, g_ReunionConfig->isSC2009RevEmuCompat() || g_ReunionConfig->getAuthVersion() >= av_reunion2018);

	return true;
}

client_auth_kind CRevEmu2013Authorizer::authorize(authdata_t* authdata)
{
	struct RevEmu2013Ticket_t
	{
		uint32_t Version;
		uint32_t Hash;
		uint32_t RevSignature;
		uint32_t Signature2;
		CSteamID SteamID;
		uint32_t ExpireTime;
		uint32_t NotCreationTime;
		uint32_t Unk20;
		uint32_t Unk24;
		char EncryptedData[REVEMU_BLOCK_SIZE];
		char EncryptedKey[REVEMU_BLOCK_SIZE];
		char Digest[SHA256_DIGESTC_LENGTH];
	};

	if (authdata->ticketLen < sizeof(RevEmu2013Ticket_t))
		return CA_UNKNOWN;

	return RevEmu2009to2013Authorize((RevEmu2013Ticket_t *)authdata->authTicket, authdata) ? CA_REVEMU_2013 : CA_UNKNOWN;
}

client_auth_kind CSteamClient2009Authorizer::authorize(authdata_t* authdata)
{
	enum
	{
		KEY_LEN = 32,
		BLOCK_SIZE = 32
	};

	struct SC2009Ticket_t
	{
		uint32_t Version;
		uint32_t Hash;
		uint32_t RevSignature;
		uint32_t Signature2;
		CSteamID SteamID;
		char EncryptedData[REVEMU_BLOCK_SIZE];
		char EncryptedKey[REVEMU_BLOCK_SIZE];
		char Digest[SHA256_DIGESTC_LENGTH];
	};

	if (authdata->ticketLen < sizeof(SC2009Ticket_t))
		return CA_UNKNOWN;

	return RevEmu2009to2013Authorize((SC2009Ticket_t *)authdata->authTicket, authdata) ? CA_STEAMCLIENT_2009 : CA_UNKNOWN;
}

client_auth_kind CRevEmuAuthorizer::authorize(authdata_t* authdata)
{
	enum
	{
		VERSION			= 0x4A,
		TICKET_BUF_SIZE	= 128
	};

	struct RevTicket_t
	{
		uint32_t Version;
		uint32_t Hash;
		uint32_t RevSignature;
		uint32_t Signature2;
		CSteamID SteamID;
		char TicketBuf[TICKET_BUF_SIZE];
	};

	if (authdata->ticketLen < sizeof(RevTicket_t)) {
		return CA_UNKNOWN;
	}

	RevTicket_t* ticket = (RevTicket_t *)authdata->authTicket;
	if (ticket->Version != VERSION || ticket->RevSignature != REVEMU_SIGNATURE || ticket->Signature2 != REVEMU_SIGNATURE2) {
		return CA_UNKNOWN;
	}

	char c = ticket->TicketBuf[sizeof ticket->TicketBuf - 1];		// keep value for revert
	ticket->TicketBuf[sizeof ticket->TicketBuf - 1] = '\0';

	uint32_t hash = revHash(ticket->TicketBuf);
	uint32_t accid = ticket->SteamID.GetAccountID();
	if (hash != ticket->Hash || hash << 1 != accid) {
		ticket->TicketBuf[sizeof ticket->TicketBuf - 1] = c;		// restore key
		return CA_UNKNOWN;
	}

	RevEmuFinishAuthorization(authdata, ticket->TicketBuf, sizeof ticket->TicketBuf - 1, g_ReunionConfig->getAuthVersion() >= av_reunion2018);

	return CA_REVEMU;
}

client_auth_kind CSettiAuthorizer::authorize(authdata_t* authdata)
{
	enum
	{
		TICKET_LEN = 0x300,
		SETTI_KEY1 = 0xD4CA7F7B,
		SETTI_KEY2 = 0xC7DB6023,
		SETTI_KEY3 = 0x6D6A2E1F,
		SETTI_KEY4 = 0xB4C43105,
	};

	uint32_t *ikey = (uint32_t *)authdata->authTicket;
	if (authdata->ticketLen != TICKET_LEN) {
		return CA_UNKNOWN;
	}

	// thx to vityan666
	if (!(ikey[0] == SETTI_KEY1 || ikey[1] == SETTI_KEY2 || ikey[2] == SETTI_KEY3 || ikey[5] == SETTI_KEY4)) {
		return CA_UNKNOWN;
	}

	authdata->idtype = AUTH_IDTYPE_LOCAL;
	authdata->steamId = STEAM_ID_LAN;

	authdata->authKeyKind = AK_OTHER;
	authdata->authKeyLen = 0;

	return CA_SETTI;
}

client_auth_kind CAVSMPAuthorizer::authorize(authdata_t* authdata)
{
	enum
	{
		VERSION = 0x14,
		PENDING_ID = 1234
	};

	struct AvsmpTicket_t
	{
		uint32_t Version;
		uint32_t Unk04;
		uint32_t Unk08;
		CSteamID SteamID;
		uint32_t UnkA0;
		uint32_t UnkA4;
	};

	static_assert(offsetof(AvsmpTicket_t, SteamID) == 12 && sizeof(AvsmpTicket_t) == 28, "wrong SteamID offset");

	if (authdata->ticketLen != sizeof(AvsmpTicket_t)) {
		return CA_UNKNOWN;
	}

	AvsmpTicket_t* ticket = (AvsmpTicket_t *)authdata->authTicket;
	if (ticket->Version != VERSION) {
		return CA_UNKNOWN;
	}

	uint32_t accId = ticket->SteamID.GetAccountID();
	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (accId == STEAM_ID_LAN || accId == PENDING_ID) {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		authdata->authKeyLen = 0;
	}
	else {
		authdata->steamId = accId;
		authdata->authKeyKind = AK_FILEID; // file
		authdata->authKeyLen = sizeof(uint32_t);
		*(uint32_t *)authdata->authKey = accId;
	}

	return CA_AVSMP;
}

client_auth_kind CSSE3Authorizer::authorize(authdata_t* authdata)
{
	enum
	{
		HEADER = 'UMEH',
		MAGIC = ' ESS',
		MIN_METHOD = 315,
		DATA_SIZE = 32,
		MAX_DATA_SIZE = 0x800,
		PENDING_ID = 1234,
		TIME_TOLERANCE = 57600, // +-16 hours of clock skew between client and server is tolerated
		SSE3_KEY_LEN = 16,
		SSE3_BLOCK_SIZE = 16,
	};

	struct SSE3Ticket_t
	{
		uint32_t Header;		// +0,  'UMEH'
		int32_t Method;			// +4,  ticket format revision, dproto/reunion just want it to be >= MIN_METHOD
		uint32_t IpAddr;		// +8,  client's external IP
		uint32_t Unk0C;			// +12
		uint32_t Unk10;			// +16
		uint32_t Unk14;			// +20
		uint64_t KeyMaterial;	// +24, client-chosen seed; first 16 bytes of SHA256(KeyMaterial) is the AES-128 key below
		uint32_t Unk20;			// +32
		uint32_t Unk24;			// +36
		uint32_t Unk28;			// +40
		uint32_t Unk2C;			// +44
		CSteamID SteamID;		// +48, plaintext copy of the SteamID
		uint32_t DataLen;		// +56, length of the encrypted part below, must be DATA_SIZE

		// Encrypted with a modified-Rijndael AES-128/CBC (zero IV) using SHA256(KeyMaterial)[0:16] as the key
		uint32_t Random;		// +60
		uint32_t Magic;			// +64, must decrypt to MAGIC
		CSteamID EncSteamID;	// +68, must match the plaintext SteamID above
		uint32_t IpAddr2;		// +76, must match either the connecting IP or a private/loopback range
		uint32_t Unused;		// +80
		int64_t GenTime;		// +84, ticket generation time (time_t), checked against TIME_TOLERANCE
	};

	static_assert(offsetof(SSE3Ticket_t, KeyMaterial) == 24 && offsetof(SSE3Ticket_t, DataLen) == 56 && offsetof(SSE3Ticket_t, Random) == 60, "wrong SSE3 ticket layout");

	if (authdata->ticketLen < offsetof(SSE3Ticket_t, Random)) {
		return CA_UNKNOWN;
	}

	SSE3Ticket_t* ticket = (SSE3Ticket_t *)authdata->authTicket;
	if (ticket->Header != HEADER || ticket->Method < MIN_METHOD) {
		return CA_UNKNOWN;
	}

	if (ticket->DataLen != DATA_SIZE || ticket->DataLen > MAX_DATA_SIZE
		|| authdata->ticketLen < ticket->DataLen + offsetof(SSE3Ticket_t, Random)) {
		return CA_UNKNOWN;
	}

	sha2 hSha;
	hSha.Init(sha2::enuSHA256);
	hSha.Update((sha_byte *)&ticket->KeyMaterial, sizeof(ticket->KeyMaterial));
	hSha.End();

	int shaLen;
	const char *cDigest = hSha.RawHash(shaLen);

	CRijndaelChanged hCrypt;
	hCrypt.MakeKey(cDigest, CRijndaelChanged::sm_chain0, SSE3_KEY_LEN, SSE3_BLOCK_SIZE);

	uint32_t decrypted[DATA_SIZE / sizeof(uint32_t)];
	if (!hCrypt.Decrypt((char *)&ticket->Random, (char *)decrypted, DATA_SIZE, CRijndaelChanged::CBC)) {
		return CA_UNKNOWN;
	}

	if (decrypted[1] != (uint32_t)MAGIC) {
		return CA_UNKNOWN;
	}

	CSteamID* decSteamID = (CSteamID *)&decrypted[2];
	uint32_t decIpAddr = decrypted[4];

	if (!IsReservedAdr(authdata->ipaddr) && decIpAddr != authdata->ipaddr) {
		return CA_UNKNOWN;
	}

	int64_t genTime;
	memcpy(&genTime, &decrypted[6], sizeof(genTime));
	if ((uint64_t)(time(nullptr) - genTime + TIME_TOLERANCE) > 2 * TIME_TOLERANCE) {
		return CA_UNKNOWN;
	}

	if (memcmp(decSteamID, &ticket->SteamID, sizeof(CSteamID)) != 0) {
		return CA_UNKNOWN;
	}

	uint32_t accId = ticket->SteamID.GetAccountID();
	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (!accId || accId == PENDING_ID) {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		authdata->authKeyLen = 0;
	}
	else {
		authdata->steamId = accId;
		authdata->authKeyKind = AK_FILEID;
		authdata->authKeyLen = sizeof(uint32_t);
		*(uint32_t *)authdata->authKey = accId;
	}

	return CA_SSE3;
}

client_auth_kind COldRevEmuAuthorizer::authorize(authdata_t* authdata)
{
	enum
	{
		TICKET_LEN	= 10,
	};

	if (authdata->ticketLen != TICKET_LEN) {
		return CA_UNKNOWN;
	}

	uint32_t* ticket = (uint32_t *)authdata->authTicket;
	if (uint16_t(ticket[0]) != 0xFFFF || uint16_t(ticket[2]) != 0) {
		return CA_UNKNOWN;
	}

	uint32_t volumeId = ticket[1] & 0x7FFFFFFF;
	LCPrintf(false, "OldRevEmu VolumeID: %u\n", volumeId);

	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (volumeId == 0) {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		authdata->authKeyLen = 0;
	}
	else {
		uint32_t hash = volumeId ^ g_SteamEmuHashKey;
		authdata->steamId = hash << 1;
		authdata->authKeyKind = AK_VOLUMEID;
		authdata->authKeyLen = sizeof(uint32_t);
		*(uint32_t *)authdata->authKey = volumeId;
	}

	return CA_OLD_REVEMU;
}

client_auth_kind CSteamEmuAuthorizer::authorize(authdata_t* authdata)
{
	enum
	{
		TICKET_LEN	= 0x300,
		PENDING_ID	= 777
	};

	if (authdata->ticketLen != TICKET_LEN) {
		return CA_UNKNOWN;
	}

	uint32_t* ticket = (unsigned int *)authdata->authTicket;
	if (ticket[20] != 0xFFFFFFFF) {
		return CA_UNKNOWN;
	}

	uint32_t accId = ticket[21];
	uint32_t volumeId = (ticket[21] ^ g_SteamEmuHashKey) & 0x7FFFFFFF;
	LCPrintf(false, "SteamEmu VolumeID: %u\n", volumeId);

	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (accId == CI_STEAM_ID_LAN || accId == PENDING_ID) {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		authdata->authKeyLen = 0;
	}
	else {
		uint32_t hash = ticket[21] ^ g_SteamEmuHashKey;
		authdata->steamId = hash << 1;
		authdata->authKeyKind = AK_VOLUMEID;
		authdata->authKeyLen = sizeof(uint32_t);

		if (g_ReunionConfig->getAuthVersion() >= av_reunion2018)
			*(uint32_t *)authdata->authKey = volumeId;
		else
			*(uint32_t *)authdata->authKey = accId;
	}

	return CA_STEAM_EMU;
}

client_auth_kind CSXEIAuthorizer::authorize(authdata_t* authdata)
{
	if (!g_ReunionConfig->isSXEIAuthEnabled()) {
		return CA_UNKNOWN;
	}

	char* hid = g_engfuncs.pfnInfoKeyValue(authdata->userinfo, "*HID");
	if (!hid || !hid[0]) {
		return CA_UNKNOWN;
	}

	uint32 sxeId;
	if (1 != sscanf(hid, "%X", &sxeId)) {
		return CA_UNKNOWN;
	}

	authdata->idtype = AUTH_IDTYPE_LOCAL;

	if (sxeId == 0) {
		authdata->steamId = STEAM_ID_PENDING;
		authdata->authKeyKind = AK_OTHER;
		authdata->authKeyLen = 0;
	}
	else {
		authdata->steamId = sxeId << 1;
		authdata->authKeyKind = AK_SXEID;

		if (g_ReunionConfig->getAuthVersion() >= av_reunion2018) {
			authdata->authKeyLen = sizeof(uint32_t);
			*(uint32_t *)authdata->authKey = sxeId;
		}
		else {
			authdata->authKeyLen = min(strlen(hid), sizeof authdata->authKey);
			memcpy(authdata->authKey, hid, authdata->authKeyLen);
		}
	}

	return CA_SXEI;
}

client_auth_kind CHLTVAuthorizer::authorize(authdata_t* authdata)
{
	char* hltv = g_engfuncs.pfnInfoKeyValue(authdata->userinfo, "*hltv");
	if (!hltv || !hltv[0]) {
		return CA_UNKNOWN;
	}

	authdata->idtype = AUTH_IDTYPE_LOCAL;
	authdata->steamId = STEAM_ID_LAN;

	authdata->authKeyKind = AK_OTHER;
	authdata->authKeyLen = 0;

	return CA_HLTV;
}

client_auth_kind CNoSteam47Authorizer::authorize(authdata_t* authdata)
{
	if (authdata->protocol != 47) {
		return CA_UNKNOWN;
	}

	authdata->steamId = STEAM_ID_LAN;
	authdata->idtype = AUTH_IDTYPE_LOCAL;

	authdata->authKeyKind = AK_OTHER;
	authdata->authKeyLen = 0;

	return CA_NO_STEAM_47;
}

client_auth_kind CNoSteam48Authorizer::authorize(authdata_t* authdata)
{
	if (authdata->protocol != 48) {
		return CA_UNKNOWN;
	}

	authdata->steamId = STEAM_ID_LAN;
	authdata->idtype = AUTH_IDTYPE_LOCAL;

	authdata->authKeyKind = AK_OTHER;
	authdata->authKeyLen = 0;

	return CA_NO_STEAM_48;
}

void Reunion_Add_Authorizer(IClientAuthorizer* a)
{
	if (g_NumClientAuthorizers >= MAX_CLIENT_AUTHORIZERS) {
		util_syserror("Reunion_Add_Authorizer: MAX_CLIENT_AUTHORIZERS limit hit\n");
		return;
	}

	g_ClientAuthorizers[g_NumClientAuthorizers++] = a;
}

void Reunion_Init_Authorizers()
{
	Reunion_Add_Authorizer(new CHLTVAuthorizer());
	Reunion_Add_Authorizer(new CSXEIAuthorizer());
	Reunion_Add_Authorizer(new CSettiAuthorizer());
	Reunion_Add_Authorizer(new CAVSMPAuthorizer());
	Reunion_Add_Authorizer(new CSSE3Authorizer());
	Reunion_Add_Authorizer(new CRevEmu2013Authorizer());
	Reunion_Add_Authorizer(new CSteamClient2009Authorizer());
	Reunion_Add_Authorizer(new CRevEmuAuthorizer());
	Reunion_Add_Authorizer(new COldRevEmuAuthorizer());
	Reunion_Add_Authorizer(new CSteamEmuAuthorizer());
	Reunion_Add_Authorizer(new CNoSteam47Authorizer());
	Reunion_Add_Authorizer(new CNoSteam48Authorizer());
}

client_auth_kind Reunion_Authorize_Client(authdata_t* authdata)
{
	for (int i = 0; i < g_NumClientAuthorizers; i++) {
		client_auth_kind authkind = g_ClientAuthorizers[i]->authorize(authdata);
		if (authkind != CA_UNKNOWN) {
			return authkind;
		}
	}

	return CA_UNKNOWN;
}

const char* Reunion_GetAuthorizerName(client_auth_kind authKind)
{
	return g_AuthorizerName[authKind];
}
