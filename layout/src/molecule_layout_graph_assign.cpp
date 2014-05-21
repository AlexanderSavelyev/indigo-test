/****************************************************************************
 * Copyright (C) 2009-2013 GGA Software Services LLC
 * 
 * This file is part of Indigo toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include "base_cpp/profiling.h"
#include "layout/molecule_layout_graph.h"
#include "layout/molecule_layout_macrocycles.h"
#include "layout/attachment_layout.h"
#include "graph/biconnected_decomposer.h"
#include "graph/cycle_enumerator.h"
#include "graph/embedding_enumerator.h"
#include "graph/morgan_code.h"
#include "layout/layout_pattern.h"

#include <math/random.h>
#include <vector>
#include <algorithm>

using namespace indigo;

enum
{
   QUERY_BOND_SINGLE_OR_DOUBLE = 5,
   QUERY_BOND_SINGLE_OR_AROMATIC = 6,
   QUERY_BOND_DOUBLE_OR_AROMATIC = 7,
   QUERY_BOND_ANY = 8
};


// Make relative coordinates of a component absolute
void MoleculeLayoutGraph::_copyLayout (MoleculeLayoutGraph &component)
{
   int i;

   for (i = component.vertexBegin(); i < component.vertexEnd(); i = component.vertexNext(i))
   {
      LayoutVertex &vert = component._layout_vertices[i];

      _layout_vertices[vert.ext_idx].pos.copy(vert.pos);
      _layout_vertices[vert.ext_idx].type = vert.type;
   }

   for (i = component.edgeBegin(); i < component.edgeEnd(); i = component.edgeNext(i))
   {
      LayoutEdge &edge = component._layout_edges[i];

      _layout_edges[edge.ext_idx].type = edge.type;
   }
}

static int _vertex_cmp (int &n1, int &n2, void *context)
{
   const MoleculeLayoutGraph &graph = *(MoleculeLayoutGraph *)context;
   const LayoutVertex &v1 = graph.getLayoutVertex(n1);
   const LayoutVertex &v2 = graph.getLayoutVertex(n2);

   if (v1.is_cyclic != v2.is_cyclic)
   {
      if (v1.is_cyclic == true)
         return 1;
      return -1;
   }

   return v1.morgan_code - v2.morgan_code;
}

void MoleculeLayoutGraph::_assignAbsoluteCoordinates (float bond_length)
{
   BiconnectedDecomposer bc_decom(*this);
   QS_DEF(Array<int>, bc_tree);
   QS_DEF(ObjArray<MoleculeLayoutGraph>, bc_components);
   QS_DEF(Array<int>, fixed_components);
   bool all_trivial = true;

   int n_comp = bc_decom.decompose();

   fixed_components.clear_resize(n_comp);
   fixed_components.zerofill();

   bc_components.clear();

   for (int i = 0; i < n_comp; i++)
   {
      Filter comp;
      bc_decom.getComponent(i, comp);
      bc_components.push().makeLayoutSubgraph(*this, comp);
   }
   
   bc_tree.clear_resize(vertexEnd());
   _makeComponentsTree(bc_decom, bc_components, bc_tree);

   // 1. Find biconnected components forming connected subgraph from fixed vertices
   _findFixedComponents(bc_decom, fixed_components, bc_components);

   all_trivial = _assignComponentsRelativeCoordinates(bc_components, fixed_components, bc_decom);

   _findFirstVertexIdx(n_comp, fixed_components, bc_components, all_trivial);

   int i, j = -1;

   // ( 1] atoms assigned absolute coordinates and adjacent to atoms not;
   //   assigned coordinates are put on a list;
   QS_DEF(Array<int>, assigned_list);
   QS_DEF(Array<int>, adjacent_list);

   while (true) 
   {
      if(cancellation && cancellation->isCancelled())
         throw Error("Molecule layout has been cancelled: %s", cancellation->cancelledRequestMessage());

      if (!_prepareAssignedList(assigned_list, bc_decom, bc_components, bc_tree))
         return;

      // ( 3.i] let k = 0  ( top of the list];;
      while (assigned_list.size() != 0)
      {
         //int middle = (rand()+1) % assigned_list.size();
         int middle = assigned_list.size() - 1;
         int last = assigned_list.size() - 1;
         int temp = assigned_list[middle];
         assigned_list[middle] = assigned_list[last];
         assigned_list[last] = temp;

         int k = assigned_list.pop();
         const Vertex &vert_k = getVertex(k);

         // ( 3.ii] a list of atoms adjacent to atom Uzel and not previously;
         //		 assigned coordinates is created and ordered with cyclic atoms;
         //       at the top of the list with descending ATCD numbers and acyclic atoms;
         //       at the bottom of the list with descending ATCD numbers;;
         adjacent_list.clear();

         for (i = vert_k.neiBegin(); i < vert_k.neiEnd(); i = vert_k.neiNext(i))
            if (_layout_vertices[vert_k.neiVertex(i)].type == ELEMENT_NOT_DRAWN)
               adjacent_list.push(vert_k.neiVertex(i));

         if (adjacent_list.size() == 0)
            break;

         // When all components outgoing from vertex are trivial (edges) then use tree algorithm
         all_trivial = true;

         for (i = 0; i < bc_decom.getIncomingCount(k); i++)
            if (!bc_components[bc_decom.getIncomingComponents(k)[i]].isSingleEdge())
            {
               all_trivial = false;
               break;
            }

         if (all_trivial && bc_tree[k] != -1 && !bc_components[bc_tree[k]].isSingleEdge())
            all_trivial = false;

         if  (all_trivial)
         {
            adjacent_list.qsort(_vertex_cmp, this);

            _attachDandlingVertices(k, adjacent_list);
         } 
         else
         {
            // Component layout in current vertex should have the same angles between components.
            // So it depends on component order and their flipping (for nontrivial components)
            AttachmentLayout att_layout(bc_decom, bc_components, bc_tree, *this, k);

            // ( 3.iii] Look over all possible orders of component layouts
            //         (vertex itself is already drawn means one component is already drawn)
            // ( 3.iv]  Choose layout with minimal energy
            LayoutChooser layout_chooser(att_layout);

            layout_chooser.perform();

            att_layout.markDrawnVertices();
         }
         // ( 3.v] let k = k + 1;;
         // ( 3.vi] repeat steps 3.ii-3.v until all atoms in the list have been processed;;
      }
      // ( 4] repeat steps 1-3 until all atoms have been assigned absolute coordinates.;
   }
}

void MoleculeLayoutGraph::_assignRelativeCoordinates (int &fixed_component, const MoleculeLayoutGraph &supergraph)
{
   int i;

   if (isSingleEdge())
   {
      _assignRelativeSingleEdge(fixed_component, supergraph);
      return;
   }

   //	2.1. Use layout of fixed components and find border edges and vertices
   if (fixed_component)
   {
      for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         _layout_vertices[i].pos = supergraph.getPos(getVertexExtIdx(i));

      CycleEnumerator ce(*this);

      ce.context = this;
      ce.cb_handle_cycle = _border_cb;

      if (ce.process())
         return;

      fixed_component = 0;
   } 
   else
   { 
      //if (PatternLayoutFinder::tryToFindPattern(*this))
         //return;
   }

   //TODO: repair exception with vec2f

   QS_DEF(ObjPool<Cycle>, cycles);
   QS_DEF(Array<int>, sorted_cycles);
    
   cycles.clear();
   int n_cycles = sssrCount();
   
   for (i = 0; i < n_cycles; i++)
   {
      int cycle_idx = cycles.add(sssrEdges(i), *this);
      
      cycles[cycle_idx].canonize();
   }

   sorted_cycles.clear();
   for (i = cycles.begin(); i < cycles.end(); i = cycles.next(i))
   {
      cycles[i].calcMorganCode(*this);
      sorted_cycles.push(i);
   }

   sorted_cycles.qsort(Cycle::compare_cb, &cycles);

   while (cycles.size() != 0) {

      _assignFirstCycle(cycles[sorted_cycles[0]]);

      cycles.remove(sorted_cycles[0]);
      sorted_cycles.remove(0);
   }

   bool chain_attached;

   // Try to attach chains with one, two or more common edges outside drawn part
   do 
   {
      chain_attached = false;

      for (i = 0; !chain_attached && i < sorted_cycles.size(); )
      {
         if (_attachCycleOutside(cycles[sorted_cycles[i]], 1.f, 1))
         {
            cycles.remove(sorted_cycles[i]);
            sorted_cycles.remove(i);
            chain_attached = true;
         } else
            i++;
      }

      for (i = 0; !chain_attached && i < sorted_cycles.size(); )
      {
         if (_attachCycleOutside(cycles[sorted_cycles[i]], 1.f, 2))
         {
            cycles.remove(sorted_cycles[i]);
            sorted_cycles.remove(i);
            chain_attached = true;
         } else
            i++;
      }

      for (i = 0; !chain_attached && i < sorted_cycles.size(); )
      {
         if (_attachCycleOutside(cycles[sorted_cycles[i]], 1.f, 0))
         {
            cycles.remove(sorted_cycles[i]);
            sorted_cycles.remove(i);
            chain_attached = true;
         } else
            i++;
      }
   } while (chain_attached);

   // Try to attach chains inside
   for (i = 0; i < sorted_cycles.size(); )
   {
      if (_attachCycleInside(cycles[sorted_cycles[i]], 1.f))
      {
         cycles.remove(sorted_cycles[i]);
         sorted_cycles.remove(i);
      } else
         i++;
   }


   // Try to attach chains inside with lower edge length
   for (i = 0; i < sorted_cycles.size(); )
   {
      if (_attachCycleInside(cycles[sorted_cycles[i]], 0.75f))
      {
         cycles.remove(sorted_cycles[i]);
         sorted_cycles.remove(i);
      } else
         i++;
   }

   do 
   {
      chain_attached = false;

      for (i = 0; !chain_attached && i < sorted_cycles.size(); )
      {
         // 1.5f (> 1) means to calculate new length;
         if (_attachCycleOutside(cycles[sorted_cycles[i]], 1.5f, 0))
         {
            cycles.remove(sorted_cycles[i]);
            sorted_cycles.remove(i);
            chain_attached = true;
         } else
            i++;
      }
   } while (chain_attached);

   do 
   {
      chain_attached = false;

      for (i = 0; !chain_attached && i < sorted_cycles.size(); )
      {
         if (_attachCycleWithIntersections(cycles[sorted_cycles[i]], 1.f))
         {
            cycles.remove(sorted_cycles[i]);
            sorted_cycles.remove(i);
            chain_attached = true;
         } else
            i++;
      }
   } while (chain_attached);

   _attachCrossingEdges();

   for (i = edgeBegin(); i < edgeEnd(); i = edgeNext(i))
   {
      if (_layout_edges[i].type == ELEMENT_NOT_PLANAR)
      {
         _buildOutline();
         break;
      }
   }


}

void MoleculeLayoutGraph::_assignFirstCycle (const Cycle &cycle)
{

   profTimerStart(t, "layout.prepearing");

   const int size = cycle.vertexCount();
   //printf("%d do layout cycle \n", size);

   QS_DEF(ObjArray<MoleculeLayoutSmoothingSegment>, segment);
   QS_DEF(Array<Vec2f>, rotation_point);
   QS_DEF(Array<int>, rotation_vertex);

   _segment_smoothing_prepearing(cycle, rotation_vertex, rotation_point, segment);

   QS_DEF(Array<int>, _is_vertex_taken);
   enum {
      NOT_CONSIDERED,
      IN_LIST,
      NOT_IN_LIST
   };
   QS_DEF(Array<int>, _list_of_vertex);
   QS_DEF(Array<int>, _segment_weight_outside);

   _segment_weight_outside.clear_resize(segment.size());
   _segment_weight_outside.zerofill();

   for (int i = 0; i < size; i++) 
      if (_layout_component_number[cycle.getEdge(i)] < 0) _layout_component_number[cycle.getEdge(i)] = _layout_component_count;

   _layout_component_count++;

   QS_DEF(Array<bool>, _is_layout_component_incoming);
   _is_layout_component_incoming.clear_resize(_layout_component_count);
   _is_layout_component_incoming.zerofill();
   for (int i = 0; i < size; i++) 
      _is_layout_component_incoming[_layout_component_number[cycle.getEdge(i)]] = true;


   for (int i = 0; i < segment.size(); i++) {
      for (int up = 0; up <= 1; up++) {
         _is_vertex_taken.clear_resize(_graph->vertexEnd());
         _is_vertex_taken.fill(NOT_CONSIDERED);

         if (i == segment.size() - 1) {
            int x = 5;
         }

         _list_of_vertex.clear_resize(0);

         bool is_segment_trivial = segment[i]._graph.vertexCount() == 2 && segment[(i + segment.size() - 1) % segment.size()]._graph.vertexCount() == 2 && up;

         for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v)) {
            if ((!segment[i].is_finish(v) && !segment[i].is_start(v) && segment[i].isVertexUp(v) ^ !up) ||
               (is_segment_trivial && !segment[i].is_finish(v))) {

               int ext_v = segment[i]._graph.getVertexExtIdx(v);
               _is_vertex_taken[getVertexExtIdx(ext_v)] = IN_LIST;
               _list_of_vertex.push(ext_v);
            }
         }

         bool touch_to_another_segment = false;

         for (int j = 0; j < _list_of_vertex.size(); j++) {
            const Vertex& vert = getVertex(_list_of_vertex[j]);
            for (int n = vert.neiBegin(); n != vert.neiEnd(); n = vert.neiNext(n)) {
               int vn = vert.neiVertex(n);

               if (_is_vertex_taken[getVertexExtIdx(vn)] != NOT_CONSIDERED) continue;

               bool is_this_comp = false;
               for (int n2 = getVertex(vn).neiBegin(); n2 != getVertex(vn).neiEnd(); n2 = getVertex(vn).neiNext(n2))
               if (_layout_component_number[getVertex(vn).neiEdge(n2)] >= 0) {
                  if (_is_layout_component_incoming[_layout_component_number[getVertex(vn).neiEdge(n2)]]) is_this_comp = true;
                  if (!is_segment_trivial && _layout_component_number[getVertex(vn).neiEdge(n2)] != segment[i].get_layout_component_number()
                                          && _layout_component_number[getVertex(vn).neiEdge(n2)] != _layout_component_count - 1) 
                                             touch_to_another_segment = true;
               }



               if (!is_this_comp) {
                  _list_of_vertex.push(vn);
                  _is_vertex_taken[getVertexExtIdx(vn)] = IN_LIST;
               } else _is_vertex_taken[getVertexExtIdx(vn)] = NOT_IN_LIST;
            }
         }

         for (int j = 0; j < _list_of_vertex.size(); j++)
            _list_of_vertex[j] = getVertexExtIdx(_list_of_vertex[j]);

         for (int j = 0; j < _list_of_vertex.size(); j++) {
            const Vertex& vert = _graph->getVertex(_list_of_vertex[j]);
            for (int n = vert.neiBegin(); n != vert.neiEnd(); n = vert.neiNext(n)) {
               int vn = vert.neiVertex(n);

               if (_is_vertex_taken[vn] != NOT_CONSIDERED) continue;

               _list_of_vertex.push(vn);
               _is_vertex_taken[vn] = IN_LIST;
            }
         }

         _segment_weight_outside[i] += (up ? 1 : -1) * (touch_to_another_segment ? 3 : 1) * _list_of_vertex.size();
      }
   }

   MoleculeLayoutMacrocycles layout(size);

   QS_DEF(Array<int>, _index_in_cycle);
   _index_in_cycle.clear_resize(vertexEnd());
   _index_in_cycle.fffill();
   for (int i = 0; i < size; i++) _index_in_cycle[cycle.getVertex(i)] = i;

   for (int i = 0; i < segment.size(); i++) {
      if (segment[i]._graph.vertexCount() == 2 && segment[(i + segment.size() - 1) % segment.size()]._graph.vertexCount() == 2)
         layout.addVertexOutsideWeight(rotation_vertex[i], _segment_weight_outside[i] - 1);
      else {
         double y1 = 0, y2 = 0;
         for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v)) {
            if (_index_in_cycle[segment[i]._graph.getVertexExtIdx(v)] == (rotation_vertex[i] + 1) % size) {
               y1 = segment[i].getIntPosition(v).y;
            }
            if (_index_in_cycle[segment[i]._graph.getVertexExtIdx(v)] == (rotation_vertex[(i + 1) % segment.size()] + size - 1) % size) {
               y2 = segment[i].getIntPosition(v).y;
            }
         }

         if ((y1 + y2)/2 > segment[i].getIntCenter().y) {
            layout.addVertexOutsideWeight(rotation_vertex[i], -_segment_weight_outside[i]);
            layout.addVertexOutsideWeight(rotation_vertex[(i + 1) % segment.size()], -_segment_weight_outside[i]);
         }
         else {
            layout.addVertexOutsideWeight(rotation_vertex[i], _segment_weight_outside[i]);
            layout.addVertexOutsideWeight(rotation_vertex[(i + 1) % segment.size()], _segment_weight_outside[i]);
         }            
      }
   }


   if (size <= 6)
   for (int i = 0; i < size; i++)
      _molecule->cis_trans.setParity(_layout_edges[cycle.getEdge(i)].orig_idx, 0);

   for (int i = 0; i < size; i++) {

      // edge parallels

      int order_next = 0;
      int edge_number = cycle.getEdge(i);
      LayoutEdge edge = _layout_edges[edge_number];
      int ext_edge_number = edge.orig_idx;
      int order = _molecule->getBondOrder(ext_edge_number);
      switch (order) {
         case BOND_SINGLE : order_next = 1; break;
         case BOND_DOUBLE : order_next = 2; break;
         case BOND_TRIPLE : order_next = 3; break;
         default: order_next = 1;
      }
      int order_prev;
      int ext_edge_number_prev = _layout_edges[cycle.getEdgeC(i - 1)].orig_idx;
      switch (_molecule->getBondOrder(ext_edge_number_prev)) {
         case BOND_SINGLE : order_prev = 1; break;
         case BOND_DOUBLE : order_prev = 2; break;
         case BOND_TRIPLE : order_prev = 3; break;
         default: order_prev = 1;

      }


      layout.setVertexEdgeParallel(i, order_next + order_prev >= 4);


      // tras-cis configuration
      int next_vertex = _layout_vertices[cycle.getEdgeFinish(i + 1)].orig_idx;
      int prev_vertex = _layout_vertices[cycle.getEdgeStart(i - 1)].orig_idx;


      if (_molecule->cis_trans.getParity(ext_edge_number)) {
         int _sameside = _molecule->cis_trans.sameside(ext_edge_number, prev_vertex, next_vertex);
         if (_sameside) layout.setEdgeStereo(i, MoleculeCisTrans::CIS);
         else layout.setEdgeStereo(i, MoleculeCisTrans::TRANS);
      } else {
//         if (_layout_vertices[cycle.getVertex(i)].type != ELEMENT_NOT_DRAWN &&
  //          _layout_vertices[cycle.getVertex((i + 1) % size)].type != ELEMENT_NOT_DRAWN) {
         if (_layout_edges[cycle.getEdge(i)].type != ELEMENT_NOT_DRAWN) {

               Vec2f prev_point;
               if (_layout_edges[cycle.getEdgeC(i - 1)].type != ELEMENT_NOT_DRAWN) prev_point = _layout_vertices[cycle.getVertexC(i - 1)].pos;
               else {
                  for (int j = getVertex(cycle.getVertex(i)).neiBegin(); j != getVertex(cycle.getVertex(i)).neiEnd(); j = getVertex(cycle.getVertex(i)).neiNext(j))
                     if (_layout_edges[getVertex(cycle.getVertex(i)).neiEdge(j)].type != ELEMENT_NOT_DRAWN &&
                        getVertex(cycle.getVertex(i)).neiVertex(j) != cycle.getVertexC(i + 1))
                        prev_point = _layout_vertices[getVertex(cycle.getVertex(i)).neiVertex(j)].pos;
               }

               Vec2f next_point;
               if (_layout_edges[cycle.getEdgeC(i + 1)].type != ELEMENT_NOT_DRAWN) next_point = _layout_vertices[cycle.getVertexC(i + 2)].pos;
               else {
                  for (int j = getVertex(cycle.getVertexC(i + 1)).neiBegin(); j != getVertex(cycle.getVertexC(i + 1)).neiEnd(); j = getVertex(cycle.getVertexC(i + 1)).neiNext(j))
                     if (_layout_edges[getVertex(cycle.getVertexC(i + 1)).neiEdge(j)].type != ELEMENT_NOT_DRAWN &&
                        getVertex(cycle.getVertexC(i + 1)).neiVertex(j) != cycle.getVertex(i))
                        next_point = _layout_vertices[getVertex(cycle.getVertexC(i + 1)).neiVertex(j)].pos;
               }

               int _sameside = _isCisConfiguratuin(prev_point,
                                                  _layout_vertices[cycle.getVertexC(i)].pos,
                                                  _layout_vertices[cycle.getVertexC(i + 1)].pos,
                                                  next_point);

               if (_layout_edges[cycle.getEdgeC(i - 1)].type != ELEMENT_NOT_DRAWN &&
                  _layout_edges[cycle.getEdgeC(i + 1)].type != ELEMENT_NOT_DRAWN) {
                  if (_sameside) layout.setEdgeStereo(i, MoleculeCisTrans::CIS);
                  else layout.setEdgeStereo(i, MoleculeCisTrans::TRANS);
                     
               } else {
                  if ((_layout_edges[cycle.getEdgeC(i - 1)].type != ELEMENT_NOT_DRAWN) ^
                  (_layout_edges[cycle.getEdgeC(i + 1)].type != ELEMENT_NOT_DRAWN)) {
                     if (_sameside) layout.setEdgeStereo(i, MoleculeCisTrans::TRANS);
                     else layout.setEdgeStereo(i, MoleculeCisTrans::CIS);
                  } else layout.setEdgeStereo(i, MoleculeCisTrans::CIS);
               }
         }
      }
      
      layout.setVertexDrawn(i, _layout_vertices[cycle.getVertex(i)].type != ELEMENT_NOT_DRAWN);

      // trees sizes

   }

   for (int i = 0; i < segment.size(); i++) {
      for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v)) {
         if (segment[i].is_start(v)) if (segment[i]._graph.getVertex(v).degree() > 2) layout.setEdgeStereo(rotation_vertex[i], 0);
         if (segment[i].is_finish(v)) if (segment[i]._graph.getVertex(v).degree() > 2) layout.setEdgeStereo((rotation_vertex[(i + 1) % segment.size()] - 1 + size) % size, 0);
      }
   }

   layout.doLayout();

   // now we must to smooth just made layout
   // lets check if all cycle is layouted ealier in single biconnected compenent

   
   /*int start = -1;
   bool undrawn = false;
   for (int i = 0; i < size; i++) undrawn |= _layout_vertices[cycle.getVertex(i)].type == ELEMENT_NOT_DRAWN;
   if (undrawn) {
      for (int i = size - 1; i >= 0; i--) if (_layout_vertices[cycle.getVertex(i)].type != ELEMENT_NOT_DRAWN) start = i;
      if (start == 0 && _layout_vertices[cycle.getVertex(size - 1)].type != ELEMENT_NOT_DRAWN) {
         while (_layout_vertices[cycle.getVertex(start)].type != ELEMENT_NOT_DRAWN) start = (start + 1) % size;
         while (_layout_vertices[cycle.getVertex(start)].type == ELEMENT_NOT_DRAWN) start = (start + 1) % size;
      }
   }*/

   QS_DEF(Array<bool>, need_to_insert);
   need_to_insert.clear_resize(size);
   need_to_insert.zerofill();
   
   for (int i = 0; i < size; i++)
      need_to_insert[i] = _layout_vertices[cycle.getVertex(i)].type != ELEMENT_NOT_DRAWN;

   int start = 0;

   bool componentIsWholeCycle = false;

   QS_DEF(Array<bool>, _is_component_touch);
   _is_component_touch.clear_resize(_layout_component_count);

   for (int index = 0; index < size; index++) if (need_to_insert[index]) {
      // 1. search of connected component
      QS_DEF(Array<int>, insideVertex);
      insideVertex.clear_resize(0);
      insideVertex.push(cycle.getVertex(index));

      QS_DEF(Array<bool>, takenVertex);
      takenVertex.clear_resize(vertexCount());
      takenVertex.zerofill();
      takenVertex[cycle.getVertex(index)] = true;

      _is_component_touch.zerofill();

      for (int i = 0; i < insideVertex.size(); i++)
         for (int j = getVertex(insideVertex[i]).neiBegin(); j != getVertex(insideVertex[i]).neiEnd(); j = getVertex(insideVertex[i]).neiNext(j)) {
            int vertj = getVertex(insideVertex[i]).neiVertex(j);
            if (_layout_edges[getVertex(insideVertex[i]).neiEdge(j)].type != ELEMENT_NOT_DRAWN && !takenVertex[vertj]) {
               _is_component_touch[_layout_component_number[getVertex(insideVertex[i]).neiEdge(j)]] = true;
               insideVertex.push(vertj);
               takenVertex[vertj] = true;
            }
         }
         
      if (!componentIsWholeCycle) {
         componentIsWholeCycle = true;
         for (int i = 0; i < size; i++)
            componentIsWholeCycle &= takenVertex[cycle.getVertex(i)];
      }

      for (int i = 0; i < size; i++)
         if (takenVertex[cycle.getVertex(i)]) need_to_insert[i] = false;

      if (componentIsWholeCycle) break;

      int startIndex = index;
      int endIndex = index;

      while (takenVertex[cycle.getVertex(startIndex)]) startIndex = (startIndex - 1 + size) % size;
      startIndex = (startIndex + 1) % size;
      while (takenVertex[cycle.getVertex(endIndex)]) endIndex = (endIndex + 1) % size;

      // 2. flip
      bool need_to_flip = false;
      float rotate1 = Vec2f::cross(layout.getPos((startIndex + 1) % size) - layout.getPos(startIndex), layout.getPos((startIndex + 2) % size) - layout.getPos((startIndex + 1) % size));
      Vec2f next_point;
      if (_layout_edges[cycle.getEdgeC(startIndex + 1)].type != ELEMENT_NOT_DRAWN) next_point = _layout_vertices[cycle.getVertexC(startIndex + 2)].pos;
      else {
         for (int j = getVertex(cycle.getVertexC(startIndex + 1)).neiBegin(); j != getVertex(cycle.getVertexC(startIndex + 1)).neiEnd(); j = getVertex(cycle.getVertexC(startIndex + 1)).neiNext(j))
            if (_layout_edges[getVertex(cycle.getVertexC(startIndex + 1)).neiEdge(j)].type != ELEMENT_NOT_DRAWN &&
                     getVertex(cycle.getVertexC(startIndex + 1)).neiVertex(j) != cycle.getVertex(startIndex))
                     next_point = _layout_vertices[getVertex(cycle.getVertexC(startIndex + 1)).neiVertex(j)].pos;
      }

      float rotate2 = Vec2f::cross(_layout_vertices[cycle.getVertex((startIndex + 1) % size)].pos - _layout_vertices[cycle.getVertex((startIndex + 0) % size)].pos,
                                 next_point - _layout_vertices[cycle.getVertex((startIndex + 1) % size)].pos);

      if (_layout_edges[cycle.getEdgeC(startIndex + 1)].type == ELEMENT_NOT_DRAWN) {
         need_to_flip = rotate1 * rotate2 > 0;
      }
      else if (_layout_edges[cycle.getEdgeC(startIndex)].type != ELEMENT_NOT_DRAWN) need_to_flip = rotate1 * rotate2 < 0;

      if (need_to_flip) {
         for (int i = 0; i < insideVertex.size(); i++)
            _layout_vertices[insideVertex[i]].pos.x *= -1;

         for (int i = 0; i < segment.size(); i++)
            if (segment[i].get_layout_component_number() >= 0 && _is_component_touch[segment[i].get_layout_component_number()])
               segment[i].inverse();
      }

           
      // 3. shift
         
      Vec2f middle_host;
      Vec2f middle_new;
      int countVertex = 0;
      for (int i = startIndex ; i != endIndex; i = (i + 1) % size) {
         middle_host += _layout_vertices[cycle.getVertex(i)].pos;
         middle_new += layout.getPos(i);
         countVertex++;
      }
      middle_host /= countVertex;
      middle_new /= countVertex;

      for (int i = 0; i < insideVertex.size(); i++)
         _layout_vertices[insideVertex[i]].pos += middle_new - middle_host;

      // 4. rotate

      Vec2f direction_host;
      Vec2f direction_new;
      if (countVertex > 1)  {
         int currentIndex = 0;
         for (int i = startIndex ; i != endIndex; i = (i + 1) % size) {
            if (2*currentIndex < countVertex - 1) {
               direction_host += _layout_vertices[cycle.getVertex(i)].pos;
               direction_new += layout.getPos(i);
            } else 
               if (2*currentIndex > countVertex - 1) {
               direction_host -= _layout_vertices[cycle.getVertex(i)].pos;
               direction_new -= layout.getPos(i);
            }
            currentIndex++;
         }
         
      float dot = Vec2f::dot(direction_host, direction_new)/(direction_host.length()*direction_new.length());
      if (dot > 1) dot = 1;
      if (dot < -1) dot = -1;
      float angle = acos(dot);
      if (Vec2f::cross(direction_host, direction_new) < 0) angle = -angle;
      for (int i = 0; i < insideVertex.size(); i++)
         _layout_vertices[insideVertex[i]].pos.rotateAroundSegmentEnd(_layout_vertices[insideVertex[i]].pos, middle_new, angle);
      }


   }


  for (int i = 0; i < size; i++)
      if (_layout_vertices[cycle.getVertex(i)].type == ELEMENT_NOT_DRAWN)
         _layout_vertices[cycle.getVertex(i)].pos = layout.getPos(i);

   _first_vertex_idx = cycle.getVertex(0);
   for (int i = 0; i < size; i++)
   {
      _layout_vertices[cycle.getVertex(i)].type = ELEMENT_DRAWN;
      _layout_edges[cycle.getEdge(i)].type = ELEMENT_DRAWN;
   }


   // 5. smoothing

   for (int e = edgeBegin(); e != edgeEnd(); e = edgeNext(e))
      if (_layout_component_number[e] >= 0 && _is_layout_component_incoming[_layout_component_number[e]])
         _layout_component_number[e] = _layout_component_count - 1;
 
   _segment_smoothing(cycle, layout, rotation_vertex, rotation_point, segment);
}

void MoleculeLayoutGraph::_segment_smoothing(const Cycle &cycle, const MoleculeLayoutMacrocycles &layout, Array<int> &rotation_vertex, Array<Vec2f> &rotation_point, ObjArray<MoleculeLayoutSmoothingSegment> &segment) {

   QS_DEF(Array<float>, target_angle);

   _segment_update_rotation_points(cycle, rotation_vertex, rotation_point, segment);
   _segment_calculate_target_angle(layout, rotation_vertex, target_angle, segment);

   if (segment.size() > 2) {
      _segment_smoothing_unstick(segment);
      _do_segment_smoothing(rotation_point, target_angle, segment);
   }
}

void MoleculeLayoutGraph::_segment_update_rotation_points(const Cycle &cycle, Array<int> &rotation_vertex, Array<Vec2f> &rotation_point, ObjArray<MoleculeLayoutSmoothingSegment> &segment) {
   for (int i = 0; i < rotation_vertex.size(); i++)
      rotation_point[i] = getPos(cycle.getVertex(rotation_vertex[i]));

   for (int i = 0; i < segment.size(); i++) segment[i].updateStartFinish();
}

void MoleculeLayoutGraph::_segment_calculate_target_angle(const MoleculeLayoutMacrocycles &layout, Array<int> &rotation_vertex, Array<float> &target_angle, ObjArray<MoleculeLayoutSmoothingSegment> &segment) {
   int segments_count = rotation_vertex.size();

   target_angle.clear_resize(segments_count);

   for (int i = 0; i < segments_count; i++) {
      Vec2f p1 = layout.getPos(rotation_vertex[(i - 1 + segments_count) % segments_count]);
      Vec2f p2 = layout.getPos(rotation_vertex[i]);
      Vec2f p3 = layout.getPos(rotation_vertex[(i + 1) % segments_count]);
      p1 = p2 - p1;
      p2 = p3 - p2;
      target_angle[i] = std::acos(Vec2f::dot(p1, p2) / p1.length() / p2.length());
      if (Vec2f::cross(p1, p2) < 0) target_angle[i] = -target_angle[i];
      target_angle[i] = PI - target_angle[i];
   }

   for (int i = 0; i < segments_count; i++)
      for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v)) {
         if (segment[i].is_start(v)) if (segment[i]._graph.getVertex(v).degree() > 2) target_angle[i] = 0;
         if (segment[i].is_finish(v)) if (segment[i]._graph.getVertex(v).degree() > 2) target_angle[(i + 1) % segments_count] = 0;
      }

}

void MoleculeLayoutGraph::_segment_smoothing_unstick(ObjArray<MoleculeLayoutSmoothingSegment> &segment) {

   profTimerStart(t, "unstick");
   int segment_count = segment.size();

   // prepearing of list of sticked pairs of vertices

   QS_DEF(Array<float>, min_x);
   min_x.clear_resize(segment_count);
   for (int i = 0; i < segment_count; i++)
      min_x[i] = segment[i].get_min_x();

   QS_DEF(Array<float>, max_x);
   max_x.clear_resize(segment_count);
   for (int i = 0; i < segment_count; i++)
      max_x[i] = segment[i].get_max_x();

   QS_DEF(Array<float>, min_y);
   min_y.clear_resize(segment_count);
   for (int i = 0; i < segment_count; i++)
      min_y[i] = segment[i].get_min_y();

   QS_DEF(Array<float>, max_y);
   max_y.clear_resize(segment_count);
   for (int i = 0; i < segment_count; i++)
      max_y[i] = segment[i].get_max_y();

   QS_DEF(Array<int>, component1);
   QS_DEF(Array<int>, component2);
   QS_DEF(Array<int>, vertex1);
   QS_DEF(Array<int>, vertex2);

   component1.clear_resize(0);
   component2.clear_resize(0);
   vertex1.clear_resize(0);
   vertex2.clear_resize(0);

   for (int i = 0; i < segment_count; i++)
      for (int j = (i + segment_count/2) % segment_count; j != i; j = (j + segment_count - 1) % segment_count) {

         if (segment_count % 2 == 0 && j + segment_count/2 == i) continue;

         if (min_x[i] <= max_x[j] && min_x[j] <= max_x[i] && min_y[i] <= max_y[j] && min_y[j] <= max_y[i]) {
            for (int v1 = segment[i]._graph.vertexBegin(); v1 != segment[i]._graph.vertexEnd(); v1 = segment[i]._graph.vertexNext(v1))
               for (int v2 = segment[j]._graph.vertexBegin(); v2 != segment[j]._graph.vertexEnd(); v2 = segment[j]._graph.vertexNext(v2)) {
   //                  if ((i + 1) % segment_count != j) printf("%10.10f \n", Vec2f::dist(segment[i].getPosition(v1), segment[j].getPosition(v2)));
                  if (Vec2f::distSqr(segment[i].getPosition(v1), segment[j].getPosition(v2)) < 0.1)
                     if ((i + 1) % segment_count != j || !segment[i].is_finish(v1)) {
                        component1.push(i);
                        component2.push(j);
                        vertex1.push(v1);
                        vertex2.push(v2);
                     }
               }
         }

      }

   int count_sticked_vertices = component1.size();
   
   bool something_done = true;
   bool something_to_do = false;
   while (something_done) {
      something_done = false;
      something_to_do = false;
      
      for (int index = 0; index < count_sticked_vertices; index++) {
         int i = component1[index];
         int j = component2[index];
         int v1 = vertex1[index];
         int v2 = vertex2[index];

         if (Vec2f::distSqr(segment[i].getPosition(v1), segment[j].getPosition(v2)) < EPSILON) {
            something_to_do = true;

            bool exist_sepatate_vertex = false;
            const Vertex &vert1 = segment[i]._graph.getVertex(v1);
            const Vertex &vert2 = segment[j]._graph.getVertex(v2);

            for (int u1 = vert1.neiBegin(); u1 != vert1.neiEnd() && !exist_sepatate_vertex; u1 = vert1.neiNext(u1)) {
               bool exist_same_vertex = false;
               int nei1 = vert1.neiVertex(u1);
               for (int u2 = vert2.neiBegin(); u2 != vert2.neiEnd() && !exist_same_vertex; u2 = vert2.neiNext(u2)) {
                  int nei2 = vert2.neiVertex(u2);
                  if (Vec2f::dist(segment[i].getPosition(nei1), segment[j].getPosition(nei2)) < EPSILON)
                     exist_same_vertex = true;
               }
               if (!exist_same_vertex) exist_sepatate_vertex = true;
            }

            if (exist_sepatate_vertex) {
               Vec2f direction;
               if (vert1.degree() == 2) {
                  direction = (segment[i].getPosition(vert1.neiVertex(vert1.neiBegin())) + 
                                 segment[i].getPosition(vert1.neiVertex(vert1.neiNext(vert1.neiBegin()))))/2;
                  direction -= segment[i].getPosition(v1);
               } else if (vert2.degree() == 2) {
                  direction = (segment[j].getPosition(vert2.neiVertex(vert2.neiBegin())) + 
                                 segment[j].getPosition(vert2.neiVertex(vert2.neiNext(vert2.neiBegin()))))/2;
                  direction -= segment[i].getPosition(v1);
               } else if (vert1.degree() == 1) {
                  direction = segment[i].getPosition(vert1.neiVertex(vert1.neiBegin()));
                  direction -= segment[i].getPosition(v1);
                  direction.rotate(1, 0);
               } else if (vert2.degree() == 1) {
                  direction = segment[j].getPosition(vert2.neiVertex(vert2.neiBegin()));
                  direction -= segment[i].getPosition(v1);
                  direction.rotate(1, 0);
               } else continue;

               direction *= 2;

               bool moved = false;
               for (int sign = 1; sign >= -1 && !moved; sign -= 2) {

                  Vec2f newpos = segment[i].getPosition(v1) + (direction * sign);
                  bool can_to_move = true;

                  for (int u1 = vert1.neiBegin(); u1 != vert1.neiEnd() && can_to_move; u1 = vert1.neiNext(u1)) {
                     int nei1 = vert1.neiVertex(u1);
                     for (int u2 = vert2.neiBegin(); u2 != vert2.neiEnd() && can_to_move; u2 = vert2.neiNext(u2)){
                        int nei2 = vert2.neiVertex(u2);
                        if (Vec2f::segmentsIntersectInternal(newpos, segment[i].getPosition(nei1), segment[j].getPosition(v2), segment[j].getPosition(nei2)))
                           can_to_move = false;
                     }
                  }

                  if (can_to_move) {
                     something_done = true;
                     moved = true;

                     segment[i].shiftStartBy(direction * sign / 2);
                     segment[i].shiftFinishBy(direction * sign / 2);

                     segment[j].shiftStartBy(direction * -sign / 2);
                     segment[j].shiftFinishBy(direction * -sign / 2);
                  }
               }
            }
         }
      }
               
   }

   for (int i = 0; i < segment_count; i++)
      for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v))
         getPos(segment[i]._graph.getVertexExtIdx(v)).copy(segment[i].getPosition(v));
}

void MoleculeLayoutGraph::_do_segment_smoothing(Array<Vec2f> &rotation_point, Array<float> &target_angle, ObjArray<MoleculeLayoutSmoothingSegment> &segment) {
   Random rand(34577);

   int segments_count = segment.size();
   
   for (int i = 0; i < 10000; i++)
      _segment_improoving(rotation_point, target_angle, segment, rand.next() % segments_count, 0.1);

   for (int i = 0; i < segments_count; i++)
      for (int v = segment[i]._graph.vertexBegin(); v != segment[i]._graph.vertexEnd(); v = segment[i]._graph.vertexNext(v))
         getPos(segment[i]._graph.getVertexExtIdx(v)).copy(segment[i].getPosition(v));

}

void MoleculeLayoutGraph::_segment_smoothing_prepearing(const Cycle &cycle, Array<int> &rotation_vertex, Array<Vec2f> &rotation_point, ObjArray<MoleculeLayoutSmoothingSegment> &segment) {
   profTimerStart(t, "smoothing.prepearing");

   int cycle_size = cycle.vertexCount();

   QS_DEF(Array<bool>, layout_comp_touch);
   layout_comp_touch.clear_resize(_layout_component_count);
   layout_comp_touch.zerofill();


   for (int i = 0; i < cycle_size; i++) {
      if (_layout_component_number[cycle.getEdge(i)] >= 0)
         layout_comp_touch[_layout_component_number[cycle.getEdge(i)]] = true;
   }

   QS_DEF(ObjArray<Filter>, segments_filter);
   segments_filter.clear();

   QS_DEF(Array<int>, segment_start);
   segment_start.clear_resize(0);
   segment_start.fffill();

   QS_DEF(Array<bool>, touch_to_currnet_component);
   touch_to_currnet_component.clear_resize(cycle_size);

   QS_DEF(Array<int>, segment_component_number);
   segment_component_number.clear();

   for (int i = 0; i < _layout_component_count; i++) if (layout_comp_touch[i]) {

      // search of vertices touch to i-th layout component
      touch_to_currnet_component.zerofill();
      for (int j = 0; j < cycle_size; j++) {
         const Vertex &vert = getVertex(cycle.getVertex(j));
         for (int nei = vert.neiBegin(); nei != vert.neiEnd(); nei = vert.neiNext(nei))
         if (getEdgeType(vert.neiEdge(nei)) != ELEMENT_NOT_DRAWN) {
            if (_layout_component_number[vert.neiEdge(nei)] == i)
               touch_to_currnet_component[j] = true;
         }
      }


      // search of start and finish of occupated segment of cycle
      // if there is at least two starts of finishes then it is separationg layout component
      int start = -1;
      int finish = -1;

      for (int j = 0; j < cycle_size; j++)
         if (touch_to_currnet_component[j] && _layout_component_number[cycle.getEdgeC(j - 1)] != i) {
            if (start != -1) throw Exception("Separating layout component in cycle\n");
            else start = j;
         }

      for (int j = 0; j < cycle_size; j++)
         if (touch_to_currnet_component[j] && _layout_component_number[cycle.getEdge(j)] != i) {
            if (finish != -1) throw Exception("Separating layout component in cycle\n");
            else finish = j;
         }

      if (start != finish) {
         segments_filter.push();
         segments_filter.top().initNone(vertexEnd());
         for (int e = edgeBegin(); e != edgeEnd(); e = edgeNext(e)) if (_layout_component_number[e] == i) {
            segments_filter.top().unhide(getEdge(e).beg);
            segments_filter.top().unhide(getEdge(e).end);
         }

         segment_start.push(start);
         segment_component_number.push(i);
      }
   }

   for (int i = 0; i < cycle_size; i++)
   if (_layout_component_number[cycle.getEdge(i)] < 0) {

      segment_start.push(i);

      segments_filter.push();
      segments_filter.top().initNone(vertexEnd());
      segments_filter.top().unhide(cycle.getVertex(i));
      segments_filter.top().unhide(cycle.getVertexC(i + 1));

      segment_component_number.push(-1);
   }

   int segments_count = segments_filter.size();

   if (segments_count == 0) return;

   QS_DEF(Array<int>, number_of_segment);
   number_of_segment.clear_resize(cycle_size);
   number_of_segment.fffill();
   for (int i = 0; i < segments_count; i++) number_of_segment[segment_start[i]] = i;

   rotation_vertex.clear_resize(0);
   for (int i = 0; i < cycle_size; i++)
      if (number_of_segment[i] != -1) rotation_vertex.push(segment_start[number_of_segment[i]]);

   rotation_point.clear_resize(segments_count);
   _segment_update_rotation_points(cycle, rotation_vertex, rotation_point, segment);

   QS_DEF(ObjArray<MoleculeLayoutGraph>, segment_graph);
   segment_graph.clear();
   for (int i = 0; i < segments_count; i++) {
      segment_graph.push().makeLayoutSubgraph(*this, segments_filter[i]);
   }

   segment.clear();

   int current_number = 0;
   for (int i = 0; i < cycle_size; i++) if (number_of_segment[i] != -1) {
      segment.push(segment_graph[number_of_segment[i]], rotation_point[current_number], rotation_point[(1 + current_number) % segments_count]);
      segment.top().set_start_finish_number(cycle.getVertex(rotation_vertex[current_number]), cycle.getVertex(rotation_vertex[(current_number + 1) % segments_count]));
      segment.top().set_layout_component_number(segment_component_number[number_of_segment[i]]);
      current_number++;
   }

}

void MoleculeLayoutGraph::_segment_improoving(Array<Vec2f> &point, Array<float> &target_angle, ObjArray<MoleculeLayoutSmoothingSegment> &segment, int move_vertex, float coef) {
   
   int segments_count = segment.size();
   Vec2f move_vector(0, 0);

   // fix angle
   Vec2f prev_point(point[(move_vertex + segments_count - 1) % segments_count]);
   Vec2f this_point(point[move_vertex]);
   Vec2f next_point(point[(move_vertex + 1) % segments_count]);



   if (abs(target_angle[move_vertex] - PI) > 0.001) {
      Vec2f chord(next_point - prev_point);

      Vec2f center(prev_point + chord/2);
      Vec2f rot_chord(chord);
      rot_chord.rotate(1, 0);
      center += rot_chord * std::tan(target_angle[move_vertex] - PI/2) /2;

      float radii = (prev_point - center).length();
      float dist = (this_point - center).length();

      move_vector += (this_point - center) * (radii - dist)/radii;
   } else {
      Vec2f center(prev_point * segment[(move_vertex + segments_count - 1) % segments_count].getLength() +
                   next_point * segment[move_vertex].getLength());

      center /= segment[(move_vertex + segments_count - 1) % segments_count].getLength() + segment[move_vertex].getLength();

      move_vector += (center - this_point);
   }


   // fix distance to neighborhoods
   move_vector += (this_point - next_point) * segment[move_vertex].getLengthCoef();
   move_vector += (this_point - prev_point) * segment[(move_vertex + segments_count - 1) % segments_count].getLengthCoef();

   // fix intersections to other components
   float min_dist = 0.7;
   for (int i = 0; i < segments_count; i++) if (i != move_vertex/* && (i + 1) % segments_count != move_vertex && i != (move_vertex + 1) % segments_count*/) {
      if (segment[move_vertex]._graph.vertexCount() == 2 && segment[i]._graph.vertexCount() == 2) continue;
      bool interseced = false;

      for (int v1 = segment[move_vertex]._graph.vertexBegin(); v1 != segment[move_vertex]._graph.vertexEnd() && !interseced; v1 = segment[move_vertex]._graph.vertexNext(v1))
         for (int v2 = segment[i]._graph.vertexBegin(); v2 != segment[i]._graph.vertexEnd() && !interseced; v2 = segment[i]._graph.vertexNext(v2)) {
            if ((move_vertex + 1) % segments_count == i && segment[move_vertex].is_finish(v1)) continue;
            if ((i + 1) % segments_count == move_vertex && segment[i].is_finish(v2)) continue;
            if (Vec2f::distSqr(segment[move_vertex].getPosition(v1), segment[i].getPosition(v2)) < min_dist * min_dist) interseced = true;
         }

      if (interseced) {
         Vec2f shift1(segment[move_vertex].getCenter());
         Vec2f shift2(segment[i].getCenter());
         Vec2f shift(shift1 - shift2);
         shift.normalize();
         shift /= 1;
         move_vector += shift;
      }
   }

   // apply
   point[move_vertex] += move_vector * coef;
}

// If vertices are already drawn
// draw edges with intersections
void MoleculeLayoutGraph::_attachCrossingEdges ()
{
   int i, j, pr;
   bool intersection;

   for (i = edgeBegin(); i < edgeEnd(); i = edgeNext(i))
   {
      const Edge &edge_i = getEdge(i);

      if (_layout_vertices[edge_i.beg].type != ELEMENT_NOT_DRAWN &&
          _layout_vertices[edge_i.end].type != ELEMENT_NOT_DRAWN &&
          _layout_edges[i].type == ELEMENT_NOT_DRAWN)
      {
         intersection = true;

         while (intersection)
         {
            intersection = false;

            for  (j = edgeBegin(); j < edgeEnd(); j = edgeNext(j))
            {
               if (_layout_edges[j].type != ELEMENT_NOT_DRAWN)
               {
                  pr = _calcIntersection(i, j);
                  // 1. If the edge 1 ends on the edge 2
                  // then shift edge 2 from edge 1 by epsilon orthogonally to the edge 2
                  if (pr == 222 || pr == 223)
                  {
                     _shiftEdge(j, 0.2f);
                     intersection = true;
                     break;
                  }
                  if (pr == 224 || pr == 225)
                  {
                     _shiftEdge(i, 0.2f);
                     intersection = true;
                     break;
                  }
                  // 2. If the edge 1 overlaps some other edge shift it by epsilon orthogonally
                  if (pr == 3 || pr == 4)
                  {
                     _shiftEdge(i, 0.2f);
                     intersection = true;
                     break;
                  }
               }
            }
         }
         _layout_edges[i].type = ELEMENT_NOT_PLANAR;
      }
   }
}

void MoleculeLayoutGraph::_buildOutline (void)
{
   Vec2f v, inter;
   Vec2f pos_i;
   int i, j;
   int first_idx = vertexBegin();
   float min_y = getPos(first_idx).y;
   const float EPS = 0.0001f;
   const float EPS_ANGLE = 1e-6f;

   for (i = vertexNext(first_idx); i < vertexEnd(); i = vertexNext(i))
   {
      if (getPos(i).y < min_y)
      {
         min_y = getPos(i).y;
         first_idx = i;
      }
   }

   i = first_idx;

   float max_angle, cur_angle;
   float i_angle = 0;
   int next_nei = 0;

   pos_i = getPos(i);

   if (_outline.get() == 0)
      _outline.create();
   else
      _outline->clear();

   while (true)
   {
      const Vertex &vert = getVertex(i);

      if (i != first_idx)
      {
         v = pos_i;
         pos_i = getPos(i);
         v.sub(pos_i);

         i_angle = v.tiltAngle2();
      } else if (_outline->size() > 0)
         break;
      
      _outline->push(pos_i);

      max_angle = 0.f;

      for (j = vert.neiBegin(); j < vert.neiEnd(); j = vert.neiNext(j))
      {
         const Vec2f &pos_nei = getPos(vert.neiVertex(j));

         v.diff(pos_nei, pos_i);

         cur_angle = v.tiltAngle2() - i_angle;

         // If cur_angle is almost zero but negative due to numeric errors (-1e-8) then 
         // on some structures the results are not stable and even inifinite loop appreas
         // Example of such structure: ClC1(C(=O)C2(Cl)C3(Cl)C14Cl)C5(Cl)C2(Cl)C3(Cl)C(Cl)(Cl)C45Cl
         if (fabs(cur_angle) < EPS_ANGLE)
            cur_angle = 0;
         if (cur_angle < 0.f)
            cur_angle += 2 * PI;

         if (max_angle < cur_angle)
         {
            max_angle = cur_angle;
            next_nei = j;
         }
      }

      i = vert.neiVertex(next_nei);

      float dist, min_dist = 0.f;
      int int_edge = -1;
      Vec2f cur_v1 = pos_i;
      Vec2f cur_v2 = getPos(i);
      int prev_edge = -1;
      int cur_edge = vert.neiEdge(next_nei);

      while (min_dist < 10000.f)
      {
         min_dist = 10001.f;

         for (j = edgeBegin(); j < edgeEnd(); j = edgeNext(j))
         {
            const Edge &edge = getEdge(j);
            const Vec2f &cur_v3 = getPos(edge.beg);
            const Vec2f &cur_v4 = getPos(edge.end);

            if (Vec2f::intersection(cur_v1, cur_v2, cur_v3, cur_v4, v))
               if ((dist = Vec2f::dist(cur_v1, v)) < min_dist)
               {
                  if (dist > EPS && j != prev_edge && j != cur_edge)
                  {
                     inter = v;
                     min_dist = dist;
                     int_edge = j;
                  }
               }
         }

         if (min_dist < 10000.f)
         {
            if (min_dist > EPSILON)
               _outline->push(v);
            
            const Edge &edge = getEdge(int_edge);
            const Vec2f &cur_v3 = getPos(edge.beg);
            const Vec2f &cur_v4 = getPos(edge.end);

            Vec2f cur_v1v;
            Vec2f cur_v3v;
            Vec2f cur_v4v;

            cur_v1v.diff(cur_v1, inter);
            cur_v3v.diff(cur_v3, inter);
            cur_v4v.diff(cur_v4, inter);

            float angle1 = cur_v1v.tiltAngle2();
            float angle3 = cur_v3v.tiltAngle2() - angle1;
            float angle4 = cur_v4v.tiltAngle2() - angle1;

            if (angle3 < 0)
               angle3 += 2 * PI;
            if (angle4 < 0)
               angle4 += 2 * PI;

            cur_v1 = inter;

            if (angle3 > angle4)
            {
               cur_v2 = cur_v3;
               i = edge.beg;
            } else
            {
               cur_v2 = cur_v4;
               i = edge.end;
            }
            
            prev_edge = cur_edge;
            cur_edge = int_edge;
         }
      }
   }
}

// Return 1 - with maximum code, 2 - neighbor of the 1 with maximum code
// 3 - neighbor with maximum code from the rest or -1 if it doesn't exist.
void MoleculeLayoutGraph::_getAnchor (int &v1, int &v2, int &v3) const
{
   int i;

   if (vertexCount() ==1)
   {
      v1 = v2 = vertexBegin();
      v3 = -1;
      return;
   }

   if (vertexCount() == 2)
   {
      v1 = vertexBegin();
      v2 = vertexNext(v1);
      v3 = -1;

      if (_layout_vertices[v1].morgan_code < _layout_vertices[v2].morgan_code)
      {
         v2 = vertexBegin();
         v1 = vertexNext(v2);
      }
      return;
   }

   v1 = vertexBegin();
   for (i = vertexNext(v1); i < vertexEnd(); i = vertexNext(i))
      if (_layout_vertices[i].morgan_code > _layout_vertices[v1].morgan_code)
         v1 = i;

   const Vertex &vert = getVertex(v1);

   v2 = vert.neiBegin();
   for (i = vert.neiNext(v2); i < vert.neiEnd(); i = vert.neiNext(i))
      if(_layout_vertices[vert.neiVertex(i)].morgan_code > _layout_vertices[vert.neiVertex(v2)].morgan_code)
         v2 = i;

   if (vert.degree() < 2)
   {
      v2 = vert.neiVertex(v2);
      v3 = -1;
      return;
   }
   
   v3 = vert.neiBegin();

   if (v3 == v2)
      v3 = vert.neiNext(v2);

   for (i = vert.neiBegin(); i < vert.neiEnd(); i = vert.neiNext(i))
      if (i != v2 && _layout_vertices[vert.neiVertex(i)].morgan_code > _layout_vertices[vert.neiVertex(v3)].morgan_code)
         v3 = i;

   v2 = vert.neiVertex(v2);
   v3 = vert.neiVertex(v3);
}

// Scale and transform
void MoleculeLayoutGraph::_assignFinalCoordinates (float bond_length, const Array<Vec2f> &src_layout)
{
   int i;

   if (_n_fixed > 0)
   {
      for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         _layout_vertices[i].pos.scale(bond_length);
      return;
   }

   if (vertexCount() == 1)
   {
      getPos(vertexBegin()).set(0.f, 0.f);
      return;
   }

   // Flip according to various rules
   if (_molecule != 0 && _n_fixed == 0)
   {
      if (_molecule->countRSites() > 1) 
      {
         // flip molecule vertically if R1 is not above other R-groups
         // flip molecule horizontally if R1 is not on the left
         QS_DEF(Array<int>, rgroup_list);
         Vec2f r1_pos, highest_pos(0.f, -1000.f);
         bool r1_exist = false;
         float center_x = 0.f;
      
         for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         {
            if (_molecule->isRSite(_layout_vertices[i].ext_idx))
            {
               _molecule->getAllowedRGroups(_layout_vertices[i].ext_idx, rgroup_list);
               if (rgroup_list.size() == 1 && rgroup_list[0] == 1)
               {
                  r1_pos = _layout_vertices[i].pos;
                  r1_exist = true;
               } else if (_layout_vertices[i].pos.y > highest_pos.y)
               {
                  highest_pos = _layout_vertices[i].pos;
               }
            }
            center_x += _layout_vertices[i].pos.x;
         }
      
         center_x /= vertexCount();
      
         if (r1_exist)
         {
            if (r1_pos.y < highest_pos.y)
               for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
                  _layout_vertices[i].pos.y *= -1;
            if (r1_pos.x > center_x)
               for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
                  _layout_vertices[i].pos.x *= -1;
         }
      }
      else
      {
         // flip molecule horizontally if the first atom is righter than the last one
         int first = vertexBegin();
         int last = first;
         for (i = first; i != vertexEnd(); i = vertexNext(i))
            last = i;

         const float EPS = 0.0001f;
         float diff = _layout_vertices[first].pos.x - _layout_vertices[last].pos.x;

         if (diff > EPS)
            for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
               _layout_vertices[i].pos.x *= -1;
      }
   }


   // 1. Choose scale ratio and first edge to match
   float scale = bond_length, src_norm, norm;
   int v1, v2, v3;
   Vec2f p1, p2, p;

   _getAnchor(v1, v2, v3);

   p1.diff(src_layout[v2], src_layout[v1]);
   p2.diff(getPos(v2), getPos(v1));

   src_norm = p1.length();
   norm = p2.length();

   if (norm < 0.0001)
      throw Error("too small edge");

   // 2.1. If matching edge has zero length - just move to this point and scale
   if (src_norm < 0.001)
   {
      p1 = src_layout[v1];
      p2 = getPos(v1);

      for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
      {
         p.diff(getPos(i), p2);
         p.scale(scale);
         _layout_vertices[i].pos.sum(p1, p);
      }
      return;
   }

   // 2.2. If it has length from L/2 to 2L - scale by it, otherwise by L
   if (src_norm >= bond_length / 2 && src_norm <= 2 * bond_length)
      scale = src_norm / norm;

   // 3. Move first vertex to (0,0)
   p = getPos(v1);
   for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
      _layout_vertices[i].pos.sub(p);

   // 4. Rotate CCW on Alpha angle between (first, second) edge and (first, second) edge in source graph
   float phi1, phi2, alpha, sina, cosa;

   phi1 = p1.tiltAngle();
   phi2 = p2.tiltAngle();
   alpha = phi1 - phi2;
   sina = sin(alpha);
   cosa = cos(alpha);

   for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
      _layout_vertices[i].pos.rotate(sina, cosa);

   // 5. Scale
   for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
      _layout_vertices[i].pos.scale(scale);

   // 6. Match first vertices - shift by vector Pos(first)
   for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
      _layout_vertices[i].pos.add(src_layout[v1]);

   // 7. If needed turn around (first, second)
   if (vertexCount() > 2)
   {
      float crit = 0.f;
      // If v3 lays on the other side of line (first, second) - turn
      p1 = getPos(v1);
      p.diff(getPos(v2), p1);

      if (v3 == -1)
      {
         for (v3 = vertexBegin(); v3 < vertexEnd(); v3 = vertexNext(v3))
         {	
            if (fabs(p.x) < 0.001f) 
               crit = (src_layout[v3].x - p1.x) * (getPos(v3).x - p1.x);
            else if (fabs(p.y) < 0.001f) 
               crit = (src_layout[v3].y - p1.y) * (getPos(v3).y - p1.y);
            else 
            {
               crit = (p.y * (src_layout[v3].x - p1.x) - p.x * (src_layout[v3].y - p1.y)) *
                  (p.y * (getPos(v3).x - p1.x) - p.x * (getPos(v3).y - p1.y));
            }
            
            if (fabs(crit) > 0.001)
               break;
         }
      } else 
         crit = -1.0;

      if (crit < 0 && v3 < vertexEnd())
      {
         // Move first vertex to (0,0)
         for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
            _layout_vertices[i].pos.sub(p1);

         // Turn by -phi1 and flip vertically
         sina = -sin(phi1);
         cosa =  cos(phi1);
         for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         {
            _layout_vertices[i].pos.rotate(sina, cosa);
            _layout_vertices[i].pos.y *= -1;
         }

         // Turn by phi1 and translate back 
         sina = -sina;
         for (i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         {
            _layout_vertices[i].pos.rotate(sina, cosa);
            _layout_vertices[i].pos.add(p1);
         }
      }
   }
}

void MoleculeLayoutGraph::_findFixedComponents (BiconnectedDecomposer &bc_decom, Array<int> &fixed_components, ObjArray<MoleculeLayoutGraph> & bc_components )
{
   // 1. Find biconnected components forming connected subgraph from fixed vertices
   if (_n_fixed == 0)
      return;

   int n_comp = bc_decom.componentsCount();
   QS_DEF(Array<int>, fixed_count);

   fixed_count.clear_resize(n_comp);
   fixed_count.zerofill();

   // calculate number of fixed vertices in each component
   for (int i = 0; i < n_comp; i++)
   {
      Filter filter;

      bc_decom.getComponent(i, filter);

      for (int j = vertexBegin(); j < vertexEnd(); j = vertexNext(j))
         if (filter.valid(j) && _fixed_vertices[j])
            fixed_count[i]++;
   }

   // keep only with fixed number greater than a half
   for (int i = 0; i < n_comp; i++)
   {
      Filter filter;

      bc_decom.getComponent(i, filter);

      if (fixed_count[i] > filter.count(*this) / 2)
         fixed_components[i] = 1;
   }

   _fixed_vertices.zerofill();

   // update fixed vertices
   for (int i = 0; i < n_comp; i++)
   {
      if (!fixed_components[i])
         continue;

      MoleculeLayoutGraph &component = bc_components[i];

      for (int j = component.vertexBegin(); j < component.vertexEnd(); j = component.vertexNext(j))
         _fixed_vertices[component.getVertexExtIdx(j)] = 1;
   }

   Filter fixed_filter(_fixed_vertices.ptr(), Filter::EQ, 1);

   Graph fixed_graph;
   QS_DEF(Array<int>, fixed_mapping);
   QS_DEF(Array<int>, fixed_inv_mapping);

   fixed_graph.makeSubgraph(*this, fixed_filter, &fixed_mapping, &fixed_inv_mapping);

   if (Graph::isConnected(fixed_graph))
      _n_fixed = fixed_filter.count(*this);
   else
   { 
      // fixed subgraph is not connected - choose its greatest component
      int n = fixed_graph.countComponents();
      const Array<int> &decomposition = fixed_graph.getDecomposition();

      fixed_count.clear_resize(n);
      fixed_count.zerofill();

      for (int i = fixed_graph.vertexBegin(); i < fixed_graph.vertexEnd(); i = fixed_graph.vertexNext(i))
         fixed_count[decomposition[i]]++;

      int j = 0;
      for (int i = 1; i < n; i++)
         if (fixed_count[i] > fixed_count[j])
            j = i;

      Filter max_filter(decomposition.ptr(), Filter::EQ, j);

      // update fixed vertices
      _fixed_vertices.zerofill();
      _n_fixed = 0;

      for (int i = fixed_graph.vertexBegin(); i < fixed_graph.vertexEnd(); i = fixed_graph.vertexNext(i))
      {
         if (max_filter.valid(i))
         {
            _fixed_vertices[fixed_mapping[i]] = 1;
            _n_fixed++;
         }
      }

      for (int i = 0; i < n_comp; i++)
      {
         if (!fixed_components[i])
            continue;

         MoleculeLayoutGraph &component = bc_components[i];

         int comp_v = component.getVertexExtIdx(component.vertexBegin());
         int mapped = fixed_inv_mapping[comp_v];
         if (!max_filter.valid(mapped))
            fixed_components[i] = 0;
      }
   }
}

bool MoleculeLayoutGraph::_assignComponentsRelativeCoordinates (ObjArray<MoleculeLayoutGraph> & bc_components, Array<int> &fixed_components, BiconnectedDecomposer &bc_decom)
{
   bool all_trivial = true;
   int n_comp = bc_decom.componentsCount();

   // Possible solutions:
   // 1. a) vertex code is calculated inside component (doesn't depend on neighbors) or
   //    b) vertex code is calculated respecting whole graph
   // 2. a) component code is the sum of 1a codes
   //    b) component code is the sum of 1b codes
   // Initially was 1a and 2b then changed to 1b and 2b
   for (int i = 0; i < n_comp; i++)
   {
      MoleculeLayoutGraph &component = bc_components[i];
      component.max_iterations = max_iterations;

      //component._calcMorganCodes();
      component._total_morgan_code = 0;

      for (int j = component.vertexBegin(); j < component.vertexEnd(); j = component.vertexNext(j))
         component._total_morgan_code += _layout_vertices[component.getLayoutVertex(j).ext_idx].morgan_code;

      // Mark cyclic atoms
      if (!component.isSingleEdge())
      {
         all_trivial = false;

         for (int j = component.vertexBegin(); j < component.vertexEnd(); j = component.vertexNext(j))
         {
            component._layout_vertices[j].is_cyclic = true;
            _layout_vertices[component._layout_vertices[j].ext_idx].is_cyclic = true;
         }

         for (int j = component.edgeBegin(); j < component.edgeEnd(); j = component.edgeNext(j))
         {
            component._layout_edges[j].is_cyclic = true;
            _layout_edges[component._layout_edges[j].ext_idx].is_cyclic = true;
         }
      }

      int fixed = fixed_components[i];

      component._assignRelativeCoordinates(fixed, *this);

      if (fixed != fixed_components[i])
      {
         fixed_components[i] = fixed;

         // update fixed vertices
         _fixed_vertices.resize(vertexEnd());
         _fixed_vertices.zerofill();
         _n_fixed = 0;
         for (int j = 0; j < n_comp; j++)
         {
            if (!fixed_components[j])
               continue;

            Filter fix_filter;

            bc_decom.getComponent(j, fix_filter);

            for (int k = vertexBegin(); k < vertexEnd(); k = vertexNext(k))
               if (!_fixed_vertices[k])
               {
                  _fixed_vertices[k] = 1;
                  _n_fixed++;
               }
         }
      }
   }
   return all_trivial;
}

void MoleculeLayoutGraph::_assignRelativeSingleEdge (int &fixed_component, const MoleculeLayoutGraph &supergraph) 
{
   // Trivial component layout
   int idx1 = vertexBegin();
   int idx2 = vertexNext(idx1);

   _layout_vertices[idx1].type = ELEMENT_BOUNDARY;
   _layout_vertices[idx2].type = ELEMENT_BOUNDARY;

   if (fixed_component)
   {
      _layout_vertices[idx1].pos = supergraph.getPos(getVertexExtIdx(idx1));
      _layout_vertices[idx2].pos = supergraph.getPos(getVertexExtIdx(idx2));
   }
   else
   {
      _layout_vertices[idx1].pos.set(0.f, 0.f);
      _layout_vertices[idx2].pos.set(0.f, 1.f);
   }

   _layout_edges[edgeBegin()].type = ELEMENT_BOUNDARY;
}

void MoleculeLayoutGraph::_findFirstVertexIdx (int n_comp, Array<int> & fixed_components, ObjArray<MoleculeLayoutGraph> &bc_components, bool all_trivial)
{
   if (_n_fixed > 0)
   {
      int j = -1;
      for (int i = 0; i < n_comp; i++)
         if (fixed_components[i])
         {
            _copyLayout(bc_components[i]);
            j = i;
         }

      if (j == -1)
         throw Error("Internal error: cannot find a fixed component with fixed vertices");

      MoleculeLayoutGraph &component = bc_components[j];

      _first_vertex_idx = component._layout_vertices[component.vertexBegin()].ext_idx;
   }
   else
   {
      // ( 0]. Nucleus.;
      //   Begin from nontrivial component with maximum code
      //   if there's no then begin from vertex with maximum code and its neighbor with maximum code too
      int nucleus_idx = 0;

      if (!all_trivial)
      {
         nucleus_idx = -1;
         for (int i = 0; i < n_comp; i++)
         {
            MoleculeLayoutGraph &component = bc_components[i];

            if (!component.isSingleEdge())
            {
               if (nucleus_idx == -1 || component._total_morgan_code > bc_components[nucleus_idx]._total_morgan_code)
                  nucleus_idx = i;
            }
         }

         if (nucleus_idx < 0)
            throw Error("Internal error: cannot find nontrivial component");

         MoleculeLayoutGraph &nucleus = bc_components[nucleus_idx];

         _copyLayout(nucleus);
         _first_vertex_idx = nucleus._layout_vertices[nucleus._first_vertex_idx].ext_idx;
      } else 
      {
         for (int i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
            if (_layout_vertices[i].morgan_code > _layout_vertices[nucleus_idx].morgan_code)
               nucleus_idx = i;

         const Vertex &nucleus = getVertex(nucleus_idx);
         int nucleus_idx2 = nucleus.neiBegin();

         for (int j = nucleus.neiNext(nucleus_idx2); j < nucleus.neiEnd(); j = nucleus.neiNext(j))
            if (_layout_vertices[nucleus.neiVertex(j)].morgan_code > _layout_vertices[nucleus.neiVertex(nucleus_idx2)].morgan_code)
               nucleus_idx2 = j;

         int nucleus_edge = nucleus.neiEdge(nucleus_idx2);
         nucleus_idx2 = nucleus.neiVertex(nucleus_idx2);

         _first_vertex_idx = nucleus_idx;
         _layout_vertices[nucleus_idx].type = ELEMENT_BOUNDARY;
         _layout_vertices[nucleus_idx].pos.set(0.f, 0.f);
         _layout_vertices[nucleus_idx2].type = ELEMENT_BOUNDARY;
         _layout_vertices[nucleus_idx2].pos.set(1.f, 0.f);
         _layout_edges[nucleus_edge].type = ELEMENT_BOUNDARY;
      }
   }
}

bool MoleculeLayoutGraph::_prepareAssignedList (Array<int> &assigned_list, BiconnectedDecomposer &bc_decom, ObjArray<MoleculeLayoutGraph> &bc_components, Array<int> &bc_tree)
{
   assigned_list.clear();

   for (int i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
   {
      if (_layout_vertices[i].type == ELEMENT_NOT_DRAWN)
         continue;

      const Vertex &vert = getVertex(i);

      for (int j = vert.neiBegin(); j < vert.neiEnd(); j = vert.neiNext(j))
      {
         if (_layout_vertices[vert.neiVertex(j)].type == ELEMENT_NOT_DRAWN)
         {
            assigned_list.push(i);
            break;
         }
      }
   }

   if (assigned_list.size() == 0)
   {
      // restore ignored ears in chains
      for (int i = vertexBegin(); i < vertexEnd(); i = vertexNext(i))
         if (_layout_vertices[i].type == ELEMENT_IGNORE)
            _layout_vertices[i].type = ELEMENT_BOUNDARY;

      _refineCoordinates(bc_decom, bc_components, bc_tree);
      return false;
   }

   // ( 2] the list is ordered with cyclic atoms at the top of the list;
   //   with descending ATCD numbers and acyclic atoms at the bottom;
   //   of the list with descending ATCD numbers;;
   assigned_list.qsort(_vertex_cmp, this);
   return true;
}
