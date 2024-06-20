#pragma once

#include "reunion_shared.h"

extern IRehldsApi* g_RehldsApi;
extern const RehldsFuncs_t* g_RehldsFuncs;
extern IRehldsHookchains* g_RehldsHookchains;
extern IRehldsServerStatic* g_RehldsSvs;
extern ISteamGameServer *g_ISteamGameServer;

extern bool Reunion_RehldsApi_Init();
