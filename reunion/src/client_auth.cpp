#include "precompiled.h"

client_auth_context_t* g_CurrentAuthContext = nullptr;

cvar_t cv_dp_rejmsg_steam = { "dp_rejmsg_steam", "Sorry, legit clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_nosteam47 = { "dp_rejmsg_nosteam47", "Sorry, no-steam p47 clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_nosteam48 = { "dp_rejmsg_nosteam48", "Sorry, no-steam p48 clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_hltv = { "dp_rejmsg_hltv", "Sorry, HLTV is not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_pending = { "dp_rejmsg_pending", "Sorry, unauthorized clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_revemu = { "dp_rejmsg_revemu", "Sorry, RevEmu clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_revemu2013 = { "dp_rejmsg_revemu2013", "Sorry, RevEmu2013 clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_steamemu = { "dp_rejmsg_steamemu", "Sorry, SteamEmu clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_oldrevemu = { "dp_rejmsg_oldrevemu", "Sorry, Old RevEmu clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_avsmp = { "dp_rejmsg_avsmp", "Sorry, AVSMP clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_revemu_sc2009 = { "dp_rejmsg_revemu_sc2009", "Sorry, revEmu/SC2009 clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };
cvar_t cv_dp_rejmsg_sxei = { "dp_rejmsg_sxei", "Sorry, sXe Injected clients are not allowed on this server", FCVAR_EXTDLL, 0, NULL };

cvar_t *pcv_dp_rejmsg_steam;
cvar_t *pcv_dp_rejmsg_nosteam47;
cvar_t *pcv_dp_rejmsg_nosteam48;
cvar_t *pcv_dp_rejmsg_hltv;
cvar_t *pcv_dp_rejmsg_pending;
cvar_t *pcv_dp_rejmsg_revemu;
cvar_t *pcv_dp_rejmsg_steamemu;
cvar_t *pcv_dp_rejmsg_oldrevemu;
cvar_t *pcv_dp_rejmsg_avsmp;
cvar_t *pcv_dp_rejmsg_revemu_sc2009;
cvar_t *pcv_dp_rejmsg_revemu2013;
cvar_t *pcv_dp_rejmsg_sxei;

cvar_t *pcv_sv_contact;

void Reunion_Reject_Deprecated(client_auth_kind authkind) {
	const char* kickMessage;

	switch (authkind) {
	case CA_HLTV: kickMessage = pcv_dp_rejmsg_hltv->string; break;
	case CA_NO_STEAM_47: kickMessage = pcv_dp_rejmsg_nosteam47->string; break;
	case CA_NO_STEAM_48: kickMessage = pcv_dp_rejmsg_nosteam48->string; break;
	case CA_SETTI: kickMessage = "Setti scanner is not allowed there"; break;
	case CA_STEAM: kickMessage = pcv_dp_rejmsg_steam->string; break;
	case CA_STEAM_PENDING: kickMessage = pcv_dp_rejmsg_pending->string; break;
	case CA_STEAM_EMU: kickMessage = pcv_dp_rejmsg_steamemu->string; break;
	case CA_OLD_REVEMU: kickMessage = pcv_dp_rejmsg_oldrevemu->string; break;
	case CA_REVEMU: kickMessage = pcv_dp_rejmsg_revemu->string; break;
	case CA_STEAMCLIENT_2009: kickMessage = pcv_dp_rejmsg_revemu_sc2009->string; break;
	case CA_REVEMU_2013: kickMessage = pcv_dp_rejmsg_revemu2013->string; break;
	case CA_AVSMP: kickMessage = pcv_dp_rejmsg_avsmp->string; break;
	case CA_SXEI: kickMessage = pcv_dp_rejmsg_sxei->string; break;
	default:
		kickMessage = "Unknown client type";
		break;
	}

	g_RehldsFuncs->RejectConnection(g_CurrentAuthContext->adr, "%s", kickMessage);
}

void SaltSteamId(authdata_t* authdata) {
	// We hash [steamid + raw ticket data + salt]
	// This makes impossible to substitute exact id if you know only old, not hashed id
	byte buf[MAX_HASHDATA_LEN];
	CSizeBuf szbuf(buf, sizeof buf);

	if (g_ReunionConfig->getAuthVersion() < av_reunion2018)
		szbuf.WriteLong(authdata->steamId);
	if (g_ReunionConfig->getAuthVersion() > av_dproto)
		szbuf.Write(authdata->authKey, authdata->authKeyLen);

	szbuf.Write(g_ReunionConfig->getSteamIdSalt(), g_ReunionConfig->getSteamIdSaltLen());

	// Hash
	sha2 hSha;
	hSha.Init(sha2::enuSHA256);
	hSha.Update(szbuf.GetData(), szbuf.GetCurSize());
	hSha.End();

	// Get hash
	int shaLen;
	const char *hash = hSha.RawHash(shaLen);

	// Set new SteamID
	authdata->steamId = *(unsigned int *)(hash + 8);
}

uint64_t SteamByIp(uint32_t ip)
{
	uint32_t accId;

	if (g_ReunionConfig->getAuthVersion() >= av_reunion2018)
		MurmurHash3_x86_32(&ip, sizeof(ip), IPGEN_KEY, &accId);
	else
		accId = ip ^ IPGEN_KEY;

	return CSteamID(accId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64();
}

bool Reunion_FinishClientAuth(CReunionPlayer* reunionPlr, USERID_t* userid, client_auth_context_t* ctx)
{
	client_auth_kind authkind;

	if (!ctx->authentificatedInSteam) {
		// native auth failed, try authorize by emulators
		authdata_t authdata;
		authdata.authTicket = ctx->authTicket;
		authdata.ticketLen = ctx->authTicketLen;
		authdata.protinfo = ctx->protinfo;
		authdata.protocol = ctx->protocol;
		authdata.userinfo = ctx->userinfo;
		authdata.ipaddr = ctx->ipaddr;
		authdata.port = bswap(ctx->adr->port);

		authkind = Reunion_Authorize_Client(&authdata);
		if (authkind == CA_UNKNOWN) {
			g_RehldsFuncs->RejectConnection(ctx->adr, "Client authorization failed");
			reunionPlr->clear();
			return false;
		}

		LCPrintf(false, "%s (%s) authorized as %s\n", g_engfuncs.pfnInfoKeyValue(ctx->userinfo, "name"), util_adr_to_string(authdata.ipaddr, authdata.port), Reunion_GetAuthorizerName(authkind));

		uint32_t steamId = authdata.steamId;
		if (IsNumericAuthKind(authkind)) {
			// check for pending id
			if (!IsValidId(authdata.steamId)) {
				authkind = CA_STEAM_PENDING;
			}
			else {
				// salt steamid
				if (g_ReunionConfig->getSteamIdSaltLen()) {
					SaltSteamId(&authdata);

					// set second prefix
					if (g_ReunionConfig->getEnableGenPrefix2())
						authdata.steamId = rotl(authdata.steamId, 1);
					else
						authdata.steamId <<= 1;
				}
			}
		}

		reunionPlr->setRawAuthData(authdata.authKeyKind, authdata.authKey, authdata.authKeyLen);

		userid->idtype      = authdata.idtype;
		userid->m_SteamID   = CSteamID(authdata.steamId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeGameServer).ConvertToUint64();
		userid->clientip    = authdata.ipaddr;

		USERID_t storageId;
		storageId.idtype    = authdata.idtype;
		storageId.m_SteamID = CSteamID(steamId, k_unSteamUserDesktopInstance, k_EUniversePublic, k_EAccountTypeGameServer).ConvertToUint64();
		storageId.clientip  = authdata.ipaddr;

		reunionPlr->setStorageId(&storageId);

	} else {
		authkind = CA_STEAM;
		reunionPlr->setRawAuthData(AK_STEAM, nullptr, 0);
	}

	// check for pending id
	if (IsNumericAuthKind(authkind) && !IsValidId(userid->m_SteamID)) {
		authkind = CA_STEAM_PENDING;
	}

	// add prefix
	client_id_kind idkind = g_ReunionConfig->getIdGenOptions(authkind)->id_kind;
	switch (idkind) {
	// check for deprecation
	case CI_DEPRECATED:
		// allow HLTV from configured port
		if (authkind == CA_HLTV && ctx->ipaddr == g_ReunionConfig->getHLTVExceptIP()) {
			idkind = CI_HLTV;
		}
		else {
			if (ctx->authentificatedInSteam) {
				g_ISteamGameServer->SendUserDisconnect(userid->m_SteamID);
				ctx->authentificatedInSteam = false;
			}
			Reunion_Reject_Deprecated(authkind);
			reunionPlr->clear();
			return false;
		}
		break;

	// check for auth by ip
	case CI_VALVE_BY_IP:
		userid->idtype = AUTH_IDTYPE_VALVE;
	case CI_STEAM_BY_IP:
		userid->m_SteamID = SteamByIp(ctx->ipaddr);
		break;

	case CI_REAL_VALVE:
	case CI_VALVE_ID_LAN:
	case CI_VALVE_ID_PENDING:
		userid->idtype = AUTH_IDTYPE_VALVE;
		break;

	default:
		break;
	}

	// set reunion_player's values
	reunionPlr->authenticated(ctx->protocol, idkind, authkind, userid);

	if (IsValidId(userid->m_SteamID) && IsUniqueIdKind(idkind)) {
		// check for duplicate steamid
		if (!ctx->authentificatedInSteam) {
			if (Reunion_IsSteamIdInUse(reunionPlr)) {
				util_connect_and_kick(*ctx->adr, "Your SteamID is already in use on this server\n");
				reunionPlr->clear();
				return false;
			}
		}
		else {
			// remove nosteamers with the same id
			Reunion_RemoveDuplicateSteamIds(reunionPlr);
		}

		// check if client being connected is banned
		if (g_RehldsFuncs->FilterUser(userid)) {
			// disconnect from steam if authorized
			if (ctx->authentificatedInSteam) {
				g_ISteamGameServer->SendUserDisconnect(userid->m_SteamID);
				ctx->authentificatedInSteam = false;
			}

			// kick
			util_connect_and_kick(*ctx->adr, "You have been banned from this server.\n");
			reunionPlr->clear();
			return false;
		}
	}

	// notify steam that someone has joined the server (if native auth was failed)
	if (!ctx->authentificatedInSteam) {
		uint64 steamUnauthenticatedIDUser = g_ISteamGameServer->CreateUnauthenticatedUserConnection().ConvertToUint64();
		reunionPlr->setUnauthenticatedId(steamUnauthenticatedIDUser);
		ctx->unauthenticatedConnection = true;
	}

	return true;
}

void SV_ConnectClient_hook(IRehldsHook_SV_ConnectClient* chain) {
#ifndef NDEBUG
	if (g_CurrentAuthContext != NULL) {
		LCPrintf(true, "WARNING: %s: recursive call\n", __FUNCTION__);
		chain->callNext();
		return;
	}
#endif

	client_auth_context_t ctx;

	if (!g_ISteamGameServer)
		g_ISteamGameServer = g_RehldsApi->GetServerData()->GetSteamGameServer();

	g_CurrentAuthContext = &ctx;
	chain->callNext();
	g_CurrentAuthContext = nullptr;
}

// #1 Hook in SV_ConnectClient
int SV_CheckProtocol_hook(IRehldsHook_SV_CheckProtocol* chain, netadr_t *adr, int nProtocol)
{
#ifndef NDEBUG
	if (g_CurrentAuthContext == NULL) {
		LCPrintf(true, "WARNING: %s is called outside auth context\n", __FUNCTION__);
		return chain->callNext(adr, nProtocol);
	}
#endif

	if (adr == NULL) {
		return chain->callNext(adr, nProtocol);
	}

	g_CurrentAuthContext->adr = adr;
	g_CurrentAuthContext->ipaddr = *(uint32 *)&adr->ip[0];
	g_CurrentAuthContext->protocol = nProtocol;

	if (nProtocol < 47) {
		g_RehldsFuncs->RejectConnection(
			adr,
			"This server is using a newer protocol ( %i ) than your client ( %i ).  You should check for updates to your client.\n",
			48,
			nProtocol);

		return FALSE;
	}

	if (nProtocol > 48) {
		const char *contact = (pcv_sv_contact->string[0] != '\0') ? pcv_sv_contact->string : "(no email address specified)";

		g_RehldsFuncs->RejectConnection(
			adr,
			"This server is using an older protocol ( %i ) than your client ( %i ).  If you believe this server is outdated, you can contact the server administrator at %s.\n",
			48,
			nProtocol,
			contact);

		return FALSE;
	}

	return TRUE;
}

// #2 Hook in SV_ConnectClient
int SV_CheckKeyInfo_hook(IRehldsHook_SV_CheckKeyInfo* chain, netadr_t *adr, char *protinfo, short unsigned int *port, int *pAuthProtocol, char *pszRaw, char *cdkey)
{
#ifndef NDEBUG
	if (g_CurrentAuthContext == NULL) {
		LCPrintf(true, "WARNING: %s is called outside auth context\n",__FUNCTION__);
		return chain->callNext(adr, protinfo, port, pAuthProtocol, pszRaw, cdkey);
	}
#endif

	g_CurrentAuthContext->protinfo = protinfo;
	g_CurrentAuthContext->pAuthProto = pAuthProtocol;

	int authProto = atoi(g_engfuncs.pfnInfoKeyValue(protinfo, "prot"));
	if (authProto <= 0 || authProto > 4) {
		g_RehldsFuncs->RejectConnection(adr, "Invalid connection.\n");
		return FALSE;
	}

	*port = PORT_CLIENT;
	*pAuthProtocol = PROTOCOL_STEAM; // we want to Steam_NotifyClientConnect be called for every client

	strncpy(pszRaw, g_engfuncs.pfnInfoKeyValue(protinfo, "raw"), 32);
	pszRaw[33] = '\0';

	strncpy(cdkey, g_engfuncs.pfnInfoKeyValue(protinfo, "cdkey"), 32);
	cdkey[33] = '\0';

	return TRUE;
}

// #3 Hook in SV_ConnectClient
int SV_FinishCertificateCheck_hook(IRehldsHook_SV_FinishCertificateCheck* chain, netadr_t *adr, int nAuthProtocol, char *szRawCertificate, char *userinfo)
{
#ifndef NDEBUG
	if (g_CurrentAuthContext == NULL) {
		LCPrintf(true, "WARNING: %s is called outside auth context\n", __FUNCTION__);
		return chain->callNext(adr, nAuthProtocol, szRawCertificate, userinfo);
	}
#endif

	g_CurrentAuthContext->userinfo = userinfo;

	sizebuf_t* pNetMessage = g_RehldsFuncs->GetNetMessage();
	int* pMsgReadCount = g_RehldsFuncs->GetMsgReadCount();

	// avoid "invalid steam certificate length" error
	if (*pMsgReadCount == pNetMessage->cursize) {
		pNetMessage->cursize += 1;
	}

	return TRUE;
}

// #4 Hook in SV_ConnectClient
qboolean Steam_NotifyClientConnect_hook(IRehldsHook_Steam_NotifyClientConnect* chain, IGameClient *cl, const void *pvSteam2Key, unsigned int ucbSteam2Key)
{
#ifndef NDEBUG
	if (g_CurrentAuthContext == NULL) {
		LCPrintf(true, "WARNING: %s is called outside auth context\n", __FUNCTION__);
		return chain->callNext(cl, pvSteam2Key, ucbSteam2Key);
	}
#endif

	uint8* ticket = (uint8 *)pvSteam2Key;
	g_CurrentAuthContext->authTicket = ticket;
	g_CurrentAuthContext->authTicketLen = ucbSteam2Key;

	// fast no-steam check
	bool authRes = IsValidSteamTicket(ticket, ucbSteam2Key);

	// try authorize in steam
	if (authRes) {
		authRes = chain->callNext(cl, pvSteam2Key, ucbSteam2Key) ? true : false;
	}

	g_CurrentAuthContext->authentificatedInSteam = authRes;

	return Reunion_FinishClientAuth(GetReunionPlayerByClientPtr(cl), cl->GetNetworkUserID(), g_CurrentAuthContext) ? TRUE : FALSE;
}

// Replace protocol number in svc_serverinfo
void SV_SendServerinfo_hook(IRehldsHook_SV_SendServerinfo* chain, sizebuf_t *msg, IGameClient* cl)
{
	int startWritePos = msg->cursize;
	chain->callNext(msg, cl);

	for (int i = startWritePos; i < msg->maxsize - 1; i++) {
		if (msg->data[i] == svc_serverinfo) {
			msg->data[i + 1] = (uint8)GetReunionPlayerByClientPtr(cl)->getProcotol();
			break;
		}
	}
}

// Used mostly in id bans stuff
char *SV_GetIDString_hook(IRehldsHook_SV_GetIDString* chain, USERID_t *id)
{
	static char idstring[64];

	CReunionPlayer* plr = GetReunionPlayerByUserIdPtr(id);
	if (plr)
		return plr->GetSteamId();

	uint32 accId = (uint32)id->m_SteamID;
	switch (id->idtype) {
	case AUTH_IDTYPE_STEAM:
		if (accId == 0) {
			strcpy(idstring, "STEAM_ID_LAN");
		}
		else if (accId == 1) {
			strcpy(idstring, "STEAM_ID_PENDING");
		}
		else {
			sprintf(idstring, "STEAM_%u:%u:%u", 0, accId & 1, accId >> 1);
		}
		break;

	case AUTH_IDTYPE_VALVE:
		if (accId == 0) {
			strcpy(idstring, "VALVE_ID_LAN");
		}
		else if (accId == 1) {
			strcpy(idstring, "VALVE_ID_PENDING");
		}
		else {
			sprintf(idstring, "VALVE_%u:%u:%u", 0, accId & 1, accId >> 1);
		}
		break;

	case AUTH_IDTYPE_LOCAL:
		return "HLTV";

	default:
		return "UNKNOWN";
	}

	return idstring;
}

// Used in id bans stuff and for detecting steamid duplicates
qboolean SV_CompareUserID_hook(IRehldsHook_SV_CompareUserID* chain, USERID_t *id1, USERID_t *id2)
{
	CReunionPlayer* plr1 = GetReunionPlayerByUserIdPtr(id1);
	CReunionPlayer* plr2 = GetReunionPlayerByUserIdPtr(id2);

	if (plr1) {
		id1 = plr1->getSerializedId();
	}

	if (plr2) {
		id2 = plr2->getSerializedId();
	}

	return chain->callNext(id1, id2); // both IDs are serialized
}

void SerializeSteamId_hook(IRehldsHook_SerializeSteamId* chain, USERID_t* id, USERID_t* serialized)
{
	CReunionPlayer* plr = GetReunionPlayerByUserIdPtr(id);

	if (plr) {
		id = plr->getSerializedId();
	}

	chain->callNext(id, serialized);
}

void Steam_NotifyClientDisconnect_hook(IRehldsHook_Steam_NotifyClientDisconnect* chain, IGameClient* cl)
{
	USERID_t *userid = cl->GetNetworkUserID();
	uint64 steamIDUser = userid->m_SteamID;

	CReunionPlayer *player = GetReunionPlayerByClientPtr(cl);
	bool isUnauthenticatedId = player->isUnauthenticatedId();
	if (isUnauthenticatedId) {
		userid->m_SteamID = player->getUnauthenticatedId();
	}

	chain->callNext(cl);

	if (isUnauthenticatedId) {
		userid->m_SteamID = steamIDUser;
	}

	player->clear();
}

bool Steam_GSBUpdateUserData_hook(IRehldsHook_Steam_GSBUpdateUserData* chain, uint64 steamIDUser, const char* pchPlayerName, uint32 uScore)
{
	CReunionPlayer *plr = GetReunionPlayerBySteamdId(steamIDUser);
	if (plr && plr->isUnauthenticatedId())
		steamIDUser = plr->getUnauthenticatedId();

	return chain->callNext(steamIDUser, pchPlayerName, uScore);
}

uint64 Steam_GSGetSteamID()
{
	return GAMESERVER_STEAMID; // not NULL
//	return g_ISteamGameServer->GetSteamID().ConvertToUint64();
}

uint64 Steam_GSGetSteamID_hook(IRehldsHook_Steam_GSGetSteamID* chain)
{
	return Steam_GSGetSteamID();
}

void SV_Frame_hook(IRehldsHook_SV_Frame* chain) {
	chain->callNext();
	g_ServerInfo->startFrame();
}

bool Reunion_Auth_Init() {

	//
	// Cvars
	//
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_steam);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_nosteam47);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_nosteam48);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_hltv);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_pending);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_revemu);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_revemu2013);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_steamemu);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_oldrevemu);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_avsmp);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_revemu_sc2009);
	g_engfuncs.pfnCvar_RegisterVariable(&cv_dp_rejmsg_sxei);

	pcv_dp_rejmsg_steam = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_steam.name);
	pcv_dp_rejmsg_nosteam47 = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_nosteam47.name);
	pcv_dp_rejmsg_nosteam48 = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_nosteam48.name);
	pcv_dp_rejmsg_hltv = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_hltv.name);
	pcv_dp_rejmsg_pending = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_pending.name);
	pcv_dp_rejmsg_revemu = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_revemu.name);
	pcv_dp_rejmsg_revemu2013 = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_revemu2013.name);
	pcv_dp_rejmsg_steamemu = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_steamemu.name);
	pcv_dp_rejmsg_oldrevemu = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_oldrevemu.name);
	pcv_dp_rejmsg_avsmp = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_avsmp.name);
	pcv_dp_rejmsg_revemu_sc2009 = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_revemu_sc2009.name);
	pcv_dp_rejmsg_sxei = g_engfuncs.pfnCVarGetPointer(cv_dp_rejmsg_sxei.name);

	pcv_sv_contact = g_engfuncs.pfnCVarGetPointer("sv_contact");

	//
	// Install hooks
	//
	g_RehldsHookchains->SV_ConnectClient()->registerHook(&SV_ConnectClient_hook);
	g_RehldsHookchains->SV_CheckProtocol()->registerHook(&SV_CheckProtocol_hook);
	g_RehldsHookchains->SV_SendServerinfo()->registerHook(&SV_SendServerinfo_hook);
	g_RehldsHookchains->SV_CheckKeyInfo()->registerHook(&SV_CheckKeyInfo_hook);
	g_RehldsHookchains->SV_FinishCertificateCheck()->registerHook(&SV_FinishCertificateCheck_hook);
	g_RehldsHookchains->Steam_NotifyClientConnect()->registerHook(&Steam_NotifyClientConnect_hook);

	g_RehldsHookchains->SV_GetIDString()->registerHook(&SV_GetIDString_hook);
	g_RehldsHookchains->SV_CompareUserID()->registerHook(&SV_CompareUserID_hook);
	g_RehldsHookchains->SerializeSteamId()->registerHook(&SerializeSteamId_hook);

	g_RehldsHookchains->Steam_NotifyClientDisconnect()->registerHook(&Steam_NotifyClientDisconnect_hook, HC_PRIORITY_LOW);
	g_RehldsHookchains->Steam_GSBUpdateUserData()->registerHook(&Steam_GSBUpdateUserData_hook, HC_PRIORITY_LOW);
	g_RehldsHookchains->Steam_GSGetSteamID()->registerHook(&Steam_GSGetSteamID_hook);

	g_RehldsHookchains->SV_Frame()->registerHook(&SV_Frame_hook);

	return true;
}
