#ifndef _FLOW_CHART_IMPL_H_
#define _FLOW_CHART_IMPL_H_

#include <sstream>

template<typename T>
struct IsString
{
	enum{value=false};
};

template<>
struct IsString<std::string>
{
	enum{value=true};
};

template<>
struct IsString<const char*>
{
	enum{value=true};
};

template<>
struct IsString<char*>
{
	enum{value=true};
};

/****************
 * 路由节点子分支
 ****************/
template<typename InputT, typename OutputT>
template<typename RouteT> 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::RouteNodeBranch(
	Node *node, BranchSelecterDecl selecter) : 
	_node(node)
{
	if(!_node)
		throw std::runtime_error("invalid node added");
	_selecter.push_back(selecter);
}


template<typename InputT, typename OutputT>
template<typename RouteT> 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::RouteNodeBranch(
	Node *node, BranchSelecterDecl selecter1, BranchSelecterDecl selecter2) : 
	_node(node)
{
	if(!_node)
		throw std::runtime_error("invalid node added");
	_selecter.push_back(selecter1);
	_selecter.push_back(selecter2);
}

template<typename InputT, typename OutputT>
template<typename RouteT> 
bool FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::operator()(
	const RouteT &routeVal) const
{
	for(size_t selecter_idx=0; selecter_idx<this->_selecter.size(); ++selecter_idx)
	{
		if(!this->_selecter[selecter_idx](routeVal))
			return false;
	}
	return true;
}

template<typename InputT, typename OutputT>
template<typename RouteT> 
template<typename ComparatorT> 
typename FlowChart<InputT, OutputT>::template RouteNodeBranch<RouteT>::BranchSelecterDecl 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecter(
	ComparatorT comparator, RouteT routeVal
)
{
	return FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecterImpl(comparator, routeVal, Int2Type<IsString<ComparatorT>::value>());
}

template<typename InputT, typename OutputT>
template<typename RouteT> 
template<typename ComparatorT> 
typename FlowChart<InputT, OutputT>::template RouteNodeBranch<RouteT>::BranchSelecterDecl 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecterImpl(
	ComparatorT comparator, RouteT routeVal, Int2Type<false> commonCase
)
{
	return std::bind2nd(comparator, routeVal);
}


template<typename InputT, typename OutputT>
template<typename RouteT> 
template<typename ComparatorT> 
typename FlowChart<InputT, OutputT>::template RouteNodeBranch<RouteT>::BranchSelecterDecl 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecterImpl(
	ComparatorT comparator, RouteT routeVal, Int2Type<true> stringCase
)
{
	struct match_pair{
		std::string name;
		typedef boost::function<bool(RouteT, RouteT)> ComparatorDecl;
		ComparatorDecl impl;
	};
	static const struct match_pair matches[] = {
		{"!=", std::not_equal_to<RouteT>()}, 
		{"<", std::less<RouteT>()}, 
		{"<=", std::less_equal<RouteT>()}, 
		{"==", std::equal_to<RouteT>()}, 
		{">", std::greater<RouteT>()}, 
		{">=", std::greater_equal<RouteT>()}
	};
	size_t idx = 0;
	for(; idx<sizeof(matches)/sizeof(matches[0]); ++idx)
	{
		if(matches[idx].name.compare(comparator)==0)
			break;
	}
	if(idx>=sizeof(matches)/sizeof(matches[0]))
	{
		std::ostringstream s;
		s<<"Unrecognized comparator \""<<comparator<<"\"";
		throw std::runtime_error(s.str());
	}
	return std::bind2nd(matches[idx].impl, routeVal);
}

/**********
 * 路由节点
 **********/
template<typename InputT, typename OutputT>
template<typename RouteT>
FlowChart<InputT, OutputT>::RouteNode<RouteT>::RouteNode(RouteNodeLogicDecl logic) : 
	_node_logic(logic)
{
}

template<typename InputT, typename OutputT>
template<typename RouteT>
void FlowChart<InputT, OutputT>::RouteNode<RouteT>::addSubNode(RouteT routeVal, Node *node)
{
	this->addSubNode(std::equal_to<RouteT>(), routeVal, node);
}

template<typename InputT, typename OutputT>
template<typename RouteT>
template<typename ComparatorT> 
void FlowChart<InputT, OutputT>::RouteNode<RouteT>::addSubNode(
	ComparatorT comparator, RouteT routeVal, Node *node)
{
	boost::shared_ptr<RouteNodeBranch<RouteT> > new_branch = boost::shared_ptr<RouteNodeBranch<RouteT> >(
		new RouteNodeBranch<RouteT>(
			node, 
			FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecter(comparator, routeVal)
		)
	);
	_branches.push_back(new_branch);
}

template<typename InputT, typename OutputT>
template<typename RouteT>
template<typename ComparatorT1, typename ComparatorT2> 
void FlowChart<InputT, OutputT>::RouteNode<RouteT>::addSubNode(
	ComparatorT1 comparator1, RouteT routeVal1, 
	ComparatorT2 comparator2, RouteT routeVal2, Node* node)
{
	boost::shared_ptr<RouteNodeBranch<RouteT> > new_branch = boost::shared_ptr<RouteNodeBranch<RouteT> >(
		new RouteNodeBranch<RouteT>(
			node, 
			FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecter(comparator1, routeVal1),
			FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::bindSelecter(comparator2, routeVal2)
		)
	);
	_branches.push_back(new_branch);
}

template<typename InputT, typename OutputT>
template<typename RouteT>
const typename FlowChart<InputT, OutputT>::Node* FlowChart<InputT, OutputT>::RouteNode<RouteT>::execute(
	const InputT &input, OutputT &output) const
{
	RouteT routeNodeResult = this->_node_logic(input, output);
	for(size_t branch_idx=0; branch_idx<this->_branches.size(); ++branch_idx)
	{
		if(_branches[branch_idx]->operator()(routeNodeResult))
			return _branches[branch_idx]->node().get();
	}
	throw std::runtime_error("Unhandled route case!");
	return NULL;
}

/**********
 * 结果节点
 **********/
template<typename InputT, typename OutputT>
FlowChart<InputT, OutputT>::ResultNode::ResultNode(ResultNodeLogicDecl logic) : 
	_node_logic(logic)
{
}


template<typename InputT, typename OutputT>
const typename FlowChart<InputT, OutputT>::Node* FlowChart<InputT, OutputT>::ResultNode::execute(const InputT &input, OutputT &output) const
{
	this->_node_logic(input, output);
	return NULL;
}

/********
 * 流程图
 ********/
template<typename InputT, typename OutputT>
FlowChart<InputT, OutputT>::FlowChart(Node *entry) : _entry(entry)
{
}

template<typename InputT, typename OutputT>
void FlowChart<InputT, OutputT>::execute(const InputT &input, OutputT &output) const
{
	const Node *node = this->_entry.get();
	while(node)
		node = node->execute(input, output);
}

#endif

