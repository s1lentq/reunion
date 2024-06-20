#pragma once

#include "reunion_shared.h"

#include <vector>
#include <iterator>

#define REUNION_CFG_FILE		"reunion.cfg"

class CReunionConfig {
private:
	int m_LogMode;

	auth_version m_AuthVersion;
	server_answer_type m_ServerAnswerType;

	enum linetype_e
	{
		LINE_NONE = 0,
		LINE_OPTION,
		LINE_SECTION,
		LINE_END_SECTION
	};

	struct line_t
	{
		line_t(char *_param, char *_value, int _cline, linetype_e _type) :
			param(_param),
			value(_value),
			cline(_cline),
			type(_type)
		{
		}

		char *param;
		char *value;
		int cline;
		linetype_e type;
	};

	std::vector<line_t> m_lines;
	std::vector<ipv4_t> m_ExceptIPs;

	bool m_bSC2009RevEmuCompat;
	bool m_bEnableSxEIds;
	bool m_bEnableGenPrefix2;
	bool m_bFixBuggedQuery;

	uint32 m_HLTVExceptIP;
	bool m_bEnableQueryLimiter;
	int m_queriesBanLevel;
	int m_floodBanTime;
	bool m_allowSplitPackets;
	int m_IDClientsLimit;

	size_t m_SteamIdHashSaltLen;
	char m_SteamIdHashSalt[MAX_STEAMIDSALTLEN];

	client_id_gen_opts_t m_AuthIdGenOptions[CA_MAX];
	client_id_gen_opts_t m_IPGenOptions;

public:
	bool load(FILE *fl);
	static CReunionConfig* createDefault();
	static FILE* open(const char* fname);

	bool parseCfgParam();
	bool parseCfgSection(const char* section, bool (CReunionConfig::*pfnCallback)(const char *param, const char *value, int cline));
	bool parseExceptIP(const char *param, const char *value, int cline);
	void flushCfg();

	std::vector<ipv4_t> &getExceptIPs();

	ipv4_error ipv4_parse(const char* stringaddr, ipv4_t *result);

	bool hasLogMode(reuinon_log_mode m) const {
		return (m_LogMode & m) == m;
	}

	bool isSC2009RevEmuCompat() const {
		return m_bSC2009RevEmuCompat;
	}

	const client_id_gen_opts_t* getIdGenOptions(client_auth_kind authkind) const {
		return &m_AuthIdGenOptions[(int)authkind];
	}

	const client_id_gen_opts_t* getIdByIpGenOpts() const {
		return &m_IPGenOptions;
	}

	bool isSXEIAuthEnabled() const {
		return m_bEnableSxEIds;
	}

	const char* getSteamIdSalt() const {
		return m_SteamIdHashSalt;
	}

	size_t getSteamIdSaltLen() const {
		return m_SteamIdHashSaltLen;
	}

	uint32 getHLTVExceptIP() const {
		return m_HLTVExceptIP;
	}

	bool getEnableGenPrefix2() const {
		return m_bEnableGenPrefix2;
	}

	bool allowFixBuggedQuery() const {
		return m_bFixBuggedQuery;
	}

	auth_version getAuthVersion() const {
		return m_AuthVersion;
	}

	server_answer_type getServerAnswerType() const {
		return m_ServerAnswerType;
	}

	bool enableQueryLimiter() const {
		return m_bEnableQueryLimiter;
	}

	int getQueriesBanLevel() const {
		return m_queriesBanLevel;
	}

	int getFloodBanTime() const {
		return m_floodBanTime;
	}

	bool isAllowSplitPackets() const {
		return m_allowSplitPackets;
	}

	int getIDClientsLimit() const {
		return m_IDClientsLimit;
	}
};

extern CReunionConfig* g_ReunionConfig;
extern bool Reunion_Cfg_LoadDefault();
