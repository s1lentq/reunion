#include "precompiled.h"

cvar_t cv_dp_r_protocol = {"dp_r_protocol", "0", FCVAR_EXTDLL, 0, NULL};
cvar_t cv_dp_r_id_provider = {"dp_r_id_provider", "0", FCVAR_EXTDLL, 0, NULL};
cvar_t cv_reunion_api = {"reunion_api", "0", FCVAR_EXTDLL, 0, NULL};

cvar_t *pcv_dp_r_protocol;
cvar_t *pcv_dp_r_id_provider;
cvar_t *pcv_reunion_api;

class CReunionApiImpl : public IReunionApi
{
public:
	CReunionApiImpl();

	int GetMajorVersion() override;
	int GetMinorVersion() override;

	int GetClientProtocol(int index) override;
	dp_authkind_e GetClientAuthtype(int index) override;

	size_t GetClientAuthdata(int index, void *data, int maxlen) override;
	const char *GetClientAuthdataString(int index, char *data, int maxlen) override;

	void GetLongAuthId(int index, unsigned char (&authId)[LONG_AUTHID_LEN]) override;
	reu_authkey_kind GetAuthKeyKind(int index) override;

	void SetConnectTime(int index, double time) override;
	USERID_t *GetSerializedId(int index) const override;
	USERID_t *GetStorageId(int index) const override;
	uint64 GetDisplaySteamId(int index) const override;

protected:
	// Safe checks
	// Just make sure that a reunion will never cause segfault
	bool IsValidIndex(int index) const;

private:
	int version_major = REUNION_API_VERSION_MAJOR;
	int version_minor = REUNION_API_VERSION_MINOR;
};

CReunionApiImpl g_ReunionApi;

int CReunionApiImpl::GetMajorVersion()
{
	return version_major;
}

int CReunionApiImpl::GetMinorVersion()
{
	return version_minor;
}

dp_authkind_e Reunion_GetPlayerAuthkind(CReunionPlayer* plr)
{
	dp_authkind_e dpAuthKind;

	switch (plr->GetAuthKind())
	{
	case CA_HLTV:
		dpAuthKind = DP_AUTH_HLTV;
		break;

	case CA_NO_STEAM_47:
	case CA_NO_STEAM_48:
	case CA_SETTI:
		dpAuthKind = DP_AUTH_DPROTO;
		break;

	case CA_STEAM:
	case CA_STEAM_PENDING:
		dpAuthKind = DP_AUTH_STEAM;
		break;

	case CA_STEAM_EMU:
		dpAuthKind = DP_AUTH_STEAMEMU;
		break;

	case CA_OLD_REVEMU:
		dpAuthKind = DP_AUTH_OLDREVEMU;
		break;

	case CA_REVEMU:
		dpAuthKind = DP_AUTH_REVEMU;
		break;

	case CA_STEAMCLIENT_2009:
		dpAuthKind = DP_AUTH_SC2009;
		break;

	case CA_REVEMU_2013:
		dpAuthKind = DP_AUTH_REVEMU2013;
		break;

	case CA_AVSMP:
		dpAuthKind = DP_AUTH_AVSMP;
		break;

	case CA_SXEI:
		dpAuthKind = DP_AUTH_SXEI;
		break;

	default:
		dpAuthKind = DP_AUTH_NONE;
		break;
	}

	return dpAuthKind;
}

CReunionApiImpl::CReunionApiImpl()
{

}

int CReunionApiImpl::GetClientProtocol(int index)
{
	if (!IsValidIndex(index)) return 0;
	return g_Players[index]->getProcotol();
}

dp_authkind_e CReunionApiImpl::GetClientAuthtype(int index)
{
	if (!IsValidIndex(index)) return DP_AUTH_NONE;
	return Reunion_GetPlayerAuthkind(g_Players[index]);
}

size_t CReunionApiImpl::GetClientAuthdata(int index, void *data, int maxlen)
{
	if (!IsValidIndex(index)) return 0;
	return g_Players[index]->getRawAuthData(data, maxlen);
}

const char *CReunionApiImpl::GetClientAuthdataString(int index, char *data, int maxlen)
{
	if (!IsValidIndex(index)) return ""; // better nullptr?

	auto* plr = g_Players[index];
	char raw[MAX_AUTHKEY_LEN];
	size_t rawlen;

	rawlen = plr->getRawAuthData(raw, sizeof raw);

	switch (plr->GetAuthKind())
	{
	/*case CA_HLTV:
	case CA_NO_STEAM_47:
	case CA_NO_STEAM_48:
	case CA_SETTI:
	case CA_STEAM:
	case CA_STEAM_PENDING:*/

	case CA_STEAM_EMU:
	case CA_OLD_REVEMU:
	case CA_AVSMP:
		snprintf(data, maxlen, "%u", *(uint32 *)raw);
		break;

	case CA_REVEMU:
	case CA_STEAMCLIENT_2009:
	case CA_REVEMU_2013:
	case CA_SXEI:
		snprintf(data, maxlen, "%.*s", rawlen, raw);
		break;

	default:
		if (maxlen != 0)
			data[0] = '\0';
		break;
	}

	return data;
}

void CReunionApiImpl::GetLongAuthId(int index, unsigned char (&authId)[LONG_AUTHID_LEN])
{
	auto player = g_Players[index];

	sha_byte raw[256];
	size_t len = player->getRawAuthData(raw, sizeof raw);

	if (!len) {
		uint32_t* uptr = (uint32_t *)raw;
		uptr[0] = player->GetAuthKind();
		uptr[1] = player->getDisplaySteamId().GetAccountID();
		len = sizeof(uint32_t) * 2;
	}

	sha2 hSha;
	hSha.Init(sha2::enuSHA256);
	hSha.Update(raw, len);
	hSha.End();

	int shaLen;
	memcpy(authId, hSha.RawHash(shaLen), LONG_AUTHID_LEN);
}

reu_authkey_kind CReunionApiImpl::GetAuthKeyKind(int index)
{
	return REU_AK_UNKNOWN;
}

USERID_t *CReunionApiImpl::GetSerializedId(int index) const {
	if (!IsValidIndex(index)) return NULL;
	return g_Players[index]->getSerializedId();
}

USERID_t *CReunionApiImpl::GetStorageId(int index) const {
	if (!IsValidIndex(index)) return NULL;
	return g_Players[index]->getStorageId();
}

uint64 CReunionApiImpl::GetDisplaySteamId(int index) const {
	if (!IsValidIndex(index)) return 0;
	return g_Players[index]->getDisplaySteamId().ConvertToUint64();
}

void CReunionApiImpl::SetConnectTime(int index, double time) {
	if (!IsValidIndex(index)) return;
	g_Players[index]->setConnectionTime(time);
}

bool CReunionApiImpl::IsValidIndex(int index) const
{
	if (index < 0 || index >= g_RehldsSvs->GetMaxClientsLimit()) {
		LCPrintf(false, "Invalid player index expected 0-%d, real %d\n", g_RehldsSvs->GetMaxClientsLimit() - 1, index);
		return false;
	}

	return true;
}

void Cmd_dp_ClientInfo()
{
	int ArgCount = g_engfuncs.pfnCmd_Argc();
	if (ArgCount > 1) {
		int clientId = atoi(g_engfuncs.pfnCmd_Argv(1)) - 1;
		if (clientId >= 0 && clientId < g_ServerInfo->getMaxPlayers()) {
			CReunionPlayer* plr = g_Players[clientId];

			if (plr->getClient()->IsConnected()) {
				char buf[32];

				sprintf(buf, "%d", plr->getProcotol());
				g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_protocol, buf);

				sprintf(buf, "%d", Reunion_GetPlayerAuthkind(plr));
				g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_id_provider, buf);
			}
			else {
				g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_protocol, "0");
				g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_id_provider, "0");
			}
		}
	}
	else {
		g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_protocol, "-1");
		g_engfuncs.pfnCvar_DirectSet(pcv_dp_r_id_provider, "-1");
	}
}

void Cmd_reu_Ban()
{
	if (CMD_ARGC() == 3)
		g_ServerInfo->banAddress(CMD_ARGV(1), strtoul(CMD_ARGV(2), nullptr, 10));
	else
		util_console_print("Usage: reu_ban <ip> <time>\n");
}

void Cmd_reu_Unban()
{
	if (CMD_ARGC() == 2)
		g_ServerInfo->unbanAddress(CMD_ARGV(1));
	else
		util_console_print("Usage: reu_unban <ip>\n");
}

void Cmd_reu_Bans()
{
	g_ServerInfo->printBans();
}

void Reunion_Api_Init()
{
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_r_protocol);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_r_id_provider);

	pcv_dp_r_protocol = g_engfuncs.pfnCVarGetPointer("dp_r_protocol");
	pcv_dp_r_id_provider = g_engfuncs.pfnCVarGetPointer("dp_r_id_provider");

	g_engfuncs.pfnAddServerCommand("dp_clientinfo", &Cmd_dp_ClientInfo);
	g_engfuncs.pfnAddServerCommand("reu_ban", &Cmd_reu_Ban);
	g_engfuncs.pfnAddServerCommand("reu_unban", &Cmd_reu_Unban);
	g_engfuncs.pfnAddServerCommand("reu_bans", &Cmd_reu_Bans);

	g_RehldsFuncs->RegisterPluginApi("reunion", &g_ReunionApi);
}
