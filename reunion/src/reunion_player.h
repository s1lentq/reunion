#pragma once

#include "reunion_shared.h"

class CReunionPlayer
{
public:
	CReunionPlayer(IGameClient* cl);
	void clear();
	IGameClient* getClient() const;
	void setStorageId(USERID_t* userid);
	void setRawAuthData(auth_key_kind kk, void* data, size_t len);
	void setConnectionTime(double time);
	void setUnauthenticatedId(uint64 steamId);
	void authenticated(int proto, client_id_kind idkind, client_auth_kind authkind, USERID_t* steamid);
	char* GetSteamId();
	CSteamID getDisplaySteamId() const;
	USERID_t* getSerializedId();
	USERID_t* getStorageId();
	uint64 getUnauthenticatedId() const;
	bool isUnauthenticatedId() const;
	int getProcotol() const;
	double getConnectionTime() const;
	client_auth_kind GetAuthKind() const;
	size_t getRawAuthData(void* buf, size_t maxlen) const;
	auth_key_kind GetAuthKeyKind() const;
	void kick(const char* reason) const;

private:
	void generateSerializedId();

	int m_Id;
	IGameClient* m_pClient;
	client_id_kind m_IdKind;
	client_auth_kind m_AuthKind;
	int m_Protocol;
	double m_ConnectionTime;

	// SteamID to display
	USERID_t m_DisplaySteamId;

	// An id that'll be written into userfilters
	USERID_t m_SerializedId;

	// Real SteamID to storage
	USERID_t m_StorageSteamId;

	// Unauthenticated SteamID
	uint64 m_UnauthenticatedSteamId;

	char m_idString[32];

	// Raw auth data. More unique then 4-byte id
	auth_key_kind m_authKeyKind;
	unsigned char m_rawAuthData[MAX_AUTHKEY_LEN + 1];
	size_t m_rawAuthDataLen;
};

extern CReunionPlayer **g_Players;

extern bool Reunion_Init_Players();
extern void Reunion_Free_Players();
extern CReunionPlayer* GetReunionPlayerByUserIdPtr(USERID_t* pUserId);
extern CReunionPlayer* GetReunionPlayerByClientPtr(IGameClient* cl);
extern CReunionPlayer* GetReunionPlayerBySteamdId(uint64 steamIDUser);
extern size_t Reunion_GetConnectedPlayersCount();
extern bool Reunion_IsSteamIdInUse(CReunionPlayer* player);
extern void Reunion_RemoveDuplicateSteamIds(CReunionPlayer* player);
