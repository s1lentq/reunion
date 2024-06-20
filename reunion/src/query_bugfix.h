#pragma once

#define BUGGED_QUERY_FIX_MAXIPS		16		// max stored ips for detecting a several queries row
#define BUGGED_QUERY_FIX_MAXDELTA	0.05

class CBuggedQueryFix
{
public:
	CBuggedQueryFix();

	bool isBuggedQuery(const netadr_t& from);
	void reset();

private:
	struct last_query_t
	{
		uint32	ip;
		double	time;
	};

	last_query_t	m_lastQueries[BUGGED_QUERY_FIX_MAXIPS];
	size_t			m_seq;
};
