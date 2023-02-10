#ifndef CUSTOMOGDFTREELAYOUT_H
#define CUSTOMOGDFTREELAYOUT_H

#include "ogdf/tree/TreeLayout.h"

namespace ogdf{
    class TreeLayout;
    class GraphAttributes;
    class Graph;
}

    /**
     * \brief The tree layout algorithm.
     *
     * The class TreeLayout represents the improved version of the tree layout
     * algorithm by Walker presented in:
     *
     * Christoph Buchheim, Michael Jünger, Sebastian Leipert: <i>Drawing
     * rooted trees in linear time</i>. Software: Practice and Experience 36(6),
     * pp. 651-665, 2006.
     *
     * The algorithm also allows to lay out a forest, i.e., a collection of trees.
     *
     * <H3>Optional parameters</H3>
     * Tree layout provides various optional parameters.
     *
     * <table>
     *   <tr>
     *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
     *   </tr><tr>
     *     <td><i>siblingDistance</i><td>double<td>20.0
     *     <td>The horizontal spacing between adjacent sibling nodes.
     *   </tr><tr>
     *     <td><i>subtreeDistance</i><td>double<td>20.0
     *     <td>The horizontal spacing between adjacent subtrees.
     *   </tr><tr>
     *     <td><i>levelDistance</i><td>double<td>50.0
     *     <td>The vertical spacing between adjacent levels.
     *   </tr><tr>
     *     <td><i>treeDistance</i><td>double<td>50.0
     *     <td>The horizontal spacing between adjacent trees in a forest.
     *   </tr><tr>
     *     <td><i>orthogonalLayout</i><td>bool<td>false
     *     <td>Determines whether edges are routed in an orthogonal
     *     or straight-line fashion.
     *   </tr><tr>
     *     <td><i>orientation</i><td> #Orientation <td> #topToBottom
     *     <td>Determines if the tree is laid out in a top-to-bottom,
     *     bottom-to-top, left-to-right, or right-to-left fashion.
     *   </tr><tr>
     *     <td><i>selectRoot</i><td> #RootSelectionType <td> RootSelectionType::Source
     *     <td>Determines how to select the root of the tree(s). Possible
     *     selection strategies are to take a (unique) source or sink in
     *     the graph, or to use the coordinates and to select the topmost
     *     node for top-to-bottom orientation, etc.
     *   </tr>
     * </table>
     *
     * The spacing between nodes is determined by the <i>siblingDistance</i>,
     * <i>subtreeDistance</i>, <i>levelDistance</i>, and <i>treeDistance</i>.
     * The layout style is determined by <i>orthogonalLayout</i> and
     * <i>orientation</i>; the root of the tree is selected according to
     * the selection strategy given by <i>selectRoot</i>.
     */
    class OGDF_EXPORT CustomOgdfTreeLayout : public ogdf::LayoutModule {
    public:
        //! Determines how to select the root of the tree.
        enum class RootSelectionType {
            Source, //!< Select a source in the graph.
            Sink,   //!< Select a sink in the graph.
            ByCoord   //!< Use the coordinates, e.g., select the topmost node if orientation is topToBottom.
        };

    private:
        double m_siblingDistance;        //!< The minimal distance between siblings.
        double m_subtreeDistance;        //!< The minimal distance between subtrees.
        double m_levelDistance;          //!< The minimal distance between levels.
        double m_treeDistance;           //!< The minimal distance between trees.

        bool m_orthogonalLayout;         //!< Option for orthogonal style (yes/no).
        ogdf::Orientation m_orientation;       //!< Option for orientation of tree layout.
        RootSelectionType m_selectRoot;  //!< Option for how to determine the root.

    public:
        //! Creates an instance of tree layout and sets options to default values.
        CustomOgdfTreeLayout();

        //! Destructor.
        ~CustomOgdfTreeLayout() = default;

        /**
         *  @name Algorithm call
         *  @{
         */

        /**
         * \brief Calls tree layout for graph attributes \p GA.
         *
         * \pre The graph is a tree or a forest.
         *
         * The order of children is given by the adjacency lists. The successor of the unique in-edge of a non-root node
         * leads to its leftmost child; the leftmost child of the root is given by its first adjacency entry.
         *
         * @param GA is the input graph and will also be assigned the layout information.
         */
        virtual void call(ogdf::GraphAttributes &GA) override;
        void callMultiLine(ogdf::GraphAttributes &AG, int numOfRows = 0);

        /** @}
         *  @name Optional parameters
         *  @{
         */

        //! Returns the the minimal required horizontal distance between siblings.
        double siblingDistance() const { return m_siblingDistance; }

        //! Sets the the minimal required horizontal distance between siblings to \p x.
        void siblingDistance(double x) { m_siblingDistance = x; }

        //! Returns the minimal required horizontal distance between subtrees.
        double subtreeDistance() const { return m_subtreeDistance; }

        //! Sets the minimal required horizontal distance between subtrees to \p x.
        void subtreeDistance(double x) { m_subtreeDistance = x; }

        //! Returns the minimal required vertical distance between levels.
        double levelDistance() const { return m_levelDistance; }

        //! Sets the minimal required vertical distance between levels to \p x.
        void levelDistance(double x) { m_levelDistance = x; }

        //! Returns the minimal required horizontal distance between trees in the forest.
        double treeDistance() const { return m_treeDistance; }

        //! Sets the minimal required horizontal distance between trees in the forest to \p x.
        void treeDistance(double x) { m_treeDistance = x; }

        //! Returns whether orthogonal edge routing style is used.
        bool orthogonalLayout() const { return m_orthogonalLayout; }

        //! Sets the option for orthogonal edge routing style to \p b.
        void orthogonalLayout(bool b) { m_orthogonalLayout = b; }

        //! Returns the option that determines the orientation of the layout.
        ogdf::Orientation orientation() const { return m_orientation; }

        //! Sets the option that determines the orientation of the layout to \p orientation.
        void orientation(ogdf::Orientation orientation) { m_orientation = orientation; }

        //! Returns the option that determines how the root is selected.
        RootSelectionType rootSelection() const { return m_selectRoot; }

        //! Sets the option that determines how the root is selected to \p rootSelection.
        void rootSelection(RootSelectionType rootSelection) { m_selectRoot = rootSelection; }


        /** @}
         *  @name Operators
         *  @{
         */

        //! Assignment operator.
        CustomOgdfTreeLayout &operator=(const CustomOgdfTreeLayout &tl);

        //! @}

    private:
        class AdjComparer;
        struct TreeStructure;

        // recursive bottom up traversal of the tree for computing
        // preliminary x-coordinates
        void firstWalk(TreeStructure &ts, ogdf::node subtree, bool upDown);

        // space out the small subtrees on the left hand side of subtree
        // defaultAncestor is used for all nodes with obsolete m_ancestor
        void apportion(
            TreeStructure &ts,
            ogdf::node subtree,
            ogdf::node &defaultAncestor,
            bool upDown);

        // recursive top down traversal of the tree for computing final
        // x-coordinates
        void secondWalkX(TreeStructure &ts, ogdf::node subtree, double modifierSum);
        void secondWalkY(TreeStructure &ts, ogdf::node subtree, double modifierSum);

        // compute y-coordinates and edge shapes
        void computeYCoordinatesAndEdgeShapes(ogdf::node root,ogdf::GraphAttributes &AG);
        void computeXCoordinatesAndEdgeShapes(ogdf::node root,ogdf::GraphAttributes &AG);

        void findMinX(ogdf::GraphAttributes &AG, ogdf::node root, double &minX);
        void findMinY(ogdf::GraphAttributes &AG, ogdf::node root, double &minY);
        void findMaxX(ogdf::GraphAttributes &AG, ogdf::node root, double &maxX);
        void findMaxY(ogdf::GraphAttributes &AG, ogdf::node root, double &maxY);
        void shiftTreeX(ogdf::GraphAttributes &AG, ogdf::node root, double shift);
        void shiftTreeY(ogdf::GraphAttributes &AG, ogdf::node root, double shift);
    };

#endif // CUSTOMOGDFTREELAYOUT_H
