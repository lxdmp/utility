#ifndef _MAP_SESSION_REPO_IMPL_H_
#define _MAP_SESSION_REPO_IMPL_H_

template<typename SessionT>
MapSessionRepo<SessionT>::MapSessionRepo() : 
	_default_max_inactive_interval(boost::posix_time::minutes(30))
{
}

template<typename SessionT>
void MapSessionRepo<SessionT>::setDefaultMaxInactiveInterval(
	boost::posix_time::time_duration defaultMaxInactiveInterval
)
{
	this->_default_max_inactive_interval = defaultMaxInactiveInterval;
}

template<typename SessionT> 
SessionT MapSessionRepo<SessionT>::createSession()
{
	SessionT result;
	if(this->_default_max_inactive_interval>boost::posix_time::hours(0))
		result.setMaxInactiveInterval(this->_default_max_inactive_interval);
	return result;
}

template<typename SessionT> 
void MapSessionRepo<SessionT>::save(const SessionT &session)
{
	this->_sessions.insert(std::make_pair(session.getId(), session));
}

template<typename SessionT> 
SessionT* MapSessionRepo<SessionT>::findById(std::string session_id)
{
	typename std::map<std::string, SessionT>::iterator iter = this->_sessions.find(session_id);
	if(iter==this->_sessions.end())
		return NULL;
	SessionT& saved_session = iter->second;
	if(saved_session.isExpired())
	{
		this->deleteById(saved_session.getId());
		return NULL;
	}
	return &saved_session;
}

template<typename SessionT> 
void MapSessionRepo<SessionT>::deleteById(std::string session_id)
{
	this->_sessions.erase(session_id);
}

#endif

