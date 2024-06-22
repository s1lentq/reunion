#include "precompiled.h"

CReunionPlayer **g_Players = NULL;

CReunionPlayer::CReunionPlayer(IGameClient* cl) {
	m_Id = cl->GetId();
	m_pClient = cl;
	clear();
}

void CReunionPlayer::clear() {
	m_IdKind = CI_UNKNOWN;
	m_AuthKind = CA_UNKNOWN;
	m_Protocol = 0;
	m_ConnectionTime = 0.0;
	m_UnauthenticatedSteamId = 0;
	memset(&m_DisplaySteamId, 0, sizeof(m_DisplaySteamId));
	memset(&m_SerializedId, 0, sizeof(m_SerializedId));
	memset(&m_StorageSteamId, 0, sizeof(m_StorageSteamId));
	memset(&m_rawAuthData, 0, sizeof(m_rawAuthData));
	m_rawAuthDataLen = 0;
}

void CReunionPlayer::setRawAuthData(auth_key_kind kk, void* data, size_t len) {
	m_authKeyKind = kk;
	m_rawAuthDataLen = min(len, sizeof(m_rawAuthData));
	if (data && len)
		memcpy(m_rawAuthData, data, m_rawAuthDataLen);
}

void CReunionPlayer::setStorageId(USERID_t* userid) {
	memcpy(&m_StorageSteamId, userid, sizeof(m_StorageSteamId));
}

void CReunionPlayer::setUnauthenticatedId(uint64 steamId) {
	m_UnauthenticatedSteamId = steamId;
}

void CReunionPlayer::authenticated(int proto, client_id_kind idkind, client_auth_kind authkind, USERID_t* userid) {
	m_Protocol = proto;
	m_IdKind = idkind;
	m_AuthKind = authkind;
	m_DisplaySteamId = *userid;
	m_ConnectionTime = g_RehldsFuncs->GetRealTime();

	generateSerializedId();

	uint32 accId = (uint32)m_DisplaySteamId.m_SteamID;
	const client_id_gen_opts_t* idGenOpts = g_ReunionConfig->getIdGenOptions(m_AuthKind);
	const client_id_gen_opts_t* idByIpGenOpts = g_ReunionConfig->getIdByIpGenOpts();

	// use auth key type prefixes
	bool akPrefixes = g_ReunionConfig->getAuthVersion() == av_reunion2018;

	switch (m_IdKind) {
	case CI_REAL_STEAM:
		sprintf(m_idString, "STEAM_%u:%u:%u", akPrefixes ? m_authKeyKind : idGenOpts->prefix1, accId & 1, accId >> 1);
		break;

	case CI_REAL_VALVE:
		sprintf(m_idString, "VALVE_%u:%u:%u", akPrefixes ? m_authKeyKind : idGenOpts->prefix1, accId & 1, accId >> 1);
		break;

	case CI_STEAM_BY_IP:
		if (akPrefixes)
			sprintf(m_idString, "STEAM_%u:%u:%u", AK_MAX, accId & 1, accId >> 1);
		else
			sprintf(m_idString, "STEAM_%u:%u:%u", idByIpGenOpts->prefix1, idByIpGenOpts->prefix2, accId >> 1);
		break;

	case CI_VALVE_BY_IP:
		if (akPrefixes)
			sprintf(m_idString, "VALVE_%u:%u:%u", AK_MAX, accId & 1, accId >> 1);
		else
			sprintf(m_idString, "VALVE_%u:%u:%u", idByIpGenOpts->prefix1, idByIpGenOpts->prefix2, accId >> 1);
		break;

	case CI_HLTV:
		sprintf(m_idString, "HLTV");
		break;

	case CI_STEAM_ID_LAN:
		sprintf(m_idString, "STEAM_ID_LAN");
		break;

	case CI_STEAM_ID_PENDING:
		sprintf(m_idString, "STEAM_ID_PENDING");
		break;

	case CI_VALVE_ID_LAN:
		sprintf(m_idString, "VALVE_ID_LAN");
		break;

	case CI_VALVE_ID_PENDING:
		sprintf(m_idString, "VALVE_ID_PENDING");
		break;

	case CI_STEAM_666_88_666:
		sprintf(m_idString, "STEAM_666:88:666");
		break;

	default:
		sprintf(m_idString, "UNKNOWN");
		break;
	}
}

void CReunionPlayer::generateSerializedId() {
	memset(&m_SerializedId, 0, sizeof(m_SerializedId));
	switch (m_IdKind) {
	case CI_REAL_STEAM:
	case CI_REAL_VALVE:
	case CI_STEAM_BY_IP:
	case CI_VALVE_BY_IP:
		m_SerializedId = m_DisplaySteamId;
		break;

	case CI_HLTV:
		m_SerializedId.idtype = AUTH_IDTYPE_LOCAL;
		m_SerializedId.m_SteamID = STEAM_ID_LAN;
		break;

	case CI_STEAM_ID_PENDING:
		m_SerializedId.idtype = AUTH_IDTYPE_STEAM;
		m_SerializedId.m_SteamID = STEAM_ID_PENDING;
		break;

	case CI_VALVE_ID_PENDING:
		m_SerializedId.idtype = AUTH_IDTYPE_VALVE;
		m_SerializedId.m_SteamID = STEAM_ID_PENDING;
		break;

	case CI_VALVE_ID_LAN:
		m_SerializedId.idtype = AUTH_IDTYPE_VALVE;
		m_SerializedId.m_SteamID = STEAM_ID_LAN;
		break;

	case CI_STEAM_ID_LAN:
	case CI_STEAM_666_88_666:
		m_SerializedId.idtype = AUTH_IDTYPE_STEAM;
		m_SerializedId.m_SteamID = STEAM_ID_LAN;
		break;

	default:
		break;
	}
}

char* CReunionPlayer::GetSteamId() {
	return m_idString;
}

CSteamID CReunionPlayer::getDisplaySteamId() const
{
	return m_DisplaySteamId.m_SteamID;
}

IGameClient* CReunionPlayer::getClient() const {
	return m_pClient;
}

USERID_t* CReunionPlayer::getSerializedId() {
	return &m_SerializedId;
}

USERID_t* CReunionPlayer::getStorageId() {
	return &m_StorageSteamId;
}

uint64 CReunionPlayer::getUnauthenticatedId() const {
	return m_UnauthenticatedSteamId;
}

bool CReunionPlayer::isUnauthenticatedId() const {
	return m_UnauthenticatedSteamId != 0;
}

int CReunionPlayer::getProcotol() const {
	return m_Protocol;
}

double CReunionPlayer::getConnectionTime() const {
	return m_ConnectionTime;
}

void CReunionPlayer::setConnectionTime(double time) {
	m_ConnectionTime = time;
}

client_auth_kind CReunionPlayer::GetAuthKind() const {
	return m_AuthKind;
}

void Reunion_Free_Players()
{
	if (g_Players == NULL)
		return;

	for (int i = 0; i < g_RehldsSvs->GetMaxClientsLimit(); i++)
	{
		delete g_Players[i];
	}

	free(g_Players);
	g_Players = NULL;
}

bool Reunion_Init_Players()
{
	Reunion_Free_Players();
	g_Players = (CReunionPlayer **)malloc(sizeof(CReunionPlayer) * g_RehldsSvs->GetMaxClientsLimit());
	if (!g_Players)
	{
		LCPrintf(true, "Failed to allocate memory to clients limit %d\n", g_RehldsSvs->GetMaxClientsLimit());
		return false;
	}

	for (int i = 0; i < g_RehldsSvs->GetMaxClientsLimit(); i++)
	{
		g_Players[i] = new CReunionPlayer(g_RehldsSvs->GetClient(i));
		if (!g_Players[i]) {
			LCPrintf(true, "Failed to allocate memory for %d slot\n", i);
			return false;
		}
	}

	return true;
}

CReunionPlayer* GetReunionPlayerByUserIdPtr(USERID_t* pUserId) {
	for (int i = 0; i < g_ServerInfo->getMaxPlayers(); i++) {
		if (g_Players[i]->getClient()->GetNetworkUserID() == pUserId)
			return g_Players[i];
	}

	return nullptr;
}

CReunionPlayer* GetReunionPlayerBySteamdId(uint64 steamIDUser) {
	for (int i = 0; i < g_ServerInfo->getMaxPlayers(); i++) {
		if (g_Players[i]->getClient()->GetNetworkUserID()->m_SteamID == steamIDUser)
			return g_Players[i];
	}

	return nullptr;
}

CReunionPlayer* GetReunionPlayerByClientPtr(IGameClient* cl) {
	return g_Players[cl->GetId()];
}

size_t CReunionPlayer::getRawAuthData(void* buf, size_t maxlen) const {
	size_t len = min(m_rawAuthDataLen, maxlen);
	memcpy(buf, m_rawAuthData, len);
	return len;
}

auth_key_kind CReunionPlayer::GetAuthKeyKind() const
{
	return m_authKeyKind;
}

void CReunionPlayer::kick(const char* reason) const
{
	char buf[256];
	snprintf(buf, sizeof buf, "kick #%i %c%s%c\n", g_engfuncs.pfnGetPlayerUserId(g_ServerInfo->getEdict(m_Id + 1)), 0x22, reason, 0x22);
	g_engfuncs.pfnServerCommand(buf);
	g_engfuncs.pfnServerExecute();
}

size_t Reunion_GetConnectedPlayersCount() {
	size_t count = 0;

	for (int i = 0; i < g_ServerInfo->getMaxPlayers(); i++) {
		if (g_Players[i]->getClient()->IsConnected())
			count++;
	}

	return count;
}

bool Reunion_IsSteamIdInUse(CReunionPlayer* player)
{
	if (g_ReunionConfig->getIDClientsLimit() == 0)
		return false;

	int idInUse = 0;

	char steamId[32];
	strncpy(steamId, player->GetSteamId(), sizeof steamId - 1);
	steamId[sizeof steamId - 1] = '\0';

	for (int i = 0; i < g_ServerInfo->getMaxPlayers(); i++) {
		auto cmpPlayer = g_Players[i];

		if (player == cmpPlayer)
			continue;

		if (cmpPlayer->GetAuthKind() == CA_UNKNOWN || !cmpPlayer->getClient()->IsConnected())
			continue;

		if (!strcmp(steamId, cmpPlayer->GetSteamId()))
		{
			if (++idInUse >= g_ReunionConfig->getIDClientsLimit())
				return true;
		}
	}

	return false;
}

void Reunion_RemoveDuplicateSteamIds(CReunionPlayer* player)
{
	char steamId[32];
	strncpy(steamId, player->GetSteamId(), sizeof steamId - 1);
	steamId[sizeof steamId - 1] = '\0';

	for (int i = 0; i < g_ServerInfo->getMaxPlayers(); i++) {
		auto cmpPlayer = g_Players[i];

		if (player == cmpPlayer)
			continue;

		if (cmpPlayer->GetAuthKind() == CA_UNKNOWN || !cmpPlayer->getClient()->IsConnected())
			continue;

		if (!strcmp(steamId, cmpPlayer->GetSteamId())) {
			cmpPlayer->kick("Connected legit steam client with the same SteamID as yours.");
			cmpPlayer->clear();
		}
	}
}
