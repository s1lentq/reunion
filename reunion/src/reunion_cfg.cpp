#include "precompiled.h"

CReunionConfig* g_ReunionConfig;

bool Reunion_Cfg_LoadDefault()
{
	FILE* fl = CReunionConfig::open(REUNION_CFG_FILE);
	if (!fl) {
		LCPrintf(true, "Failed to load config: can't find %s in server root or gamedir\n", REUNION_CFG_FILE);
		return false;
	}

	CReunionConfig* cfg = CReunionConfig::createDefault();
	if (!cfg)
		return false;

	if (!cfg->load(fl)) {
		delete cfg;
		return false;
	}

	if (!cfg->parseCfgParam()) {
		return false;
	}

	if (!cfg->parseCfgSection("QueryLimiterExceptIP", &CReunionConfig::parseExceptIP)) {
		return false;
	}

	if (g_ReunionConfig) {
		delete g_ReunionConfig;
	}

	cfg->flushCfg();
	g_ReunionConfig = cfg;
	return true;
}

bool CReunionConfig::parseExceptIP(const char *param, const char *value, int cline)
{
	ipv4_t ipv4;
	ipv4_error err = ipv4_parse(param, &ipv4);
	if (err != ERROR_NO)
	{
		LCPrintf(true, "Config line parsing error '%s' for reason %d on line '%i'\n", param, err, cline);
		return false;
	}

	m_ExceptIPs.emplace_back(ipv4);
	return true;
}

std::vector<ipv4_t> &CReunionConfig::getExceptIPs()
{
	return m_ExceptIPs;
}

bool CReunionConfig::load(FILE *fl)
{
	char linebuf[2048];
	int cline = 0;

	bool sectionStart = false;
	while (fgets(linebuf, sizeof(linebuf), fl))
	{
		cline++;
		bool BOM = linebuf[0] == '\xEF' && linebuf[1] == '\xBB' && linebuf[2] == '\xBF';
		char* l = trimbuf(BOM ? linebuf + 3 : linebuf);
		if (l[0] == '#' || l[0] == ';' || l[0] == '/' || l[0] == '\\')
			continue;

		if (l[0] == '\0')
		{
			if (sectionStart)
			{
				sectionStart = false;
				m_lines.emplace_back(nullptr, nullptr, cline, LINE_END_SECTION); // for break to section list
			}

			continue;
		}

		if (l[0] == '[')
		{
			char *end;
			if ((end = strchr(++l, ']')))
			{
				*end = '\0';

				if (sectionStart) {
					m_lines.emplace_back(nullptr, nullptr, cline, LINE_END_SECTION);
				}

				auto s = Q_strcpy(new char[(end - l) + 1], l);
				m_lines.emplace_back(s, nullptr, cline, LINE_SECTION);
				sectionStart = true;
			}
		}
		else
		{
			char* valSeparator = strchr(l, '=');
			if (!valSeparator && !sectionStart) {
				LCPrintf(true, "Config line parsing error: missed '=' on line %d\n", cline);
				continue;
			}

			char* s = Q_strcpy(new char[Q_strlen(l) + 1], l);
			if (valSeparator) {
				s[valSeparator - l] = '\0';
			}

			char* param = trimbuf(s);
			char* value = nullptr;
			if (valSeparator) { // read value if present
				value = trimbuf(s + (valSeparator - l) + 1);
			}

			m_lines.emplace_back(param, value, cline, sectionStart ? LINE_SECTION : LINE_OPTION);
		}
	}

	fclose(fl);
	return true;
}

CReunionConfig* CReunionConfig::createDefault()
{
	CReunionConfig* cfg = new CReunionConfig();

	cfg->m_LogMode = rl_console | rl_logfile;

	cfg->m_AuthVersion = av_reunion2015;
	cfg->m_ServerAnswerType = sat_source;

	cfg->m_bSC2009RevEmuCompat = true;
	cfg->m_bEnableSxEIds = false;
	cfg->m_bEnableGenPrefix2 = false;
	cfg->m_bFixBuggedQuery = true;

	cfg->m_HLTVExceptIP = 0;
	cfg->m_queriesBanLevel = 400;
	cfg->m_floodBanTime = QUERY_FLOOD_DEFAULT_BANTIME;
	cfg->m_allowSplitPackets = false;
	cfg->m_IDClientsLimit = 1;

	cfg->m_SteamIdHashSaltLen = 0;

	memset(cfg->m_SteamIdHashSalt, 0, sizeof(cfg->m_SteamIdHashSalt));
	memset(cfg->m_AuthIdGenOptions, 0, sizeof(cfg->m_AuthIdGenOptions));
	memset(&cfg->m_IPGenOptions, 0, sizeof(cfg->m_IPGenOptions));

	cfg->m_AuthIdGenOptions[CA_HLTV].id_kind = CI_HLTV;

	cfg->m_AuthIdGenOptions[CA_NO_STEAM_47].id_kind = CI_STEAM_ID_LAN;
	cfg->m_AuthIdGenOptions[CA_NO_STEAM_48].id_kind = CI_STEAM_ID_LAN;
	cfg->m_AuthIdGenOptions[CA_SETTI].id_kind = CI_STEAM_ID_LAN;

	cfg->m_AuthIdGenOptions[CA_STEAM].id_kind = CI_REAL_STEAM;
	cfg->m_AuthIdGenOptions[CA_STEAM_PENDING].id_kind = CI_STEAM_ID_PENDING;

	cfg->m_AuthIdGenOptions[CA_STEAM_EMU].id_kind = CI_STEAM_ID_LAN;
	cfg->m_AuthIdGenOptions[CA_OLD_REVEMU].id_kind = CI_STEAM_ID_LAN;
	cfg->m_AuthIdGenOptions[CA_AVSMP].id_kind = CI_STEAM_ID_LAN;

	cfg->m_AuthIdGenOptions[CA_REVEMU].id_kind = CI_REAL_STEAM;
	cfg->m_AuthIdGenOptions[CA_STEAMCLIENT_2009].id_kind = CI_REAL_STEAM;
	cfg->m_AuthIdGenOptions[CA_REVEMU_2013].id_kind = CI_REAL_STEAM;
	cfg->m_AuthIdGenOptions[CA_SXEI].id_kind = CI_REAL_STEAM;

	return cfg;
}

void CReunionConfig::flushCfg()
{
	for (auto &l : m_lines) {
		if (l.param) {
			delete [] l.param;
		}
	}

	m_lines.clear();
}

bool CReunionConfig::parseCfgSection(const char* section, bool (CReunionConfig::*pfnCallback)(const char *param, const char *value, int cline))
{
	bool foundSection = false;
	for (auto &l : m_lines)
	{
		if (l.type != LINE_SECTION &&
			l.type != LINE_END_SECTION) {
			continue;
		}

		if (foundSection)
		{
			if (l.type == LINE_END_SECTION) {
				break;
			}

			(this->*pfnCallback)(l.param, l.value, l.cline);
		}
		else if (!Q_stricmp(section, l.param)) {
			foundSection = true;
		}
	}

	return true;
}

bool CReunionConfig::parseCfgParam()
{
#define REU_CFG_PARSE_IDKIND(paramName, authkind) \
	if (!strcasecmp(paramName, param)) { \
		int i = atoi(value); \
		if (i <= CI_UNKNOWN || i >= CI_MAX || i == CI_RESERVED) { \
			LCPrintf(true, "Invalid %s value '%s'\n", param, value); \
			continue; \
		} \
		if(!IsNumericAuthKind(authkind) && (i == CI_REAL_STEAM || i == CI_REAL_VALVE)) { \
			LCPrintf(true, "Invalid %s value '%s'\n", param, value); \
			continue; \
		} \
		m_AuthIdGenOptions[authkind].id_kind = (client_id_kind) i; \
		continue; \
	}

#define REU_CFG_PARSE_INT(paramName, field, _type, minVal, maxVal) \
	if (!strcasecmp(paramName, param)) { \
		int i = atoi(value); \
		if (i < minVal || i > maxVal) { \
			LCPrintf(true, "Invalid %s value '%s'\n", param, value); \
			continue; \
		} \
		field = (_type) i; \
		continue; \
	}

#define REU_CFG_PARSE_IP(paramName, field) \
	if (!strcasecmp(paramName, param)) { \
		field = inet_addr(value); \
		continue; \
	}

#define REU_CFG_PARSE_BOOL(paramName, field) \
	if (!strcasecmp(paramName, param)) { \
		int i = atoi(value); \
		if (i < 0 || i > 1) { \
			LCPrintf(true, "Invalid %s value '%s'\n", param, value); \
			continue; \
		} \
		field = i ? true : false; \
		continue; \
	}

#define REU_CFG_PARSE_STR(paramName, field) \
	if (!strcasecmp(paramName, param)) { \
		strncpy(field, value, ARRAYSIZE(field) - 1); \
		field[ARRAYSIZE(field) - 1] = '\0'; \
		continue; \
	}

	for (auto &l : m_lines)
	{
		auto param = l.param;
		auto value = l.value;

		if (!param || l.type != LINE_OPTION) {
			continue;
		}

		REU_CFG_PARSE_INT("LoggingMode", m_LogMode, int, rl_none, (rl_console|rl_logfile))

		REU_CFG_PARSE_INT("AuthVersion", m_AuthVersion, auth_version, av_dproto, av_reunion2018)
		REU_CFG_PARSE_INT("ServerInfoAnswerType", m_ServerAnswerType, server_answer_type, sat_source, sat_hybrid)

		REU_CFG_PARSE_BOOL("SC2009_RevCompatMode", m_bSC2009RevEmuCompat)
		REU_CFG_PARSE_BOOL("EnableSXEIdGeneration", m_bEnableSxEIds)
		REU_CFG_PARSE_BOOL("EnableGenPrefix2", m_bEnableGenPrefix2)
		REU_CFG_PARSE_BOOL("FixBuggedQuery", m_bFixBuggedQuery)

		REU_CFG_PARSE_IP("HLTVExcept_IP", m_HLTVExceptIP)
		REU_CFG_PARSE_BOOL("EnableQueryLimiter", m_bEnableQueryLimiter)
		REU_CFG_PARSE_INT("QueryFloodBanLevel", m_queriesBanLevel, int, 0, 8192)
		REU_CFG_PARSE_INT("QueryFloodBanTime", m_floodBanTime, int, 0, 60);
		REU_CFG_PARSE_BOOL("AllowSplitPackets", m_allowSplitPackets)
		REU_CFG_PARSE_INT("IDClientsLimit", m_IDClientsLimit, int, 0, MAX_PLAYERS)

		REU_CFG_PARSE_STR("SteamIdHashSalt", m_SteamIdHashSalt)

		REU_CFG_PARSE_IDKIND("cid_HLTV", CA_HLTV)
		REU_CFG_PARSE_IDKIND("cid_NoSteam47", CA_NO_STEAM_47)
		REU_CFG_PARSE_IDKIND("cid_NoSteam48", CA_NO_STEAM_48)
		REU_CFG_PARSE_IDKIND("cid_Setti", CA_SETTI)
		REU_CFG_PARSE_IDKIND("cid_RevEmu", CA_REVEMU)
		REU_CFG_PARSE_IDKIND("cid_RevEmu2013", CA_REVEMU_2013)
		REU_CFG_PARSE_IDKIND("cid_SC2009", CA_STEAMCLIENT_2009)
		REU_CFG_PARSE_IDKIND("cid_SteamEmu", CA_STEAM_EMU)
		REU_CFG_PARSE_IDKIND("cid_SXEI", CA_SXEI)
		REU_CFG_PARSE_IDKIND("cid_OldRevEmu", CA_OLD_REVEMU)
		REU_CFG_PARSE_IDKIND("cid_Steam", CA_STEAM)
		REU_CFG_PARSE_IDKIND("cid_AVSMP", CA_AVSMP)
		REU_CFG_PARSE_IDKIND("cid_SteamPending", CA_STEAM_PENDING);

		REU_CFG_PARSE_INT("IPGen_Prefix1", m_IPGenOptions.prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("IPGen_Prefix2", m_IPGenOptions.prefix2, int, 0, 255)
		REU_CFG_PARSE_INT("Native_Prefix1", m_AuthIdGenOptions[CA_STEAM].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("RevEmu_Prefix1", m_AuthIdGenOptions[CA_REVEMU].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("SC2009_Prefix1", m_AuthIdGenOptions[CA_STEAMCLIENT_2009].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("RevEmu2013_Prefix1", m_AuthIdGenOptions[CA_REVEMU_2013].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("SteamEmu_Prefix1", m_AuthIdGenOptions[CA_STEAM_EMU].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("OldRevEmu_Prefix1", m_AuthIdGenOptions[CA_OLD_REVEMU].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("Setti_Prefix1", m_AuthIdGenOptions[CA_SETTI].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("AVSMP_Prefix1", m_AuthIdGenOptions[CA_AVSMP].prefix1, int, 0, 255)
		REU_CFG_PARSE_INT("SXEI_Prefix1", m_AuthIdGenOptions[CA_SXEI].prefix1, int, 0, 255)
	}

	m_SteamIdHashSaltLen = strlen(m_SteamIdHashSalt);

	if (m_AuthVersion == av_dproto)
		m_bEnableGenPrefix2 = false;

	if (m_AuthVersion >= av_reunion2018) {
		if (m_SteamIdHashSaltLen < 16) {
			LCPrintf(true, "SteamIdHashSalt is not set or too short\n");
			return false;
		}

		m_bSC2009RevEmuCompat = true;
		m_bEnableGenPrefix2 = true;
	}

	return true;
}

FILE* CReunionConfig::open(const char* fname)
{
	char path[MAX_PATH];
	strncpy(path, gpMetaUtilFuncs->pfnGetPluginPath(PLID), sizeof path - 1);
	path[sizeof path - 1] = '\0';

	char* s = strrchr(path, '/');
	if (s) {
		size_t maxlen = sizeof path - 1 - (s + 1 - path);
		strncpy(s + 1, fname, maxlen);
		path[sizeof path - 1] = '\0';

		FILE *fl = fopen(path, "r");
		if (fl) return fl;
	}

	char gamedir[MAX_PATH];
	g_engfuncs.pfnGetGameDir(gamedir);
	snprintf(path, sizeof path, "./%s/%s", gamedir, fname);
	FILE *fl = fopen(path, "r");
	if (fl) return fl;

	snprintf(path, sizeof path, "./%s", fname);
	return fopen(path, "r");
}

ipv4_error CReunionConfig::ipv4_parse(const char* stringaddr, ipv4_t *result)
{
	bool at_least_one_symbol = false;
	uint8_t  symbol, string_index = 0, result_index = 0;
	uint16_t data = 0;
	uint32_t string_length = Q_strlen(stringaddr);

	result->port = 0; // port any

	while (string_index < string_length)
	{
		symbol = stringaddr[string_index];

		if (isdigit(symbol) != 0)
		{
			symbol -= '0';
			data = data * 10 + symbol;

			if (data > UINT8_MAX)
			{
				return ERROR_IPV4_DATA_OVERFLOW; // 127.0.0.256
			}

			at_least_one_symbol = true;
		}
		else if (symbol == '.')
		{
			if (result_index < 3)
			{
				if (at_least_one_symbol)
				{
					result->ipByte[result_index] = (uint8_t)data;
					data = 0;
					result_index++;
					at_least_one_symbol = false;
				}
				else
				{
					return ERROR_IPV4_NO_SYMBOL; // 127.0..1
				}
			}
			else
			{
				return ERROR_IPV4_INPUT_OVERFLOW; // 127.0.0.1.2
			}
		}
		else if (symbol == ':')
		{
			if (result_index == 3)
			{
				if (at_least_one_symbol)
				{
					result->ipByte[result_index] = (uint8_t)data;
					at_least_one_symbol = false;
					result->port = Q_atoi(&stringaddr[string_index + 1]);
					return ERROR_NO;
				}
				else
				{
					return ERROR_IPV4_NO_SYMBOL; // 127.0..1
				}
			}
			else
			{
				return ERROR_IPV4_INPUT_OVERFLOW; // 127.0.0.1:80:
			}
		}
		else
		{
			return ERROR_IPV4_INVALID_SYMBOL; // 127.*
		}

		string_index++;
	}

	if (result_index == 3)
	{
		if (at_least_one_symbol)
		{
			result->ipByte[result_index] = (uint8_t)data;
			return ERROR_NO;
		}
		else
		{
			return ERROR_IPV4_NOT_ENOUGH_INPUT; // 127.0.0.
		}
	}
	else
	{
		// result_index will be always less than 3
		return ERROR_IPV4_NOT_ENOUGH_INPUT; // 127.0
	}
}
