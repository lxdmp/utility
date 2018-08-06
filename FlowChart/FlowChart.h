/*
 * 流程图实现,目前实现了两类节点:
 * - 路由节点;
 * - 结果节点.
 *
 * 对于一些流程、逻辑比较复杂、混乱的情况,可用该实现拆解逻辑.
 */
#ifndef _FLOW_CHART_H_
#define _FLOW_CHART_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

template<typename InputT, typename OutputT>
class FlowChart
{
	template<int v>
	class Int2Type
	{
		enum {value = v };
	};

// 节点
public:
	class Node
	{
	public:
		virtual ~Node(){}
		virtual const Node* execute(const InputT &input, OutputT &output) const = 0;
	};

// 路由节点子分支
public:
	template<typename RouteT>
	class RouteNodeBranch
	{
	public:
		typedef boost::function<bool(const RouteT&)> BranchSelecterDecl;

		RouteNodeBranch(Node *node, BranchSelecterDecl selecter);
		RouteNodeBranch(Node *node, BranchSelecterDecl selecter1, BranchSelecterDecl selecter2);

		bool operator()(const RouteT &routeVal) const;

		boost::shared_ptr<Node> node() const{return _node;}

		/*
		 * ComparatorT支持以下类型:
		 * - functor : bool()(const RouteT&, const RouteT&) const, 包含std::less,std::greater,...;
		 * - std::string : '>,>=,!=,==,...', 等效于std::less,std::greater,...
		 * 
		 * 上一级节点的输出作为lhs,与rhs通过运算决定是否流向指向的节点.
		 */
		template<typename ComparatorT> 
		static BranchSelecterDecl bindSelecter(ComparatorT comparator, RouteT rhs);

	private:
		template<typename ComparatorT> 
		static BranchSelecterDecl bindSelecterImpl(ComparatorT comparator, RouteT routeVal, Int2Type<false> commonCase);
		template<typename ComparatorT> 
		static BranchSelecterDecl bindSelecterImpl(ComparatorT comparator, RouteT routeVal, Int2Type<true> stringCase);

	private:
		boost::shared_ptr<Node> _node; // 指向的节点
		std::vector<BranchSelecterDecl> _selecter; // 需满足的条件
	};

// 路由节点
public:
	template<typename RouteT>
	class RouteNode : public Node
	{
	public:
		typedef boost::function<RouteT(const InputT &input, OutputT &output)> RouteNodeLogicDecl;
	public:
		RouteNode(RouteNodeLogicDecl logic);
		virtual const Node* execute(const InputT &input, OutputT &output) const;

		void addSubNode(RouteT routeVal, Node *node);

		template<typename ComparatorT> 
		void addSubNode(ComparatorT comparator, RouteT routeVal, Node *node);

		template<typename ComparatorT1, typename ComparatorT2> 
		void addSubNode(ComparatorT1 comparator1, RouteT routeVal1, 
			ComparatorT2 comparator2, RouteT routeVal2, Node *node);

	private:
		typename RouteNode<RouteT>::RouteNodeLogicDecl _node_logic;
		std::vector<boost::shared_ptr<RouteNodeBranch<RouteT> > > _branches;
	};

// 结果节点
public:
	class ResultNode : public Node
	{
	public:
		typedef boost::function<void(const InputT &input, OutputT &output)> ResultNodeLogicDecl;
	public:
		ResultNode(ResultNodeLogicDecl logic);
		virtual const Node* execute(const InputT &input, OutputT &output) const;
	private:
		typename ResultNode::ResultNodeLogicDecl _node_logic;
	};

public:
	FlowChart(Node *entry);
	void execute(const InputT &input, OutputT &output) const;
private:
	boost::shared_ptr<Node> _entry;
};

#include "FlowChartImpl.h"

#endif

