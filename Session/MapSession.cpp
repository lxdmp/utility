#include "MapSession.h"

const boost::posix_time::time_duration MapSession::_default_max_inactive_interval = 
	boost::posix_time::minutes(30);

MapSession::MapSession() : MapSession(MapSession::getRandomUuid())
{
}


MapSession::MapSession(std::string id) : 
	_id(id), _original_id(this->_id), 
	_creation_time(MapSession::now()), 
	_last_accessed_time(this->_creation_time), 
	_max_inactive_interval(MapSession::_default_max_inactive_interval)
{
}

MapSession::MapSession(const Session &session)
{
	this->_id = session.getId();
	this->_original_id = this->_id;

	std::set<std::string> names = session.getAttributeNames();
	for(std::set<std::string>::const_iterator names_iter=names.begin(); names_iter!=names.end(); ++names_iter)
	{
		const std::string &name = *names_iter;
		boost::any *val = session.getAttribute(name);
		this->setAttribute(name, *val);
	}

	this->_last_accessed_time = session.getLastAccessedTime();
	this->_creation_time = session.getCreationTime();
	this->_max_inactive_interval = session.getMaxInactiveInterval();
}

const std::map<std::string, boost::any>& MapSession::attributes() const
{
	return this->_session_attributes;
}

const std::string& MapSession::getId() const
{
	return this->_id;
}

void MapSession::setLastAccessedTime(boost::posix_time::ptime lastAccessedTime)
{
	this->_last_accessed_time = lastAccessedTime;
}

const boost::posix_time::ptime& MapSession::getCreationTime() const
{
	return this->_creation_time;
}

const std::string& MapSession::getOriginalId() const
{
	return this->_original_id;
}

void MapSession::setOriginalId(std::string originalId)
{
	this->_original_id = originalId;
}

const std::string& MapSession::changeSessionId()
{
	std::string changedId = MapSession::getRandomUuid();
	this->_id = changedId;
	return this->_id;
}

const boost::posix_time::ptime& MapSession::getLastAccessedTime() const
{
	return this->_last_accessed_time;
}

void MapSession::setMaxInactiveInterval(boost::posix_time::time_duration interval)
{
	this->_max_inactive_interval = interval;
}

const boost::posix_time::time_duration& MapSession::getMaxInactiveInterval() const
{
	return this->_max_inactive_interval;
}

bool MapSession::isExpired() const
{
	return this->isExpired(MapSession::now());
}

bool MapSession::isExpired(boost::posix_time::ptime stamp) const
{
	if(this->_max_inactive_interval.is_negative())
		return false;
	/*
	std::cout
		<<boost::posix_time::to_simple_string(stamp)<< "-"
		<<boost::posix_time::to_simple_string(this->_last_accessed_time)<<" >= "
		<<boost::posix_time::to_simple_string(this->_max_inactive_interval)
		<<std::endl;
	*/
	return (stamp-this->_last_accessed_time>=this->_max_inactive_interval);
}

std::set<std::string> MapSession::getAttributeNames() const
{
	std::set<std::string> ret;
	for(std::map<std::string, boost::any>::const_iterator iter=this->_session_attributes.begin(); 
		iter!=this->_session_attributes.end(); ++iter)
		ret.insert(iter->first);
	return ret;
}

boost::any* MapSession::getAttribute(std::string attributeName) const
{
	std::map<std::string, boost::any>::const_iterator iter = this->_session_attributes.find(attributeName);
	if(iter==this->_session_attributes.end())
		return NULL;
	return const_cast<boost::any*>(&(iter->second));
}

void MapSession::setAttribute(std::string attributeName, const boost::any &attributeVal)
{
	this->_session_attributes.insert(std::make_pair(attributeName, attributeVal));
}

void MapSession::removeAttribute(std::string attributeName)
{
	this->_session_attributes.erase(attributeName);
}

bool MapSession::operator<(const MapSession &session) const
{
	return this->_id<session.getId();
}

boost::posix_time::ptime MapSession::now()
{
	return boost::posix_time::second_clock::local_time();
}

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
std::string MapSession::getRandomUuid()
{
	std::ostringstream s;
	boost::uuids::random_generator random_gen;
	boost::uuids::uuid u = random_gen();
	s<<u;
	return s.str();
}

#ifdef MAP_SESSION_UNIT_TEST
#include <iostream>
#include <boost/shared_ptr.hpp>
int main()
{
	boost::shared_ptr<Session> session(new MapSession());
	std::cout<<session->getId()<<std::endl;

	session->setAttribute("abc", 1);
	session->setAttribute("aaa", std::string("123"));

	{
		std::string key = "abc";
		int *val = Session::getAttribute<int>(*session, key);
		if(!val)
			std::cout<<"no attribute named "<<key<<std::endl;
		else
			std::cout<<key<<" : "<<*val<<std::endl;
	}

	{
		std::string key = "aaa";
		std::string *val = Session::getAttribute<std::string>(*session, key);
		if(!val)
			std::cout<<"no attribute named "<<key<<std::endl;
		else
			std::cout<<key<<" : "<<*val<<std::endl;
	}
	
	{
		std::string key = "aab";
		int *val = Session::getAttribute<int>(*session, key);
		if(!val)
			std::cout<<"no attribute named "<<key<<std::endl;
		else
			std::cout<<key<<" : "<<*val<<std::endl;
	}

	{
		std::string key = "aab";
		std::string *val = Session::getAttribute<std::string>(*session, key);
		if(!val)
			std::cout<<"no attribute named "<<key<<std::endl;
		else
			std::cout<<key<<" : "<<*val<<std::endl;
	}

	return 0;
}
#endif

