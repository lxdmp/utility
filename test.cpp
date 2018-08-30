#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "CustomDataType.h"
#include "tool.h"

#ifdef CPPBIND_JSON_TEST
class Test1
{
public:
	void setBind(cppbind::Binder *binder)
	{
		binder->bind("a1", a1);
		binder->bind("a2", a2);
		binder->bind("a3", a3);
		binder->bind("a4", a4);
		binder->bind("a5", a5);
		binder->bind("a6_1", a6_1);
		binder->bind("a6_2", a6_2);
		binder->bind("a7_1", a7_1);
		binder->bind("a7_2", a7_2);
		binder->bind("posix_time", pt);
		binder->bind("gregorian_date", gd);
	}

public:
	Test1()
	{
	}

	void test()
	{
		a1 = 1;
		a2 = 2.3;
		a3 = "abc";
		a4 = boost::shared_ptr<int>(new int(12));
		a6_1.push_back("123");
		a6_1.push_back("321");
		a7_2.insert(std::make_pair("abc", 123));
		a7_2.insert(std::make_pair("ab", 321));
		pt = boost::posix_time::ptime(boost::gregorian::date(2010, 10, 2), boost::posix_time::time_duration(10, 11, 23));
		gd = boost::gregorian::date(2013, 2, 27);
	}

	int a1;
	double a2;
	std::string a3;
	boost::shared_ptr<int> a4;
	boost::shared_ptr<int> a5;
	std::vector<std::string> a6_1;
	std::vector<std::string> a6_2;
	std::map<std::string, int> a7_1;
	std::map<std::string, int> a7_2;
	JsonPosixTime pt;
	JsonGregorianDate gd;

	void serialize(std::ostringstream &s) const
	{
		s<<"a1 : "<<a1<<std::endl;
		s<<"a2 : "<<a2<<std::endl;
		s<<"a3 : "<<a3<<std::endl;
		if(a4.get())
			s<<"a4 : "<<*a4<<std::endl;
		if(a5.get())
			s<<"a5 : "<<*a5<<std::endl;

		s<<"a6_1 : ";
		for(size_t i=0; i<a6_1.size(); ++i)
		{
			if(i>0)
				s<<",";
			s<<a6_1[i];
		}
		s<<std::endl;
	
		s<<"a6_2 : ";
		for(size_t i=0; i<a6_2.size(); ++i)
		{
			if(i>0)
				s<<",";
			s<<a6_2[i];
		}
		s<<std::endl;

		s<<"a7_1 : ";
		for(std::map<std::string, int>::const_iterator iter=a7_1.begin(); iter!=a7_1.end(); ++iter)
		{
			if(iter!=a7_1.begin())
				s<<", ";
			s<<iter->first<<" : "<<iter->second;
		}
		s<<std::endl;

		s<<"a7_2 : ";
		for(std::map<std::string, int>::const_iterator iter=a7_2.begin(); iter!=a7_2.end(); ++iter)
		{
			if(iter!=a7_2.begin())
				s<<", ";
			s<<iter->first<<" : "<<iter->second;
		}
		s<<std::endl;

		s<<"pt : ";
		s<<boost::posix_time::to_simple_string(pt);
		s<<std::endl;

		s<<"gd : ";
		s<<boost::gregorian::to_simple_string(gd);
		s<<std::endl;
	}
};

class Test2
{
public:
	void setBind(cppbind::Binder *binder)
	{
		binder->bind("test", _test);
	}

	void test()
	{
		_test.test();
	}

	void serialize(std::ostringstream &s) const
	{
		s<<"test : {"<<std::endl;
		_test.serialize(s);
		s<<"}"<<std::endl;
	}

	Test1 _test;
};

void cppbind_json_test()
{
	std::ostringstream buf;
		
	// encode test
	std::cout<<"ENCODE TEST:"<<std::endl;
	{
		Test2 test2;
		test2.test();
		cppbind::JsonBind<Test2> binder;
		std::stringstream s;
		binder.encode(test2, &s);
		buf<<s.str();
		std::cout<<buf.str()<<std::endl;
	}

	// decode test
	std::cout<<"DECODE TEST:"<<std::endl;
	{
		cppbind::JsonBind<Test2> binder;
		boost::shared_ptr<Test2> test2 = binder.decode(buf.str());
		std::ostringstream s;
		test2->serialize(s);
		std::cout<<s.str()<<std::endl;
	}
}
#endif

int main()
{
	try{
	#ifdef CPPBIND_JSON_TEST
		cppbind_json_test();
	#endif
	}catch(std::exception &e){
		std::cout<<e.what();
	}

	{
		std::vector<std::string> a;
		a.push_back("1");
		a.push_back("2");
		std::cout<<StringJoiner(',')(a)<<std::endl;
	}

	{
		std::vector<int> a;
		a.push_back(3);
		a.push_back(4);
		std::cout<<StringJoiner(',')(a.begin(), a.end(), [&](int val){
			std::stringstream s;
			s<<val;
			return s.str();
		})<<std::endl;
	}

	return 0;
}

