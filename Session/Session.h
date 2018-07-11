// 实现各种应用中的会话.
#ifndef _SESSION_H_
#define _SESSION_H_

#include <string>
#include <set>
#include <boost/date_time/posix_time/posix_time.hpp>

/*
 * Session - 会话
 */
class Session
{
public:
	virtual const std::string& getId() const = 0;
	virtual const std::string& changeSessionId() = 0; // 返回新设置的id.

	virtual std::set<std::string> getAttributeNames() const = 0;
	virtual boost::any* getAttribute(std::string attributeName) const = 0;
	virtual void setAttribute(std::string attributeName, const boost::any &attributeVal) = 0;
	virtual void removeAttribute(std::string attributeName) = 0;

	template<typename AttributeT> 
	static AttributeT* getAttribute(const Session &session, std::string attributeName);
	template<typename AttributeT> 
	static void setAttribute(Session &session, std::string attributeName, const AttributeT &attributeVal);

	virtual const boost::posix_time::ptime& getCreationTime() const = 0;
	virtual void setLastAccessedTime(boost::posix_time::ptime lastAccessedTime) = 0;
	virtual const boost::posix_time::ptime& getLastAccessedTime() const = 0;
	virtual void setMaxInactiveInterval(boost::posix_time::time_duration interval) = 0;
	virtual const boost::posix_time::time_duration& getMaxInactiveInterval() const = 0;
	virtual bool isExpired() const = 0;
};

template<typename AttributeT> 
AttributeT* Session::getAttribute(const Session &session, std::string attributeName)
{
	boost::any *val = session.getAttribute(attributeName);
	if(!val)
		return NULL;
	AttributeT *ret = boost::any_cast<AttributeT>(val);
	if(!ret)
		return NULL;
	return ret;
}

template<typename AttributeT> 
void Session::setAttribute(Session &session, std::string attributeName, const AttributeT &attributeVal)
{
	session.setAttribute(attributeName, boost::any(attributeVal));
}

#endif

