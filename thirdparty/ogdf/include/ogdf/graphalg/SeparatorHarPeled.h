/** \file
 * \brief Declaration of class SeparatorHarPeled.
 *
 * \author Thomas Klein
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/basic/DualGraph.h>
#include <ogdf/graphalg/PlanarSeparatorModule.h>

namespace ogdf {

namespace planar_separators {

/**
 * \brief Specialised tree representation for Har-Peled.
 * Once a 2/3-separator cycle has been found, his tree can be reconstructed
 * so that it is rooted at the root of the separator.
 */
class OGDF_EXPORT BFSTreeHP : public ArrayBFSTree {
public:
	/**
	 * Constructor.
	 *
	 * @param G the GraphCopy generated by the separator that uses this tree
	 * @param rootNode the initial root for the tree (usually, a random node)
	 */
	BFSTreeHP(GraphCopy& G, node rootNode) : ArrayBFSTree(G, rootNode) { construct(); }

	/**
	 * Builds the tree by performing BFS search.
	 */
	void construct();

	/**
	 * Reconstructs the tree, rooting it at root of cycle.
	 *
	 * @param cycle the cycle based on which we reconstruct.
	 */
	void reconstruct(const Cycle& cycle);

	/**
	 * Calculates the number of children of each node in the tree.
	 */
	void calculateDescendants();
};

struct Ring; // see below

}

using namespace planar_separators;

//! Computes planar separators according to Har-Peled.
/**
 * Computes planar separators based on the algorithm presented by
 * Sariel Har-Peled and Amir Nayyeri in "A Simple Algorithm for Computing a Cycle Separator",
 * arXiv preprint at arXiv:1709.08122v2.
 *
 * @ingroup ga-plansep
 */
class OGDF_EXPORT SeparatorHarPeled : public PlanarSeparatorModule {
	/** We will need a set of nested rings of nodes, see below. */
	friend struct planar_separators::Ring;

public:
	/**
	 * Constructor.
	 */
	SeparatorHarPeled() { }

	virtual double getMaxSeparatorSize(int n) const override { return sqrt(8) * sqrt(n); }

	virtual std::string getSpecificName() const override { return "HPN"; } // Har-Peled and Nayyeri

protected:
#ifdef OGDF_DEBUG

	void verifyRing(const Ring& ring) const;

#endif

	/**
	 * Entry point for the core of the algorithm.
	 *
	 * @param G the Graph to be separated
	 * @param separator list of nodes that will contain the separator nodes
	 * @param first list of nodes that will contain the first half of the separation
	 * @param second list of nodes that will contain the second half of the separation
	 * @return whether the algorithm was successful or not
	 */
	virtual bool doSeparate(const Graph& G, List<node>& separator, List<node>& first,
			List<node>& second) override;

	/** A special tree that can be reconstructed, see above. */
	std::shared_ptr<BFSTreeHP> tree;

	/**
	 * Resets all fields, clears lists and re-initializes arrays to enable reuse of the module.
	 */
	virtual void reset() override;

	/**
	 * Constructs the BFSTreeHP from a random node.
	 */
	virtual void makeTree();

	/**
	 * Finds a non-tree edge that, together with the tree, forms a (possibly too large)
	 * 2/3-separator.
	 *
	 * @return a non-tree edge that forms a 2/3-separator
	 */
	edge findSeparatorEdge() const;

	/**
	 * Creates the dual of the graph to help find a separator edge.
	 *
	 * @param Dual the graph that should contain the dual
	 * @param oldEdge array that maps dual edges back to original edges
	 */
	void createDual(Graph& Dual, EdgeArray<edge>& oldEdge) const;

	/**
	 * Performs a BFS over the faces of the embedding (more or less) to assign a
	 * level to each face and find the borders of each nested area of faces.
	 *
	 * @param root the root node of the tree
	 */
	void findFaceLevels(const node root);

	/**
	 * Constructs the array of concentric rings of nodes formed by the
	 * (potentially partial) borders of the regions found by #findFaceLevels.
	 *
	 * @param cycle the 2/3-separator found in the initial phase
	 */
	void buildRings(const Cycle& cycle);

	/**
	 * Finds i0, the first step of the first "ladder" of rings that is not larger
	 * than n / delta nodes.
	 *
	 * @param delta the delta-parameter, ceil(sqrt( n / 2 ))
	 * @return i0, the index of the first ring of the ladder
	 */
	int find_i0(int delta) const;

	/**
	 * Finds the regions R1 and R2 formed by the separator cycle and an inner and outer ring.
	 * Fills \p region with the border nodes if R1 or R2 was big enough.
	 *
	 * @param region the list that will contain the border (i.e. the resulting separator)
	 * @param cycle the 2/3-separator found in an earlier phase
	 * @param inner the inner ring of R1 / R2
	 * @param outerIdx the index of the outer ring (which is potentially degenerate)
	 * @return true if a large enough region was found
	 */
	bool findRegions(List<node>& region, const Cycle& cycle, const Ring& inner, int outerIdx) const;

	/**
	 * Used in region construction: Walks along the 2/3-separator from \p startNode to \p endNode.
	 *
	 * @param startNode the node at which the walk starts
	 * @param endNode the node at which the walk ends
	 * @param regionBorder stores whether an edge is part of the region border
	 * @param region list to which the border nodes are added
	 */
	void walkAlongSeparator(node startNode, node endNode, EdgeArray<bool>& regionBorder,
			List<node>& region) const;

	/**
	 * Used in region construction: Walks along a Ring and stores nodes and edges of the section.
	 *
	 * @param ring the ring to be walked
	 * @param firstSection whether we walk from in to out (true, "normal") or out to in (false)
	 * @param invert whether we invert all adjEntries of the section
	 * @param regionBorder stores whether an edge belongs to the border or not
	 * @param region contains the border nodes
	 */
	void walkAlongRing(const Ring& ring, bool firstSection, bool invert,
			EdgeArray<bool>& regionBorder, List<node>& region) const;

	/**
	 * Checks whether the inside [outside] of the region defined by regionBorder is greater or smaller than 1/3 f.
	 *
	 * @param startNode a node on the region border
	 * @param regionBorder an EdgeArray that is true if that edge is on the border
	 * @param inside whether we want the inside of the region or its outside
	 * @param regionSize the number of nodes on the region border
	 * @return true if the region is greater than 1/3 f
	 */
	bool testRegionSize(node startNode, const EdgeArray<bool>& regionBorder, bool inside,
			int regionSize) const;

	/**
	 * Builds the region R1 or R2.
	 *
	 * @param region the list of nodes that form the border of the region
	 * @param cycle the separator cycle
	 * @param inner the inner ring
	 * @param outer the outer ring
	 * @param inside whether we want R1 (true = the inside of the cycle) or R2 (false = the outside of the cycle)
	 * @return true if it found a region that was large enough
	 */
	bool findRegion(List<node>& region, const Cycle& cycle, const Ring& inner, const Ring& outer,
			bool inside) const;

	/**
	 * Builds the region K.
	 *
	 * @param region the list of nodes that form the border of the region
	 * @param cycle the separator cycle
	 * @param inner the inner ring
	 * @param outer the outer ring
	 * @return true if everything worked
	 */
	bool constructK(List<node>& region, const Cycle& cycle, const Ring& inner,
			const Ring& outer) const;

	/**
	 * Takes a list of nodes of the GraphCopy and removes their counterparts from the original graph, thereby
	 * separating the graph. Also sets the exitPoint.
	 *
	 * @param exit the identifier of the exit point
	 * @param region the separator nodes in the copy
	 * @param separator separator nodes in the original graph
	 * @param first first half of the separation
	 * @param second second half of the separation
	 * @return true if everything worked
	 */
	bool finalize(std::string exit, const List<node>& region, List<node>& separator,
			List<node>& first, List<node>& second);

private:
	ConstCombinatorialEmbedding E;
	node psi; // the deeper of the two endpoints of tree-separator u,v

	FaceArray<int> faceLevels; // holds for each face which level it inhabits
	EdgeArray<int> border; // holds for every edge for which value i it lies on the border of region P<=i
	List<List<face>> faceFrontiers; // for each level of the expansion, holds the list of faces that form the inner lining of the ring

	/**
	 * hold for every node the segment of the border between levels i-1 and i that runs through this node
	 * (this has to be a list because there might be more than 2)
	 * */
	NodeArray<List<adjEntry>> ringIn;
	NodeArray<List<adjEntry>> ringOut;
	NodeArray<bool> isMultiNode; // holds whether this node has more than 2 connecting edges

	Array<Ring, int> rings; // the concentric rings of nodes around the root

	NodeArray<adjEntry> mainSeparator; // represents the big separator in a way that allows immediate access
};

namespace planar_separators {

/**
 * \brief A closed ring of nodes.
 * A representation of the border of a closed region containing the root node.
 */
struct Ring {
	/** Nodes and adjEntries that form the border of the ring. */
	List<node> nodes;
	List<adjEntry> entries;

	/** Crossing points with the main separator S: in is where S enters, out is where S leaves. */
	node in;
	node out;

	int faces; // number of faces inside this ring

	/** A degenerate ring contains only one node. */
	bool isDegenerate = false;

	/**
	 * Constructor.
	 */
	Ring() { } // to be able to store them in arrays

	/**
	 * Constructor for degenerate rings.
	 *
	 * @param n the only node on the ring
	 */
	Ring(node n) : in {n}, out {n}, isDegenerate {true} { }

	/**
	 * \brief Constructor for a full ring.
	 * Constructs a ring by following the border edges of the region on the inside. If a complicated
	 * node is encountered, we take the next clockwise out-edge of that node.
	 *
	 * @param startNode first node on the ring, also intersection point of ring and separator
	 * @param endNode second intersection point of ring and separator
	 * @param outPointer next adjEntry in the separator that points from startnode in the direction of psi
	 * @param separator the separator that uses this ring
	 */
	Ring(node startNode, node endNode, adjEntry outPointer, const SeparatorHarPeled& separator) {
		OGDF_ASSERT(outPointer->theNode() == startNode);

		nodes.pushBack(startNode);
		in = startNode;
		out = endNode;

		node next; // the next node in the ring
		adjEntry nextEntry; // the adjEntry that points to the next node

		// handling edge case of a multinode as startnode
		if (separator.isMultiNode[startNode]) {
			adjEntry outP = outPointer;
			while (separator.ringOut[startNode].search(outP) == separator.ringOut[startNode].end()) {
				outP = outP->cyclicSucc();
			}
			nextEntry = outP;
		} else {
			nextEntry = separator.ringOut[startNode].front();
		}
		next = nextEntry->twinNode();

		// walk along ring until we arrive back at startNode
		while (next != startNode) {
			nodes.pushBack(next);
			entries.pushBack(nextEntry);

			// find next nextEntry
			if (separator.ringOut[next].size() > 1) {
				adjEntry candidate =
						nextEntry->twin()->cyclicSucc(); // start checking at the entry via which we entered
				while (separator.ringOut[next].search(candidate) == separator.ringOut[next].end()) {
					candidate = candidate->cyclicSucc();
				}
				nextEntry = candidate;
			} else if (separator.ringOut[next].size() == 1) {
				nextEntry = separator.ringOut[next].front();
			} else {
				OGDF_ASSERT(false); // this should not happen, only the root is not part of a ring
			}
			next = nextEntry->twinNode();
		}
		entries.pushBack(nextEntry); // to close the ring, point back to first node
	}

	/**
	 * @return the number of faces inside this ring
	 */
	int getFaces() const { return faces; }

	/**
	 * @return size (=number of nodes) of this ring
	 */
	int getSize() const { return nodes.size(); }

	/**
	 * Returns a section of this ring, either the first one from in to out, or the second one from out to in.
	 *
	 * @param firstSection whether to return the first (true) or second (false) section
	 * @return a ring section
	 */
	List<adjEntry> getSectionInSeparator(bool firstSection) const {
		List<adjEntry> list;

		if (isDegenerate) {
			return list; // degenerate ring is empty
		}

		auto adjIt = entries.cbegin();

		if (firstSection) {
			// go start to end
			while (adjIt != entries.cend() && (*adjIt)->theNode() != out) {
				list.pushBack(*adjIt);
				++adjIt;
			}
		} else {
			// go end to start
			while ((*adjIt)->theNode() != out && adjIt != entries.cend()) {
				adjIt++;
			}
			while (adjIt != entries.cend()) {
				list.pushBack(*adjIt);
				++adjIt;
			}
		}
		return list;
	}
};

}
}
