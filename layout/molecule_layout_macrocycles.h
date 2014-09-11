/****************************************************************************
 * Copyright (C) 2009-2011 GGA Software Services LLC
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

#ifndef __molecule_layout_macrocycles_h__
#define __molecule_layout_macrocycles_h__

#include "molecule/molecule.h"
#include "layout/molecule_layout_graph.h"
#include <algorithm>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4251)
#endif

using namespace std;
namespace indigo {

class DLLEXPORT MoleculeLayoutMacrocycles
{
public:
   CP_DECL;
   MoleculeLayoutMacrocycles (int size);

   void addVertexOutsideWeight (int v, int weight);
   void setVertexEdgeParallel (int v, bool parallel);
   void set_vertex_added_square(int v, double s);
   void setEdgeStereo (int e, int stereo);
   void setVertexDrawn(int v, bool drawn);
   void set_component_finish(int v, int f);
   void set_target_angle(int v, double angle);
   void set_angle_importance(int, double);

   int getVertexStereo (int v);

   Vec2f &getPos (int v) const;

   void doLayout ();

// private:
public:
   static bool canApply (BaseMolecule &mol);

   double layout (BaseMolecule &mol);

   void smoothing(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p, bool profi);
   void smoothing2(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p);
   void improvement2(int i, int vertex_count, int cycle_size, int *rotate_angle, int *edge_lenght, int *vertex_number, Vec2f *p, int base_vertex, bool fix_angle, bool fix_next, double multiplyer);
   double badness(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p, int diff);
   double depictionMacrocycleGreed(bool profi);
   double depictionCircle();

   DECL_ERROR;

private:
   int length;
   static const int max_size;
   static const int init_x;
   static const int init_y;
   static const int init_rot;
   static const double CHANGE_FACTOR;
   static const int WEIGHT_FACTOR = 12;


   int get_diff_grid(int x, int y, int rot, int value);
   int get_diff_circle(int x, int y, int rot, int value);

   struct Data
   {
      enum { max_size = 105 };
      unsigned short minRotates[max_size][max_size][2][max_size][max_size];
   };

   TL_CP_DECL(Data, data);
   TL_CP_DECL(Array<int>, _vertex_weight);
   TL_CP_DECL(Array<int>, _vertex_stereo);
   TL_CP_DECL(Array<double>, _vertex_added_square);
   TL_CP_DECL(Array<int>, _edge_stereo);
   TL_CP_DECL(Array<bool>, _vertex_drawn);
   TL_CP_DECL(Array<Vec2f>, _positions);
   TL_CP_DECL(Array<int>, _component_finish);
   TL_CP_DECL(Array<double>, _target_angle);
   TL_CP_DECL(Array<double>, _angle_importance);

};

class DLLEXPORT MoleculeLayoutMacrocyclesLattice
{
public:
   CP_DECL;

   MoleculeLayoutMacrocyclesLattice(int size, int* _v_w, int* _v_s, int* _e_s);

   void addVertexOutsideWeight(int v, int weight);
   void setVertexEdgeParallel(int v, bool parallel);
   void setEdgeStereo(int e, int stereo);

   DECL_ERROR;

private:

   int length;
   int rotate_length;
   static const int INFINITY = 60000;

   void calculate_rotate_length();
   void rotate_cycle(int shift);

   TL_CP_DECL(Array<int>, _vertex_weight);
   TL_CP_DECL(Array<int>, _vertex_stereo);
   TL_CP_DECL(Array<int>, _edge_stereo);
};

struct rectangle {
   int min_x;
   int max_x;
   int min_y;
   int max_y;

   bool empty;

   rectangle() {
      set_empty();
   }

   rectangle(int x1, int x2, int y1, int y2) {
      set(x1, x2, y1, y2);
   }

   void set(int x1, int x2, int y1, int y2) {
      min_x = x1;
      max_x = x2;
      min_y = y1;
      max_y = y2;

      empty = (min_x > max_x) || (min_y > max_y);
   }

   void intersec(int x1, int x2, int y1, int y2) {
      set(max(min_x, x1), min(max_x, x2), max(min_y, y1), min(max_y, y2));
   }

   void set(rectangle rec) {
      set(rec.min_x, rec.max_x, rec.min_y, rec.max_y);
   }

   void intersec(rectangle rec) {
      intersec(rec.min_x, rec.max_x, rec.min_y, rec.max_y);
   }

   rectangle expand(int w) {
      if (empty) return rectangle(1, 0, 1, 0);
      else return rectangle(min_x - w, max_x + w, min_y - w, max_y + w);
   }

   rectangle expand() {
      return expand(1);
   }

   void set_empty() {
      empty = true;
   }

   rectangle shift(int x, int y) {
      return rectangle(min_x + x, max_x + x, min_y + y, max_y + y);
   }

   bool contains(int x, int y) {
      return !empty && x >= min_x && x <= max_x && y >= min_y && y <= max_y;
   }
};



class DLLEXPORT TriangleLattice
{
public:
   
   TriangleLattice();

   TriangleLattice(rectangle rec, int rem, byte* data_link);

   void init(rectangle rec, int rem, byte* data_link);

   unsigned short& getCell(int x, int y);
   int getFirstValidY(int x);
   bool isIncreaseForValidY(int);

   static int getAllocationSize(rectangle rec) {
      if (rec.empty) return 0;
      int sq = (((rec.max_x - rec.min_x + 1) * (rec.max_y - rec.min_y + 1) + 2) / 3) * sizeof(unsigned short) + (rec.max_x - rec.min_x + 1) * sizeof(unsigned short*);
      return sq;
   };

   DECL_ERROR;

private:

   int _difference_reminder;
   unsigned short** _starts;

   unsigned short _sink;

   rectangle _BORDER;

};

class DLLEXPORT AnswerField
{
public:
   CP_DECL;
   AnswerField(int len, int target_x, int target_y, double target_rotation, int* vertex_weight_link, int* vertex_stereo_link, int* edge_stereo_link);

   void fill();

   DECL_ERROR;

private:

   const int ACCEPTABLE_ERROR = 10;

   int length;

   byte* _hidden_data_field;
   ObjArray<Array<rectangle>> border_array;
   Array<rectangle*> border;

   static const int dx[6];
   static const int dy[6];

   TL_CP_DECL(Array<int>, _vertex_weight);
   TL_CP_DECL(Array<int>, _vertex_stereo);
   TL_CP_DECL(Array<int>, _edge_stereo);
   TL_CP_DECL(Array<int>, _rotation_parity);
   TL_CP_DECL(Array<int>, _coord_diff_reminder); // (y - x) % 3
   TL_CP_DECL(Array<TriangleLattice>, _field);
   TL_CP_DECL(ObjArray<ObjArray<ObjArray<TriangleLattice>>>, _lattices);

   TriangleLattice _sink_lattice;

   TriangleLattice& get_lattice(int l, int rot, int p);
};

}



#ifdef _WIN32
#pragma warning(pop)
#endif

#endif 