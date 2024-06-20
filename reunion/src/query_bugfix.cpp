#include "precompiled.h"

CBuggedQueryFix::CBuggedQueryFix()
{
	reset();
}

bool CBuggedQueryFix::isBuggedQuery(const netadr_t& from)
{
	double realtime = g_ServerInfo->getRealTime();
	uint32 ip = *(uint32 *)from.ip;

	for (last_query_t& q : m_lastQueries) {
		if (q.ip == ip) {
			double delta = realtime - q.time;
			q.time = realtime;
			return delta < BUGGED_QUERY_FIX_MAXDELTA;
		}
	}

	last_query_t& q = m_lastQueries[m_seq++ & (BUGGED_QUERY_FIX_MAXIPS - 1)];
	q.ip = ip;
	q.time = realtime;
	return false;
}

void CBuggedQueryFix::reset()
{
	memset(m_lastQueries, 0, sizeof m_lastQueries);
	m_seq = 0;
}
