#pragma once
#include <vector>
#include <algorithm>
#include "Memory.h"

#include "BBox.h"

namespace RayTrace
{
	// KdTree Declarations
	struct KdNode {
		void init(float p, uint32_t a) {
			splitPos = p;
			splitAxis = a;
			rightChild = (1 << 29) - 1;
			hasLeftChild = 0;
		}
		void initLeaf() {
			splitAxis = 3;
			rightChild = (1 << 29) - 1;
			hasLeftChild = 0;
		}
		// KdNode Data
		float splitPos;
		uint32_t splitAxis : 2;
		uint32_t hasLeftChild : 1, rightChild : 29;
	};


	template <typename NodeData> class KdTree {
	public:
		// KdTree Public Methods
		KdTree(const std::vector<NodeData>& data);
		~KdTree() {
			FreeAligned(nodes);
			FreeAligned(nodeData);
		}
		template <typename LookupProc> void Lookup(const Vector3f& _point,
			LookupProc& process, float& maxDistSquared) const;
	private:
		// KdTree Private Methods
		void recursiveBuild(uint32_t nodeNum, int start, int end,
			const NodeData** buildNodes);
		template <typename LookupProc> void privateLookup(uint32_t nodeNum,
			const Vector3f& _point, LookupProc& process, float& maxDistSquared) const;

		// KdTree Private Data
		KdNode* nodes;
		NodeData* nodeData;
		uint32_t nNodes, nextFreeNode;
	};


	template <typename NodeData> struct CompareNode {
		CompareNode(int a) { axis = a; }
		int axis;
		bool operator()(const NodeData* d1, const NodeData* d2) const {
			return d1->m_point[axis] == d2->m_point[axis] ? (d1 < d2) :
				d1->m_point[axis] < d2->m_point[axis];
		}
	};



	// KdTree Method Definitions
	template <typename NodeData>
	KdTree<NodeData>::KdTree(const std::vector<NodeData>& d) {
		nNodes = (uint32_t) d.size();
		nextFreeNode = 1;
		nodes = AllocAligned<KdNode>(nNodes);
		nodeData = AllocAligned<NodeData>(nNodes);
		std::vector<const NodeData*> buildNodes(nNodes, NULL);
		for (uint32_t i = 0; i < nNodes; ++i)
			buildNodes[i] = &d[i];
		// Begin the KdTree building process
		recursiveBuild(0, 0, nNodes, &buildNodes[0]);
	}


	template <typename NodeData> void
		KdTree<NodeData>::recursiveBuild(uint32_t nodeNum, int start, int end,
			const NodeData** buildNodes) 
	{
		// Create leaf node of kd-tree if we've reached the bottom
		if (start + 1 == end) {
			nodes[nodeNum].initLeaf();
			nodeData[nodeNum] = *buildNodes[start];
			return;
		}

		// Choose split direction and partition data

		// Compute bounds of data from _start_ to _end_
		BBox3f bound;
		for (int i = start; i < end; ++i)
			 bound.addPoint(buildNodes[i]->m_point);		

		eAxis splitAxis = bound.maximumExtent();
		int splitPos = (start + end) / 2;
		std::nth_element(&buildNodes[start], &buildNodes[splitPos],
			&buildNodes[end], CompareNode<NodeData>(splitAxis));

		// Allocate kd-tree node and continue recursively
		nodes[nodeNum].init(buildNodes[splitPos]->m_point[splitAxis], splitAxis);
		nodeData[nodeNum] = *buildNodes[splitPos];
		if (start < splitPos) {
			nodes[nodeNum].hasLeftChild = 1;
			uint32_t childNum = nextFreeNode++;
			recursiveBuild(childNum, start, splitPos, buildNodes);
		}
		if (splitPos + 1 < end) {
			nodes[nodeNum].rightChild = nextFreeNode++;
			recursiveBuild(nodes[nodeNum].rightChild, splitPos + 1,
				end, buildNodes);
		}
	}


	template <typename NodeData> template <typename LookupProc>
	void KdTree<NodeData>::Lookup(const Vector3f& _point, LookupProc& proc,
		float& maxDistSquared) const {
		privateLookup(0, _point, proc, maxDistSquared);
	}


	template <typename NodeData> template <typename LookupProc>
	void KdTree<NodeData>::privateLookup(uint32_t nodeNum, const Vector3f& _point,
		LookupProc& process, float& maxDistSquared) const {
		KdNode* node = &nodes[nodeNum];
		// Process kd-tree node's children
		int axis = node->splitAxis;
		if (axis != 3) {
			float dist2 = (_point[axis] - node->splitPos) * (_point[axis] - node->splitPos);
			if (_point[axis] <= node->splitPos) {
				if (node->hasLeftChild)
					privateLookup(nodeNum + 1, _point, process, maxDistSquared);
				if (dist2 < maxDistSquared && node->rightChild < nNodes)
					privateLookup(node->rightChild, _point, process, maxDistSquared);
			}
			else {
				if (node->rightChild < nNodes)
					privateLookup(node->rightChild, _point, process, maxDistSquared);
				if (dist2 < maxDistSquared && node->hasLeftChild)
					privateLookup(nodeNum + 1, _point, process, maxDistSquared);
			}
		}

		// Hand kd-tree node to processing function
		float dist2 = (nodeData[nodeNum].m_point - _point).lengthSqr(); //DistanceSquared(nodeData[nodeNum].p, p);
		if (dist2 < maxDistSquared)
			process(_point, nodeData[nodeNum], dist2, maxDistSquared);
	}
}