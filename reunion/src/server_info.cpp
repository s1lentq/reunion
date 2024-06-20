#include "precompiled.h"

CServerInfo* g_ServerInfo;

const char* CServerInfo::DETAILS			= "details";
const char* CServerInfo::PLAYERS			= "players";
const char* CServerInfo::CONNECT			= "connect";
const char* CServerInfo::CHALLENGE			= "challenge";
const char* CServerInfo::GETCHALLENGE		= "getchallenge";

CServerInfo::CServerInfo()
{
	m_maxPlayers = 0;
	m_pEdicts = nullptr;
	m_pcv_hostname = nullptr;
	m_pcv_sv_tags = nullptr;
	m_pcv_sv_visiblemaxplayers = nullptr;
	m_pcv_sv_password = nullptr;
	m_pcv_net_address = nullptr;
	m_pEntityInterface = nullptr;
	m_realTime = 0.0;
	m_port = 0;
	m_appId = 0;
	memset(m_appVersion, 0, sizeof(m_appVersion));
	memset(m_gameDir, 0, sizeof(m_gameDir));
	m_multipacketId = rand() | (rand() << 16);

	// Query emulator
	m_respSource.updateInterval		= QUERY_UPDATE_INTERVAL;
	m_respSource.write				= &CServerInfo::writeSourceResponse;
	m_respGoldSrc.updateInterval	= QUERY_UPDATE_INTERVAL;
	m_respGoldSrc.write				= &CServerInfo::writeGoldSourceResponse;
	m_respPlayers.updateInterval	= PLAYERS_UPDATE_INTERVAL;
	m_respPlayers.write				= &CServerInfo::writePlayersList;
	m_respRules.updateInterval		= RULES_UPDATE_INTERVAL;
	m_respRules.write				= &CServerInfo::writeRulesList;

	m_queryLimiter.addExceptIPs(g_ReunionConfig->getExceptIPs());
	m_lastRatesCheck = g_RehldsFuncs->GetRealTime();
}

void CServerInfo::writeSourceResponse(CSizeBuf& szbuf) const
{
	szbuf.WriteLong(CONNECTIONLESS_HEADER);				// connectionless header
	szbuf.WriteByte(S2A_INFO);							// data header
	szbuf.WriteByte(48);								// protocol
	szbuf.WriteString(getHostName());					// hostname
	szbuf.WriteString(getMapName());					// mapname
	if (m_queryLimiter.isUnderFlood()) {
		szbuf.WriteChar('\0');							// gamedir
		szbuf.WriteChar('\0');							// gamename
	} else {
		szbuf.WriteString(getGameDir());				// gamedir
		szbuf.WriteString(getGameDescription());		// gamename
	}
	szbuf.WriteShort(getAppId());						// appid
	szbuf.WriteByte((uint8)getPlayersCount());			// players
	szbuf.WriteByte((uint8)getVisibleMaxPlayers());		// maxplayers
	szbuf.WriteByte((uint8)getPlayingBots());			// bots
	szbuf.WriteByte('d');								// server type
	szbuf.WriteByte(getOS());							// server os
	szbuf.WriteByte(getIsPasswordSet());				// password
	szbuf.WriteByte(getSecure());						// secure
	szbuf.WriteString(getAppVersion());					// app version

	// Extra Data Flag (EDF)
	uint8_t edf = EDF_FLAG_PORT;

	const char *pchTags = getTags();
	if (pchTags && pchTags[0])
		edf |= EDF_FLAG_GAME_TAGS;

	szbuf.WriteByte(edf);

	// Server port
	if (edf & EDF_FLAG_PORT)
		szbuf.WriteShort(getPort());

	// Server's SteamID
	if (edf & EDF_FLAG_STEAMID)
		szbuf.WriteUint64(Steam_GSGetSteamID());

	// Port    short   Spectator port number for SourceTV
	// Name    string  Name of the spectator server for SourceTV
	if (edf & EDF_FLAG_SOURCE_TV)
		szbuf.WriteShort(0);

	// Keywords    string  Tags that describe the game according to the server (for future use)
	if (edf & EDF_FLAG_GAME_TAGS)
		szbuf.WriteString(getTags());

	// GameID  long long   The server's 64-bit GameID
	// If this is present, a more accurate AppID is present in the low 24 bits
	// The earlier AppID could have been truncated as it was forced into 16-bit storage
	if (edf & EDF_FLAG_GAMEID)
		szbuf.WriteUint64(getAppId());
}

void CServerInfo::writeGoldSourceResponse(CSizeBuf& szbuf) const
{
	szbuf.WriteLong(CONNECTIONLESS_HEADER);			// connectionless header
	szbuf.WriteByte(S2A_INFO_DETAILED);				// data header (detailed mode)
	szbuf.WriteString(getServerAddress());			// server address
	szbuf.WriteString(getHostName());				// hostname
	szbuf.WriteString(getMapName());				// mapname
	szbuf.WriteString(getGameDir());				// gamedir
	szbuf.WriteString(getGameDescription());		// gamename
	szbuf.WriteByte((uint8)getPlayersCount());		// players
	szbuf.WriteByte((uint8)getVisibleMaxPlayers());	// maxplayers
	szbuf.WriteByte(47);							// protocol
	szbuf.WriteByte('d');							// server type
	szbuf.WriteByte(getOS());						// server os
	szbuf.WriteByte(getIsPasswordSet());			// password
	szbuf.WriteByte(FALSE);							// mod info
	szbuf.WriteByte(getSecure());					// secure
	szbuf.WriteByte((uint8)getPlayingBots());		// bots
}

void CServerInfo::writePlayersList(CSizeBuf& szbuf) const
{
	szbuf.WriteLong(CONNECTIONLESS_HEADER);					// connectionless header
	szbuf.WriteByte(S2A_PLAYERS);							// data header
	byte* pcount = szbuf.GetData() + szbuf.GetCurSize();
	szbuf.WriteByte(0);										// players count

	size_t count = 0;

	for (int i = 0; i < m_maxPlayers; i++)
	{
		const CReunionPlayer *player = g_Players[i];

		if (!player->getClient()->IsConnected())
			continue;

		szbuf.WriteByte((uint8)(count + (int)player->getClient()->IsFakeClient() * 128));	// index of player chunk starting from 0
		szbuf.WriteString(player->getClient()->GetName());									// name
		szbuf.WriteLong(int(m_pEdicts[i + 1].v.frags));										// frags
		szbuf.WriteFloat(m_realTime - player->getConnectionTime());							// playing time

		count++;
	}

	*pcount = byte(count);
}

void CServerInfo::writeRulesList(CSizeBuf& szbuf) const
{
	szbuf.WriteLong(CONNECTIONLESS_HEADER);									// connectionless header
	szbuf.WriteByte(S2A_RULES);												// data header
	uint16* pcount = (uint16 *)(szbuf.GetData() + szbuf.GetCurSize());
	szbuf.WriteShort(0);													// cvars count

	size_t count = 0;

	for (cvar_t* var = g_RehldsFuncs->GetCvarVars(); var; var = var->next)
	{
		if (var->flags & FCVAR_SERVER)
		{
			szbuf.WriteString(var->name);
			if (var->flags & FCVAR_PROTECTED)
			{
				if (var->string[0] && strcmp(var->string, "none") != 0)
					szbuf.WriteChar('1');
				else
					szbuf.WriteChar('0');
				szbuf.WriteChar('\0');
			}
			else
				szbuf.WriteString(var->string);

			if (szbuf.IsOverflowed())
				break;

			count++;
		}
	}

	*pcount = uint16(count);
}

void CServerInfo::sendResponse(const netadr_t& adr, response_t& response)
{
	if (float(m_realTime - response.lastUpdate) > response.updateInterval) {
		response.lastUpdate = m_realTime;
		response.cache.Clear();
		(this->*response.write)(response.cache);
	}

	if (response.cache.GetCurSize() <= STEAM_MAX_PACKET_SIZE || !g_ReunionConfig->isAllowSplitPackets())
		return g_RehldsFuncs->NET_SendPacket(response.cache.GetCurSize(), response.cache.GetData(), adr);

	uint8_t buf[STEAM_MAX_PACKET_SIZE];
	size_t fragSize = STEAM_MAX_PACKET_SIZE - sizeof(long) - sizeof(long) - sizeof(byte);
	size_t fragCount = (response.cache.GetCurSize() + fragSize - 1) / fragSize;
	size_t offset = 0;

	for (size_t packetNumber = 0; packetNumber < fragCount; packetNumber++) {
		if (packetNumber == fragCount - 1)
			fragSize = response.cache.GetCurSize() - offset;

		uint8_t packetID = (uint8_t)((packetNumber << 4) + fragCount);
		CSizeBuf sendbuf(buf, sizeof buf);
		sendbuf.WriteLong(MULTIPACKET_HEADER);			// connectionless header
		sendbuf.WriteLong(m_multipacketId);				// sequence number
		sendbuf.WriteByte(packetID);					// fragment number
		sendbuf.Write(response.cache.GetData() + offset, fragSize);

		g_RehldsFuncs->NET_SendPacket(sendbuf.GetCurSize(), sendbuf.GetData(), adr);

		offset += fragSize;
	}

	m_multipacketId++;
}

void CServerInfo::sendQueryChallenge(const netadr_t& to)
{
	uint8 buf[16];
	CSizeBuf szbuf(buf, sizeof buf);

	szbuf.WriteLong(CONNECTIONLESS_HEADER);
	szbuf.WriteChar(S2C_CHALLENGE);
	szbuf.WriteLong(g_RehldsFuncs->SV_GetChallenge(to));

	g_RehldsFuncs->NET_SendPacket(szbuf.GetCurSize(), szbuf.GetData(), to);
}

void CServerInfo::sendServerInfo(const netadr_t& to, server_answer_type sat)
{
	switch (sat)
	{
	case sat_source:
		sendResponse(to, m_respSource);
		break;

	case sat_goldsource:
		sendResponse(to, m_respGoldSrc);
		break;

	case sat_hybrid:
		sendResponse(to, m_respGoldSrc);
		if (g_ReunionConfig->allowFixBuggedQuery() && m_queryBugfix.isBuggedQuery(to)) {
			sendEmptyPlayersList(to);
		}
		sendResponse(to, m_respSource);
		break;

	default:
		util_syserror("invalid response type %u\n", sat);
		break;
	}
}

void CServerInfo::sendPlayersList(const netadr_t& to)
{
	sendResponse(to, m_respPlayers);
}

void CServerInfo::sendEmptyPlayersList(const netadr_t& to)
{
	uint8 buf[16];
	CSizeBuf szbuf(buf, sizeof buf);

	szbuf.WriteLong(CONNECTIONLESS_HEADER);		// connectionless header
	szbuf.WriteByte(S2A_PLAYERS);				// data header
	szbuf.WriteByte(0);							// some clients crashing if not 0

	g_RehldsFuncs->NET_SendPacket(szbuf.GetCurSize(), szbuf.GetData(), to);
}

void CServerInfo::sendRulesList(const netadr_t& to)
{
	sendResponse(to, m_respRules);
}

#define equal(x, s) !memcmp(x, s, sizeof s - 1)

bool CServerInfo::handleQueryGlobal(IRehldsHook_PreprocessPacket* chain, CSizeBuf& szbuf, const netadr_t& from)
{
	switch (szbuf.ReadChar())
	{
		/* Source style requests. All requests starting from registered chars are handled in steam client library. */
		case A2S_INFO:
		{
			// Ignore invalid requests
			if (szbuf.GetMaxSize() < A2S_INFO_LEN || szbuf.GetData()[5] != 'S') // "Source Engine Query"
				return false;

			g_RehldsFuncs->NET_SendPacket(m_respSource.cache.GetCurSize(), m_respSource.cache.GetData(), from);
			return false;
		}

		case A2S_PLAYER:
		{
			if (szbuf.GetCurSize() < A2S_PLAYER_MINLEN) // actual len is 10, but 9 also valid
				return false;

			int challenge = szbuf.ReadLong();

			if (challenge <= 0 || !g_RehldsFuncs->CheckChallenge(from, challenge)) {
				sendQueryChallenge(from);
			}
			else {
				sendPlayersList(from);
			}

			return false;
		}

		/* GoldSource style requests */
		case 'c': // connect/challenge
		{
			if (equal(szbuf.GetData() + 4, CONNECT) && szbuf.GetCurSize() > 64) {
				break;
			}
			if (equal(szbuf.GetData() + 4, CHALLENGE)) {
				break;
			}
			return false;
		}

		case 'g': // getchallenge
		{
			if (equal(szbuf.GetData() + 4, GETCHALLENGE)) {
				break;
			}
			return false;
		}

		default:
			return false;
	}

	return chain->callNext(szbuf.GetData(), szbuf.GetCurSize(), from);
}

bool CServerInfo::handleQuery(IRehldsHook_PreprocessPacket* chain, uint8* data, unsigned int len, const netadr_t& from)
{
	m_queryLimiter.incomingPacket(len + PACKET_HEADER_SIZE);

	if (len < 5) {
		return false;
	}

	CSizeBuf szbuf(data, len, len);

	// Not connectionless
	if (szbuf.ReadLong() != -1) {
		return chain->callNext(data, len, from);
	}

	if (g_ReunionConfig->enableQueryLimiter()) {
		m_queryLimiter.incomingQuery();
		if (m_queryLimiter.getUseGlobalRateLimit()) {
			return m_queryLimiter.allowGlobalQuery() ? handleQueryGlobal(chain, szbuf, from) : false;
		}
	}

	//LCPrintf(true, "Connectionless packet %i: %.*s\n", len, max(0, int(len) - 4), szbuf.GetData() + 4);

	switch (szbuf.ReadChar())
	{
		/* Source style requests. All requests starting from registered chars are handled in steam client library. */
		case A2S_INFO:
		{
			// Ignore invalid requests
			if (szbuf.GetMaxSize() < A2S_INFO_LEN || data[5] != 'S') // "Source Engine Query"
				return false;

			if (m_queryLimiter.allowQuery(from)) {
				server_answer_type sat = g_ReunionConfig->getServerAnswerType();

				if (sat == sat_hybrid && m_queryLimiter.isUnderFlood()) {
					sat = sat_source;
				}

				sendServerInfo(from, sat);
			}

			return false;
		}

		case A2S_RULES:
		{
			if (len != A2S_RULES_MINLEN)
				return false;

			int challenge = szbuf.ReadLong();

			if (challenge <= 0 || !g_RehldsFuncs->CheckChallenge(from, challenge)) { // HLSW often uses outdated challenge
				sendQueryChallenge(from);
			}
			else {
				sendRulesList(from);
			}

			return false;
		}

		case A2S_PLAYER:
		{
			if (len < A2S_PLAYER_MINLEN) // actual len is 10, but 9 also valid
				return false;

			int challenge = szbuf.ReadLong();

			if (challenge <= 0 || !g_RehldsFuncs->CheckChallenge(from, challenge)) {
				sendQueryChallenge(from);
			}
			else {
				sendPlayersList(from);
			}

			return false;
		}

		// ignore incoming server query responces
		case S2A_INFO:
		case S2A_PLAYERS:
		case S2A_INFO_OLD:
		case S2A_INFO_DETAILED:
			return false;

		/* GoldSource style requests */

		// old 'add to favorites' query
		case 'd': // details
		{
			if (!equal(data + 4, DETAILS))
				break;

			server_answer_type sat = g_ReunionConfig->getServerAnswerType();

			if (sat != sat_source && m_queryLimiter.allowQuery(from) && !m_queryLimiter.isUnderFlood()) {
				sendServerInfo(from, sat_goldsource);
			}

			return false;
		}

		case 'p':
		{
			if (!equal(data + 4, PLAYERS))
				break;

			server_answer_type sat = g_ReunionConfig->getServerAnswerType();

			if (sat != sat_source) {
				if (m_queryLimiter.allowQuery(from) && !m_queryLimiter.isUnderFlood()) {
					sendPlayersList(from);
				}
				else {
					sendEmptyPlayersList(from);
				}
			}
			break;
		}

		case 'c': // connect/challenge
		{
			if (equal(data + 4, CONNECT) && len < 64) {
				return false;
			}
			if (equal(data + 4, CHALLENGE) && !m_queryLimiter.allowQuery(from, true)) {
				return false;
			}
			break;
		}

		case 'g': // getchallenge
		{
			if (equal(data + 4, GETCHALLENGE) && !m_queryLimiter.allowQuery(from, true)) {
				return false;
			}
			break;
		}

		default:
			break;
	}

	// query not handled
	return chain->callNext(data, len, from);
}

bool CServerInfo::isAddressBanned(const netadr_t& from) const
{
	return m_queryBanList.isBanned(*(uint32 *)from.ip);
}

void CServerInfo::banAddress(const char* addr, uint32_t time)
{
	m_queryBanList.addBan(inet_addr(addr), time);
}

void CServerInfo::unbanAddress(const char* addr)
{
	m_queryBanList.removeBan(inet_addr(addr));
}

void CServerInfo::printBans()
{
	m_queryBanList.print();
}

void CServerInfo::startFrame()
{
	m_realTime = g_RehldsFuncs->GetRealTime();

	if (!g_ReunionConfig->enableQueryLimiter())
		return;

	double realtime = g_RehldsFuncs->GetRealTime();
	float delta = float(realtime - m_lastRatesCheck);

	if (delta > QUERY_CHECK_INTERVAL) {
		m_lastRatesCheck = realtime;
		m_queryBanList.removeExpired(realtime);
		m_queryLimiter.checkRates(realtime, delta, m_queryBanList);
	}
}

edict_t* CServerInfo::getEdict(size_t index) const
{
	return m_pEdicts + index;
}

void CServerInfo::serverActivate(edict_t* edicts, int maxclients)
{
	m_pEdicts = edicts;
	m_maxPlayers = maxclients;

	GET_HOOK_TABLES(PLID, nullptr, &m_pEntityInterface, nullptr);

	m_pcv_hostname = g_engfuncs.pfnCVarGetPointer("hostname");
	m_pcv_sv_tags = g_engfuncs.pfnCVarGetPointer("sv_tags");
	m_pcv_sv_visiblemaxplayers = g_engfuncs.pfnCVarGetPointer("sv_visiblemaxplayers");
	m_pcv_sv_password = g_engfuncs.pfnCVarGetPointer("sv_password");
	m_pcv_net_address = g_engfuncs.pfnCVarGetPointer("net_address");

	m_appId = parseAppId();
	parseAppVersion(m_appVersion, sizeof m_appVersion - 1);
	m_port = atoi(g_engfuncs.pfnCVarGetString("hostport"));

	if (!m_port) {
		m_port = atoi(g_engfuncs.pfnCVarGetString("port"));
		if (!m_port) {
			m_port = PORT_SERVER;
		}
	}

	g_engfuncs.pfnGetGameDir(m_gameDir);
}

const char* CServerInfo::getHostName() const
{
	return m_pcv_hostname->string;
}

const char* CServerInfo::getTags() const
{
	return m_pcv_sv_tags ? m_pcv_sv_tags->string : nullptr;
}

const char* CServerInfo::getMapName()
{
	return STRING(gpGlobals->mapname);
}

const char* CServerInfo::getGameDir() const
{
	return m_gameDir;
}

const char* CServerInfo::getGameDescription() const
{
	if (m_pEntityInterface->pfnGetGameDescription)
	{
		return m_pEntityInterface->pfnGetGameDescription();
	}

	return MDLL_GetGameDescription();
}

size_t CServerInfo::getPlayersCount()
{
	return Reunion_GetConnectedPlayersCount();
}

size_t CServerInfo::getPlayingBots() const
{
	size_t count = 0;

	for (int i = 1; i <= m_maxPlayers; i++)
	{
		if (!(m_pEdicts[i].v.flags & FL_FAKECLIENT))
			continue;

		if (m_pEdicts[i].v.flags & (FL_SPECTATOR | FL_DORMANT))
			continue;

		count++;
	}

	return count;
}

int CServerInfo::getMaxPlayers() const
{
	return m_maxPlayers;
}

int CServerInfo::getVisibleMaxPlayers() const
{
	int visible = atoi(m_pcv_sv_visiblemaxplayers->string);
	return (visible < 0) ? m_maxPlayers : visible;
}

char CServerInfo::getOS()
{
#ifdef _WIN32
	return 'w';
#else
	return 'l';
#endif
}

uint8 CServerInfo::getIsPasswordSet() const
{
	return (m_pcv_sv_password->string[0] && strcmp(m_pcv_sv_password->string, "none")) ? TRUE : FALSE;
}

uint8 CServerInfo::getSecure()
{
	return g_RehldsFuncs->GSBSecure();
}

const char* CServerInfo::getServerAddress() const
{
	return m_pcv_net_address->string;
}

double CServerInfo::getRealTime() const
{
	return m_realTime;
}

int CServerInfo::getPort() const
{
	return m_port;
}

int CServerInfo::getAppId() const
{
	return m_appId;
}

const char* CServerInfo::getAppVersion() const
{
	return m_appVersion;
}

int CServerInfo::parseAppId()
{
	int appId = 0;
	char filename[MAX_PATH];
	snprintf(filename, sizeof filename, "./%s/steam_appid.txt", m_gameDir);

	FILE* fp = fopen(filename, "rt");
	if (fp) {
		char line[256];

		if (fgets(line, sizeof line, fp))
			appId = atoi(line);

		fclose(fp);
	}

	if (!appId) {
		appId = HLDS_APPID;
	}

	return appId;
}

void CServerInfo::parseAppVersion(char* buf, size_t maxlen)
{
	char filename[MAX_PATH];
	snprintf(filename, sizeof filename, "./%s/steam.inf", m_gameDir);

	FILE* fp = fopen(filename, "rt");
	if (fp) {
		char line[256];

		while (fgets(line, sizeof line, fp)) {
			char* value = strchr(line, '=');
			if (!value) {
				continue;
			}

			*value++ = '\0';
			trimbuf(line);
			trimbuf(value);

			if (!strcmp(line, "PatchVersion=")) {
				snprintf(buf, maxlen, "%s/Stdio", value);
				break;
			}
		}
		fclose(fp);
	}

	if (!buf[0]) {
		strncpy(buf, HLDS_APPVERSION, maxlen);
	}
}

bool Reunion_PreprocessPacket(IRehldsHook_PreprocessPacket* chain, uint8* data, unsigned int len, const netadr_t& from)
{
	if (g_ServerInfo->isAddressBanned(from))
		return false;

	return g_ServerInfo->handleQuery(chain, data, len, from);
}

bool Reunion_Init_ServerInfo()
{
	g_ServerInfo = new CServerInfo();
	g_RehldsHookchains->PreprocessPacket()->registerHook(&Reunion_PreprocessPacket);
	return true;
}
