#include "precompiled.h"

CQueryLimiter::CQueryLimiter()
{
	m_totalIncomingSize = 0;
	m_totalPackets = 0;
	m_totalQueries = 0;
	m_uniqueQueries = 0;
	m_useGlobalRateLimit = false;
	m_lastAdded = -1;
	memset(m_lastQueries, 0, sizeof m_lastQueries);

	if ((MAX_STORED_QUERIES & (MAX_STORED_QUERIES - 1)) != 0)
		util_syserror("MAX_STORED_QUERIES must be power of 2");
	if ((BUGGED_QUERY_FIX_MAXIPS & (BUGGED_QUERY_FIX_MAXIPS - 1)) != 0)
		util_syserror("BUGGED_QUERY_FIX_MAXIPS must be power of 2");
}

bool CQueryLimiter::allowQuery(const netadr_t& adr, bool critical)
{
	if (!g_ReunionConfig->enableQueryLimiter())
		return true;

	if (allowForExcept(*(uint32_t *)adr.ip, ntohs(adr.port)))
		return true;

	for (size_t i = 0; i < MAX_STORED_QUERIES; i++)
	{
		if (m_lastQueries[i].ip != *(uint32 *)adr.ip) {
			continue;
		}

		if (critical && m_lastQueries[i].rate < MIN_HP_QUERIES_RATE) {
			m_lastQueries[i].rate = MIN_HP_QUERIES_RATE;
			return true;
		}

		// check flood from addr
		return ++m_lastQueries[i].rate <= uint32(critical ? MAX_HP_QUERIES_RATE : MAX_LP_QUERIES_RATE);
	}

	// flood from many new ips
	if (++m_uniqueQueries > MAX_STORED_QUERIES) {
		checkForGlobalRateLimit();
		return false;
	}

	// save new addr
	size_t id = ++m_lastAdded & (MAX_STORED_QUERIES - 1);
	m_lastQueries[id].ip = *(uint32 *)adr.ip;
	m_lastQueries[id].rate = 1;
	return true;
}

void CQueryLimiter::addExceptIPs(std::vector<ipv4_t>& exceptIPs)
{
	std::move(exceptIPs.begin(), exceptIPs.end(), std::back_inserter(m_exceptIPs));
	exceptIPs.clear();
}

bool CQueryLimiter::allowForExcept(uint32_t ip, uint16_t port) const
{
	for (const ipv4_t &e : m_exceptIPs)
	{
		if (e.ip == ip &&
			(e.port == 0 || e.port == port)) {
			return true;
		}
	}

	return false;

}

bool CQueryLimiter::isUnderFlood() const
{
	return m_uniqueQueries >= MAX_STORED_QUERIES;
}

void CQueryLimiter::checkRates(float currentTime, float delta, CQueryBanlist& banlist)
{
	for (size_t i = 0; i < MAX_STORED_QUERIES; i++)
	{
		const int rate = m_lastQueries[i].rate;

		if (rate)
		{
			const int bantime = g_ReunionConfig->getFloodBanTime();
			const int pps = int(double(rate) / delta);

			// ban sender if query rate exceeds limit
			if (bantime != 0 && pps > g_ReunionConfig->getQueriesBanLevel())
			{
				banlist.addBan(m_lastQueries[i].ip, bantime, pps);
				m_lastQueries[i].rate = 0;
				continue;
			}

			// gradually reduce rate
			if (rate <= MAX_LP_QUERIES_RATE * 2)
				m_lastQueries[i].rate = 0;
			else
				m_lastQueries[i].rate = max(MAX_LP_QUERIES_RATE / 2, rate - int(g_ReunionConfig->getQueriesBanLevel() * delta));
		}
	}

	// check for high-pps flood
	checkForGlobalRateLimit();

	// log flood from many ips
	if (isUnderFlood() && currentTime - m_lastFloodLog > QUERY_FLOOD_LOG_INTERVAL)
	{
		m_lastFloodLog = currentTime;

		const int globalPps = int(double(m_totalPackets) / delta);
		const float globalMbps = (m_totalIncomingSize * 8.0 / 1000000.0) / delta;
		LCPrintf(true, "Query flood blocking: %i pps (%.3f mbps) from %i+ IPs\n", globalPps, globalMbps, MAX_STORED_QUERIES);
	}

	// reduce global query rate
	m_totalIncomingSize = 0;
	m_totalPackets = 0;
	m_totalQueries = 0;

	if (m_uniqueQueries <= MAX_STORED_QUERIES * 3/4)
		m_uniqueQueries = 0;
	else
		m_uniqueQueries = max(MAX_STORED_QUERIES * 3/4, m_uniqueQueries - MAX_STORED_QUERIES);
}

void CQueryLimiter::incomingPacket(size_t bytes)
{
	m_totalIncomingSize += bytes;
	m_totalPackets++;
}

void CQueryLimiter::incomingQuery()
{
	m_totalQueries++;
}

void CQueryLimiter::checkForGlobalRateLimit()
{
	m_useGlobalRateLimit = m_uniqueQueries > MAX_UNIQUE_QUERIES_FILTER || m_totalQueries > MAX_TOTAL_QUERIES_FILTER;
}

bool CQueryLimiter::getUseGlobalRateLimit() const
{
	return m_useGlobalRateLimit;
}

bool CQueryLimiter::allowGlobalQuery() const
{
	return m_totalQueries < MAX_STORED_QUERIES;
}
