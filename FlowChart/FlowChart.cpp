#include "FlowChart.h"
#include <boost/bind.hpp>
#include <iostream>

#ifdef FLOW_CHART_UNIT_TEST
static int route1(const std::string &input, int &output)
{
	std::cout<<"\tvia route1"<<std::endl;
	return input.size()%3;
}

static std::string route2(const std::string &input, int &output)
{
	std::cout<<"\tvia route2"<<std::endl;
	return std::string(input.begin(), ++input.begin());
}

static void result1(const std::string &input, int &output)
{
	std::cout<<"\tvia result1"<<std::endl;
	output = 1;
}

static void result2(const std::string &input, int &output)
{
	std::cout<<"\tvia result2"<<std::endl;
	output = 2;
}

static void result3(const std::string &input, int &output)
{
	std::cout<<"\tvia result3"<<std::endl;
	output = 3;
}

static void result4(const std::string &input, int &output)
{
	std::cout<<"\tvia result4"<<std::endl;
	output = 4;
}

int main()
{
	FlowChart<std::string, int>::RouteNode<int> *root = new FlowChart<std::string, int>::RouteNode<int>(
		boost::bind(route1, _1, _2)
	);
	FlowChart<std::string, int> chart(root);
	root->addSubNode(0, new FlowChart<std::string, int>::ResultNode(boost::bind(result1, _1, _2)));
	root->addSubNode(1, new FlowChart<std::string, int>::ResultNode(boost::bind(result2, _1, _2)));

	FlowChart<std::string, int>::RouteNode<std::string> *sub = new FlowChart<std::string, int>::RouteNode<std::string>(boost::bind(route2, _1, _2));
	root->addSubNode(2, sub);
	sub->addSubNode("a", new FlowChart<std::string, int>::ResultNode(boost::bind(result3, _1, _2)));
	sub->addSubNode(
		std::greater<std::string>(), "b", 
		std::less<std::string>(), "d", 
		new FlowChart<std::string, int>::ResultNode(boost::bind(result4, _1, _2))
	);
	sub->addSubNode(
		std::greater<std::string>(), "c", 
		new FlowChart<std::string, int>::ResultNode(boost::bind(result1, _1, _2))
	);

	const char* strs[] = {
		"abc", 
		"abcd",
		"abcde", 
		"bbcde", 
		"cbcde",
		"dbcde"
	};
	for(size_t i=0; i<sizeof(strs)/sizeof(strs[0]); ++i)
	{
		std::string input(strs[i]);
		int output;
		std::cout<<"input : "<<input<<std::endl;
		try{
			chart.execute(input, output);
			std::cout<<"output : "<<output<<std::endl;
		}catch(std::exception &e){
			std::cout<<e.what()<<std::endl;
		}
	}

	return 0;
}
#endif

