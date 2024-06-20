#include "precompiled.h"

void CQueryBanlist::addBan(uint32_t ip, uint32_t time, int pps)
{
	m_bans.push_back({ ip, (uint32_t)g_ServerInfo->getRealTime() + time * 60, pps });
	const char* ipstr = util_adr_to_string(ip);
	LCPrintf(true, "ip %s banned for %i minutes for query flooding (PPS: %i)\n", ipstr, time, pps);
}

void CQueryBanlist::addBan(uint32_t ip, uint32_t time)
{
	m_bans.push_back({ ip, (uint32_t)g_ServerInfo->getRealTime() + time * 60, 0 });
	util_console_print("ReuBans: ip %s banned for %i minutes from console\n", util_adr_to_string(ip), time);
}

bool CQueryBanlist::removeBan(uint32_t ip)
{
	std::vector<ban_t>::iterator ban = std::find_if(m_bans.begin(), m_bans.end(), [ip](ban_t& ban) { return ban.ip == ip; });
	if (ban != m_bans.end()) {
		util_console_print("ReuBans: removed ban of %s\n", util_adr_to_string(ip));
		m_bans.erase(ban);
		return true;
	}

	util_console_print("ReuBans: ip %s is not banned\n", util_adr_to_string(ip));
	return false;
}

bool CQueryBanlist::isBanned(uint32_t ip) const
{
	for (const ban_t &ban : m_bans) {
		if (ban.ip == ip)
			return true;
	}

	return false;
}

void CQueryBanlist::removeExpired(uint32_t realtime)
{
	if (!m_bans.empty()) {
		m_bans.erase(std::remove_if(m_bans.begin(), m_bans.end(), [&](ban_t& ban) { return ban.end < realtime; }), m_bans.end());
	}
}

void CQueryBanlist::print()
{
	util_console_print("%-16s\t%-8s\tExpire time\n", "IP", "pps");

	for (const ban_t &ban : m_bans) {
		uint32_t exp = ban.end - (uint32_t)g_ServerInfo->getRealTime();
		util_console_print("%-16s\t%-8u\t%u:%.2u\n", util_adr_to_string(ban.ip), ban.pps, exp / 60, exp % 60);
	}
}
