#ifndef _FLOW_CHART_IMPL_H_
#define _FLOW_CHART_IMPL_H_

/****************
 * 路由节点子分支
 ****************/
template<typename InputT, typename OutputT>
template<typename RouteT> 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::RouteNodeBranch(
	Node *node, BranchSelecterDecl selecter) : 
	_node(node)
{
	_selecter.push_back(selecter);
}


template<typename InputT, typename OutputT>
template<typename RouteT> 
FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::RouteNodeBranch(
	Node *node, BranchSelecterDecl selecter1, BranchSelecterDecl selecter2) : 
	_node(node)
{
	_selecter.push_back(selecter1);
	_selecter.push_back(selecter2);
}

template<typename InputT, typename OutputT>
template<typename RouteT> 
bool FlowChart<InputT, OutputT>::RouteNodeBranch<RouteT>::operator()(RouteT routeVal) const
{
	for(size_t selecter_idx=0; selecter_idx<this->_selecter.size(); ++selecter_idx)
	{
		if(!this->_selecter[selecter_idx](routeVal))
			return false;
	}
	return true;
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
	if(!node)
		throw std::runtime_error("invalid node added");
	boost::shared_ptr<RouteNodeBranch<RouteT> > new_branch = boost::shared_ptr<RouteNodeBranch<RouteT> >(
		new RouteNodeBranch<RouteT>(node, std::bind2nd(comparator, routeVal))
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
	if(!node)
		throw std::runtime_error("invalid node added");
	boost::shared_ptr<RouteNodeBranch<RouteT> > new_branch = boost::shared_ptr<RouteNodeBranch<RouteT> >(
		new RouteNodeBranch<RouteT>(
			node, 
			std::bind2nd(comparator1, routeVal1), 
			std::bind2nd(comparator2, routeVal2)
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
			return _branches[branch_idx]->_node.get();
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

