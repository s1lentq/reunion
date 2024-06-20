#include "precompiled.h"

cvar_t cv_reu_version = {"reu_version", APP_VERSION_STRD, FCVAR_SERVER|FCVAR_EXTDLL, 0, NULL};

cvar_t* pcv_reu_version;
cvar_t* pcv_mp_logecho;
char logstring[2048];

void LCPrintf(bool critical, const char *fmt, ...)
{
	const int prefixlen = 11; //sizeof(LOG_PREFIX) - 1;

	bool bNeedWriteInConsole = critical || g_ReunionConfig->hasLogMode(rl_console);
	bool bNeedWriteInLog = critical || g_ReunionConfig->hasLogMode(rl_logfile);

	if (bNeedWriteInConsole && bNeedWriteInLog && g_RehldsSvs && g_RehldsSvs->IsLogActive())
	{
		if (pcv_mp_logecho && pcv_mp_logecho->value != 0.0)
			bNeedWriteInConsole = false;
	}

	if (!bNeedWriteInConsole && !bNeedWriteInLog) {
		return;
	}

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(logstring + prefixlen, sizeof(logstring) - prefixlen, fmt, argptr);
	va_end(argptr);

	if (bNeedWriteInConsole)
		SERVER_PRINT(logstring);

	if (bNeedWriteInLog)
		ALERT(at_logged, logstring);
}

bool Reunion_Utils_Init() {
	g_engfuncs.pfnCvar_RegisterVariable(&cv_reu_version);

	pcv_reu_version = g_engfuncs.pfnCVarGetPointer(cv_reu_version.name);
	pcv_mp_logecho = g_engfuncs.pfnCVarGetPointer("mp_logecho");

	strcpy(logstring, LOG_PREFIX);
	return true;
}

char* trimbuf(char *str) {
	char *ibuf;

	if (!str) return nullptr;
	for (ibuf = str; *ibuf && (byte)(*ibuf) < (byte)0x80 && isspace(*ibuf); ++ibuf)
		;

	int i = strlen(ibuf);
	if (str != ibuf)
		memmove(str, ibuf, i);

	while (--i >= 0) {
		if (!isspace(str[i]))
			break;
	}
	str[i + 1] = '\0';
	return str;
}

size_t strecpy(char* dest, const char* src, size_t maxlen, const char* exceptChars, const char repChar)
{
	size_t len = 0;

	for (; *src && len < maxlen; src++) {
		char c = *src;
		if (strchr(exceptChars, c)) {
			if (!repChar)
				continue;

			c = repChar;
		}

		len++;
		*dest++ = c;
	}

	*dest = '\0';
	return len;
}

bool IsNumericAuthKind(client_auth_kind authkind) {
	switch (authkind) {
	case CA_STEAM:
	case CA_STEAM_EMU:
	case CA_OLD_REVEMU:
	case CA_REVEMU:
	case CA_STEAMCLIENT_2009:
	case CA_REVEMU_2013:
	case CA_AVSMP:
	case CA_SXEI:
		return true;

	default:
		return false;
	}
}

bool IsUniqueIdKind(client_id_kind idkind)
{
	switch (idkind) {
	case CI_REAL_STEAM:
	case CI_REAL_VALVE:
	case CI_STEAM_BY_IP:
	case CI_VALVE_BY_IP:
		return true;

	default:
		return false;
	}
}

bool IsValidId(uint32 authId) {
	return authId > STEAM_ID_PENDING;
}

bool IsValidSteamTicket(const uint8 *pvSteam2Key, size_t ucbSteam2Key) {
	if (ucbSteam2Key < 16) {
		return false;
	}

	const size_t TicketOff = *(size_t *)pvSteam2Key;
	if (TicketOff < 0 || (TicketOff + 16) > ucbSteam2Key) {
		return false;
	}

	const int* cc = (int *)(pvSteam2Key + 4 + TicketOff);
	return cc[0] >= 0 && cc[1] >= 0;
}

bool IsHddsnNumber(const char* authstring)
{
	for (const char* c = authstring; *c; c++) {
		// HHDSN have spaces or letters
		if (*c < '0' || *c > '9')
			return true;
	}

	return strtoull(authstring, nullptr, 10) >= UINT32_MAX; // SSD
}

void util_console_print(const char* fmt, ...)
{
	char buf[1024];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(buf, sizeof buf, fmt, argptr);
	va_end(argptr);

	SERVER_PRINT(buf);
}

void util_syserror(const char* fmt, ...)
{
	va_list	argptr;
	char buf[4096];
	va_start(argptr, fmt);
	vsnprintf(buf, ARRAYSIZE(buf) - 1, fmt, argptr);
	buf[ARRAYSIZE(buf) - 1] = 0;
	va_end(argptr);

	LCPrintf(true, "ERROR: %s", buf);
	*(int *)0 = 0;
}

void util_execute_cmd(const char* fmt, ...)
{
	va_list	argptr;
	char buf[4096];
	va_start(argptr, fmt);
	vsnprintf(buf, ARRAYSIZE(buf) - 1, fmt, argptr);
	buf[ARRAYSIZE(buf) - 1] = 0;
	va_end(argptr);

	g_engfuncs.pfnServerCommand(buf);
	g_engfuncs.pfnServerExecute();
}

void util_debug_log(const char* fmt, ...)
{
	va_list	argptr;
	char buf[4096];
	va_start(argptr, fmt);
	vsnprintf(buf, ARRAYSIZE(buf) - 1, fmt, argptr);
	buf[ARRAYSIZE(buf) - 1] = 0;
	va_end(argptr);

	FILE* fp = fopen("reunion_debug.log", "a");

	if (fp)
	{
		fprintf(fp, "%f: %s", g_RehldsFuncs->GetRealTime(), buf);
		fclose(fp);
	}
}

const char* util_netadr_to_string(const netadr_t& adr, bool without_port)
{
	return util_adr_to_string(adr.ip, without_port ? 0 : bswap(adr.port));
}

const char* util_adr_to_string(uint32 ip, uint16 port)
{
	return util_adr_to_string((const byte *)&ip, port);
}

const char* util_adr_to_string(const byte *ip, uint16 port)
{
	static char buf[32];
	snprintf(buf, sizeof buf, port ? "%u.%u.%u.%u:%u" : "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3], port);
	return buf;
}

void util_client_printf(IGameClient* client, const char *fmt, ...)
{
	char string[1024];

	va_list va;
	va_start(va, fmt);
	vsnprintf(string, sizeof string, fmt, va);
	va_end(va);

	string[sizeof string - 1] = '\0';

	auto buf = client->GetNetChan()->GetMessageBuf();
	g_RehldsFuncs->MSG_WriteByte(buf, svc_print);
	g_RehldsFuncs->MSG_WriteString(buf, string);
}

static void util_oob_print(const netadr_t& adr, char* format, ...)
{
	char string[1024];

	va_list argptr;
	va_start(argptr, format);
	size_t len = vsnprintf(string + 4, sizeof string - 4, format, argptr);
	va_end(argptr);

	*(int *)string = -1;
	g_RehldsFuncs->NET_SendPacket(4 + len + 1, string, adr);
}

static const unsigned char mungify_table2[] =
{
	0x05, 0x61, 0x7A, 0xED,
	0x1B, 0xCA, 0x0D, 0x9B,
	0x4A, 0xF1, 0x64, 0xC7,
	0xB5, 0x8E, 0xDF, 0xA0
};

static void util_munge2(unsigned char *data, int len, int seq)
{
	int i;
	int mungelen;
	int c;
	int *pc;
	unsigned char *p;
	int j;

	mungelen = len & ~3;
	mungelen /= 4;

	for (i = 0; i < mungelen; i++) {
		pc = (int *)&data[i * 4];
		c = *pc;
		c ^= ~seq;
		c = bswap(c);

		p = (unsigned char *)&c;
		for (j = 0; j < 4; j++) {
			*p++ ^= (0xa5 | (j << j) | j | mungify_table2[(i + j) & 0x0f]);
		}

		c ^= seq;
		*pc = c;
	}
}

void util_connect_and_kick(const netadr_t& adr, const char* reason)
{
	// send fake connect confirmation
	util_oob_print(adr, "B 0 0.0.0.0 1 0");

	// it will be first packet
	const int outgoing_sequence = 1;
	const int incoming_sequence = 0;

	uint8 buf[128];
	CSizeBuf szbuf(buf, sizeof buf);
	szbuf.WriteLong(outgoing_sequence);
	szbuf.WriteLong(incoming_sequence);
	szbuf.WriteChar(svc_disconnect);
	szbuf.WriteString(reason);

	// fill up to 16+ bytes
	while (szbuf.GetCurSize() < 16)
		szbuf.WriteByte(svc_nop);

	// munge
	util_munge2(szbuf.GetData() + 8, szbuf.GetCurSize() - 8, outgoing_sequence);

	// send
	g_RehldsFuncs->NET_SendPacket(szbuf.GetCurSize(), szbuf.GetData(), adr);

	util_console_print("Refusing connection %s from server\nReason:  %s", util_netadr_to_string(adr), reason);
}
