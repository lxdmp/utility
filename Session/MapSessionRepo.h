/*
 * 以std::map存储多个会话.
 */
#ifndef _MAP_SESSION_REPO_H_
#define _MAP_SESSION_REPO_H_

#include "SessionRepo.h"
#include "MapSession.h"

template<typename SessionT>
class MapSessionRepo : public SessionRepo<SessionT>
{
public:
	MapSessionRepo();
	void setDefaultMaxInactiveInterval(boost::posix_time::time_duration defaultMaxInactiveInterval);

	SessionT createSession();
	void save(const SessionT &session);
	SessionT* findById(std::string session_id);
	void deleteById(std::string session_id);

private:
	boost::posix_time::time_duration _default_max_inactive_interval;
	std::map<std::string, SessionT> _sessions;
};

#include "MapSessionRepoImpl.h"

#endif

