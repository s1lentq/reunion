#pragma once

#define MAX_LP_QUERIES_RATE			2		// maximum low-priority queries from player between flood checks
#define MAX_HP_QUERIES_RATE			11		// maximum high-priority queries from player between flood checks
#define MIN_HP_QUERIES_RATE			9		// minimal rate for high-priority queries

#define MAX_STORED_QUERIES			64		// maximum number of ips for individual filtering
#define MAX_UNIQUE_QUERIES_FILTER	(MAX_STORED_QUERIES * 3)	// filter ip's separately when unique queries rate <= limit
#define MAX_TOTAL_QUERIES_FILTER	(MAX_STORED_QUERIES * MAX_HP_QUERIES_RATE)	// filter ip's separately when total queries rate <= limit

#define PACKET_HEADER_SIZE			42

// Query flood limiter
#define QUERY_CHECK_INTERVAL		0.25f	// time interval between flood checks
#define QUERY_FLOOD_DEFAULT_BANTIME	10.0f
#define QUERY_FLOOD_LOG_INTERVAL	3.0f

class CQueryLimiter
{
public:
	CQueryLimiter();

	void addExceptIPs(std::vector<ipv4_t>& exceptIPs);

	bool allowQuery(const netadr_t& adr, bool critical = false);
	bool allowForExcept(uint32_t ip, uint16_t port) const;
	bool isUnderFlood() const;

	void checkRates(float currentTime, float delta, CQueryBanlist& banlist);

	void incomingPacket(size_t bytes);
	void incomingQuery();
	void checkForGlobalRateLimit();
	bool getUseGlobalRateLimit() const;
	bool allowGlobalQuery() const;

private:
	struct last_query_t
	{
		uint32	ip;
		uint32	rate;
	};

	size_t			m_totalIncomingSize;
	size_t			m_totalPackets;
	size_t			m_totalQueries;
	int				m_uniqueQueries;
	double			m_lastFloodLog;

	bool			m_useGlobalRateLimit;

	std::vector<ipv4_t>	m_exceptIPs;

	size_t			m_lastAdded;
	last_query_t	m_lastQueries[MAX_STORED_QUERIES];
};
