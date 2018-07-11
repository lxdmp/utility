/*
 * 以std::map与boost::any存储一个会话中的各项属性.
 */
#ifndef _MAP_SESSION_H_
#define _MAP_SESSION_H_

#include <map>
#include <boost/any.hpp>
#include "Session.h"

class MapSession : public Session
{
public:
	MapSession();
	MapSession(std::string id);
	MapSession(const Session &session);

	const std::map<std::string, boost::any>& attributes() const;

	const std::string& getId() const;
	void setLastAccessedTime(boost::posix_time::ptime lastAccessedTime);
	const boost::posix_time::ptime& getCreationTime() const;

	const std::string& getOriginalId() const;
	void setOriginalId(std::string originalId);
	const std::string& changeSessionId();

	const boost::posix_time::ptime& getLastAccessedTime() const;

	void setMaxInactiveInterval(boost::posix_time::time_duration interval);
	const boost::posix_time::time_duration& getMaxInactiveInterval() const;

	bool isExpired() const;
	bool isExpired(boost::posix_time::ptime stamp) const;

	std::set<std::string> getAttributeNames() const;
	boost::any* getAttribute(std::string attributeName) const;
	void setAttribute(std::string attributeName, const boost::any &attributeVal);
	void removeAttribute(std::string attributeName);

	bool operator<(const MapSession &session) const;

public:
	static std::string getRandomUuid();
	static boost::posix_time::ptime now();

private:
	static const boost::posix_time::time_duration _default_max_inactive_interval;
	std::string _id;
	std::string _original_id;
	std::map<std::string, boost::any> _session_attributes;
	boost::posix_time::ptime _creation_time;
	boost::posix_time::ptime _last_accessed_time;
	boost::posix_time::time_duration _max_inactive_interval;
};

#endif

