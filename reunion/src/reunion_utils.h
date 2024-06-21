#pragma once

extern cvar_t* pcv_mp_logecho;

extern void LCPrintf(bool critical, const char *fmt, ...);

extern bool Reunion_Utils_Init();
extern void Reunion_Api_Init();
extern bool Reunion_Load();

extern char* trimbuf(char *str);
size_t strecpy(char* dest, const char* src, size_t maxlen, const char* exceptChars, const char repChar = '\0');

extern bool IsNumericAuthKind(client_auth_kind authkind);
extern bool IsUniqueIdKind(client_id_kind idkind);
extern bool IsValidId(uint32 authId);
extern bool IsValidSteamTicket(const uint8 *pvSteam2Key, size_t ucbSteam2Key);
extern bool IsHddsnNumber(const char* authstring);
extern bool IsValidHddsnNumber(const void* data, size_t maxlen);

extern void util_console_print(const char* fmt, ...);
extern void util_syserror(const char* fmt, ...);
extern void util_execute_cmd(const char* fmt, ...);
extern void util_debug_log(const char* fmt, ...);
extern const char* util_netadr_to_string(const netadr_t& adr, bool without_port = true);
extern const char* util_adr_to_string(uint32 ip, uint16 port = 0);
extern const char* util_adr_to_string(const byte *ip, uint16 port = 0);
extern void util_client_printf(IGameClient* client, const char *fmt, ...);
extern void util_connect_and_kick(const netadr_t& adr, const char* reason);
