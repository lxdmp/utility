#ifndef _FLOW_CHART_IMPL_H_
#define _FLOW_CHART_IMPL_H_

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
	typedef std::pair<typename std::map<RouteT, boost::shared_ptr<Node> >::iterator, bool> RouteNodeInsertResType;
	RouteNodeInsertResType res = this->_map.insert(std::make_pair(routeVal, node));
	if(!res.second)
		throw std::runtime_error("duplicate node in route node");
}

template<typename InputT, typename OutputT>
template<typename RouteT>
const typename FlowChart<InputT, OutputT>::Node* FlowChart<InputT, OutputT>::RouteNode<RouteT>::execute(const InputT &input, OutputT &output) const
{
	RouteT routeNodeResult = this->_node_logic(input, output);
	typename std::map<RouteT, boost::shared_ptr<Node> >::const_iterator iter = this->_map.find(routeNodeResult);
	if(iter==this->_map.end())
		throw std::runtime_error("Unhandled route case!");
	return iter->second.get();
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

