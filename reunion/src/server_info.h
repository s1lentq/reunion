#pragma once

#include "query_bugfix.h"
#include "query_banlist.h"
#include "query_limiter.h"

#define QUERY_UPDATE_INTERVAL		0.5f	// A2S_INFO cache time
#define PLAYERS_UPDATE_INTERVAL		0.5f	// A2S_PLAYERS cache time
#define RULES_UPDATE_INTERVAL		2.0f	// A2S_RULES cache time

class CServerInfo;
typedef void (CServerInfo::*write_response_t)(CSizeBuf& szbuf) const;

struct response_t
{
	// ReSharper disable once CppPossiblyUninitializedMember
	response_t(uint8_t* buf, size_t maxsize) : cache(buf, maxsize), lastUpdate(0.0)
	{
	}

	CSizeBuf			cache;
	double				lastUpdate;
	float				updateInterval;
	write_response_t	write;
};

template<size_t N>
struct query_response_t : response_t
{
	query_response_t() : response_t(new uint8_t[N], N)
	{
	}
	~query_response_t()
	{
		delete[] cache.GetData();
	}
};

class CServerInfo
{
public:
	CServerInfo();

	bool handleQuery(IRehldsHook_PreprocessPacket* chain, uint8* data, unsigned int len, const netadr_t& from);
	bool handleQueryGlobal(IRehldsHook_PreprocessPacket* chain, CSizeBuf& szbuf, const netadr_t& from);

	// QUERY PROCEDURES
	void writeSourceResponse(CSizeBuf& szbuf) const;
	void writeGoldSourceResponse(CSizeBuf& szbuf) const;
	void writePlayersList(CSizeBuf& szbuf) const;
	void writeRulesList(CSizeBuf& szbuf) const;

	void serverActivate(edict_t* edicts, int maxclients);

	double getRealTime() const;
	int getMaxPlayers() const;
	void startFrame();
	edict_t* getEdict(size_t index) const;

	bool isAddressBanned(const netadr_t& from) const;
	void banAddress(const char* addr, uint32_t time);
	void unbanAddress(const char* addr);
	void printBans();

protected:
	void sendQueryChallenge(const netadr_t& to);
	void sendEmptyPlayersList(const netadr_t& to);
	void sendRulesList(const netadr_t& to);
	void sendServerInfo(const netadr_t& to, server_answer_type sat);
	void sendPlayersList(const netadr_t& to);
	void sendResponse(const netadr_t& adr, response_t& response);

private:
	const char* getHostName() const;
	const char* getTags() const;
	static const char* getMapName();
	const char* getGameDir() const;
	const char* getGameDescription() const;
	static size_t getPlayersCount();
	size_t getPlayingBots() const;
	int getVisibleMaxPlayers() const;
	static char getOS();
	uint8 getIsPasswordSet() const;
	static uint8 getSecure();
	const char* getServerAddress() const;
	int getPort() const;
	int getAppId() const;
	const char* getAppVersion() const;
	int parseAppId();
	void parseAppVersion(char* buf, size_t maxlen);

private:
	CQueryLimiter		m_queryLimiter;
	CBuggedQueryFix		m_queryBugfix;
	CQueryBanlist		m_queryBanList;

	double				m_lastRatesCheck;

	query_response_t<STEAM_MAX_PACKET_SIZE>		m_respSource;
	query_response_t<STEAM_MAX_PACKET_SIZE>		m_respGoldSrc;
	query_response_t<STEAM_MAX_PACKET_SIZE>		m_respPlayers;
	query_response_t<STEAM_MAX_PACKET_SIZE * 2>	m_respRules;

	int m_maxPlayers;
	edict_t *m_pEdicts;
	cvar_t *m_pcv_hostname;
	cvar_t *m_pcv_sv_tags;
	cvar_t *m_pcv_sv_visiblemaxplayers;
	cvar_t *m_pcv_sv_password;
	cvar_t *m_pcv_net_address;
	DLL_FUNCTIONS *m_pEntityInterface;
	double m_realTime;
	int m_port;
	int m_appId;
	char m_appVersion[32];
	char m_gameDir[MAX_PATH];
	size_t m_multipacketId;

	enum
	{
		A2S_INFO_LEN = 24,
		A2S_PLAYER_MINLEN = 9,
		A2S_RULES_MINLEN = 9
	};

	static const char*	DETAILS;
	static const char*	PLAYERS;
	static const char*	CONNECT;
	static const char*	CHALLENGE;
	static const char*	GETCHALLENGE;
};

extern CServerInfo* g_ServerInfo;

extern bool Reunion_Init_ServerInfo();
