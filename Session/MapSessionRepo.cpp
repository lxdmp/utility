#ifdef MAP_SESSION_REPO_UNIT_TEST

#include "MapSessionRepo.h"
#include "MapSession.h"
#include <boost/thread.hpp>

int main()
{
	MapSessionRepo<MapSession> repo;
	repo.setDefaultMaxInactiveInterval(boost::posix_time::seconds(10));

	std::string session_id;
	const char key[] = "times";
	{
		MapSession session = repo.createSession();
		int val = 1;
		Session::setAttribute(session, key, val);
		repo.save(session);
		session_id = session.getId();
		std::cout<<"session with id "<<session_id<<" added"<<std::endl;
	}

	while(true)
	{
		MapSession *session = repo.findById(session_id);
		if(!session){
			std::cout<<"no session with id "<<session_id<<" found"<<std::endl;
			break;
		}else{
			int *val = Session::getAttribute<int>(*session, key);
			std::cout<<"session with id "<<session_id<<" found, val : "<<*val<<std::endl;
			*val += 1;
		}
		boost::thread::sleep(boost::get_system_time()+boost::posix_time::seconds(1));
	}

	return 0;
}

#endif

