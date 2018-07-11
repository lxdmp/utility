#ifndef _SESSION_REPO_H_
#define _SESSION_REPO_H_

#include <string>

template<typename SessionT> 
class SessionRepo
{
	virtual SessionT createSession() = 0; // 创建会话
	virtual void save(const SessionT &session) = 0; // 保存会话
	virtual SessionT* findById(std::string session_id) = 0; // 查找会话
	virtual void deleteById(std::string session_id) = 0; // 删除会话
};

#endif

