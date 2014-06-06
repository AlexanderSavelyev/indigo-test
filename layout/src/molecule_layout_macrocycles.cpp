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

#include "layout/molecule_layout_macrocycles.h"

#include "base_cpp/profiling.h"
#include <limits.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <cmath>
#include <string>
#include <sstream>
#include <map>
#include <stdio.h>
#include <math/random.h>
#include "layout/molecule_layout.h"
#include <algorithm>

using namespace std;
using namespace indigo;

IMPL_ERROR(MoleculeLayoutMacrocycles, "molecule_layout_macrocycles");

const int MoleculeLayoutMacrocycles::max_size = MoleculeLayoutMacrocycles::Data::max_size;
const int MoleculeLayoutMacrocycles::init_x = MoleculeLayoutMacrocycles::max_size / 2;
const int MoleculeLayoutMacrocycles::init_y = MoleculeLayoutMacrocycles::max_size / 2;
const int MoleculeLayoutMacrocycles::init_rot = MoleculeLayoutMacrocycles::max_size / 2 / 6 * 6;

CP_DEF(MoleculeLayoutMacrocycles);

MoleculeLayoutMacrocycles::MoleculeLayoutMacrocycles (int size) : 
   CP_INIT,
   TL_CP_GET(data), // tree size
   TL_CP_GET(_vertex_weight), // tree size
   TL_CP_GET(_vertex_stereo), // there is an angle in the vertex
   TL_CP_GET(_edge_stereo), // trans-cis configuration
   TL_CP_GET(_vertex_drawn), // is each vertex has been drawn earlier
   TL_CP_GET(_positions), // position of vertex
   TL_CP_GET(_vertex_added_square), // square of attached layout component
   TL_CP_GET(_component_finish), // finish of component starting with this vertex
   TL_CP_GET(_target_angle), // prefer angle in this vertex
   TL_CP_GET(_angle_importance) // importance of angle in this vertiex is close to target
{
   // Set default values...
   length = size;

   _vertex_weight.clear_resize(size);
   _vertex_weight.fill(0);

   _vertex_stereo.clear_resize(size);
   _vertex_stereo.zerofill();

   _edge_stereo.clear_resize(size);
   _edge_stereo.zerofill();

   _vertex_drawn.clear_resize(size);
   _vertex_drawn.zerofill();

   _positions.clear_resize(size);

   _vertex_added_square.clear_resize(size);
   _vertex_added_square.zerofill();

   _component_finish.clear_resize(size);
   for (int i = 0; i < size; i++) _component_finish[i] = i;

   _target_angle.clear_resize(size);
   _target_angle.zerofill();

   _angle_importance.clear_resize(size);
   _angle_importance.fill(1);

}

void MoleculeLayoutMacrocycles::addVertexOutsideWeight (int v, int weight)
{
   _vertex_weight[v] += weight;
}

void MoleculeLayoutMacrocycles::setVertexEdgeParallel (int v, bool parallel) 
{
   _vertex_stereo[v] = !parallel;
}

void MoleculeLayoutMacrocycles::set_vertex_added_square(int v, double s) {
   _vertex_added_square[v] = s;
}

int MoleculeLayoutMacrocycles::getVertexStereo (int v) 
{
   return _vertex_stereo[v];
}

void MoleculeLayoutMacrocycles::setEdgeStereo (int e, int stereo)
{
   _edge_stereo[e] = stereo;
}

void MoleculeLayoutMacrocycles::setVertexDrawn(int v, bool drawn) {
   _vertex_drawn[v] = drawn;
}

Vec2f &MoleculeLayoutMacrocycles::getPos (int v) const
{
   return _positions[v];
}

void MoleculeLayoutMacrocycles::set_component_finish(int v, int f) {
   _component_finish[v] = f;
}

void MoleculeLayoutMacrocycles::set_target_angle(int v, double angle) {
   _target_angle[v] = angle;
}

void MoleculeLayoutMacrocycles::set_angle_importance(int v, double imp) {
   _angle_importance[v] = imp;
}


void MoleculeLayoutMacrocycles::doLayout ()
{
   profTimerStart(t, "bc.layout");

   double b2 = depictionCircle();
   double b = depictionMacrocycleGreed(false);

   if (b > b2) {
      depictionCircle();
   }
}

bool MoleculeLayoutMacrocycles::canApply (BaseMolecule &mol)
{
   if (!mol.isConnected(mol)) {
      return false;
   }

   for (int v = mol.vertexBegin(); v != mol.vertexEnd(); v = mol.vertexNext(v)) {
      if (mol.getVertex(v).degree() != 2) {
         return false;
      }
   }

   return true;
}

double sqr(double x) {return x*x;}

int improvement(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p, bool profi, bool do_dist, double multiplier, int worstVertex) {
   
   Vec2f p1 = p[(worstVertex - 1 + ind) % ind];
   Vec2f p2 = p[(worstVertex + 1 + ind) % ind];
   double r1 = edgeLenght[(ind + worstVertex - 1) % ind];
   double r2 = edgeLenght[(ind + worstVertex) % ind];

   double len1 = Vec2f::dist(p1, p[worstVertex]);
   double len2 = Vec2f::dist(p2, p[worstVertex]);

   double r3 = Vec2f::dist(p1, p2)/sqrt(3.0);
      
   Vec2f p3 = (p1 + p2)/2;

   if (rotateAngle[worstVertex] != 0) {
      Vec2f a = (p2 - p1)/sqrt(12.0f);
      a.rotate(PI/2 * rotateAngle[worstVertex]);
      p3 += a;
   } else {
      p3 = (p1*r1 + p2*r2)/(r1 + r2);
   }


   double len3 = Vec2f::dist(p3, p[worstVertex]);
   if (rotateAngle[worstVertex] == 0) r3 = 0;

   //printf("%5.5f %5.5f %5.5f %5.5f\n", len1, len2, len3, r3);
   Vec2f newPoint;
   double eps = 1e-4;
   double eps2 = eps * eps;
   if (len1 < eps || len2 < eps || len3 < eps) {
      p[worstVertex] = (p1 + p2)/2.0;
   } else {
      double coef1 = (r1/len1 - 1);
      double coef2 = (r2/len2 - 1);
      double coef3 = (r3/len3 - 1);
      if (rotateAngle[worstVertex] != 0) {
         double angle = acos(Vec2f::cross(p1 - p[worstVertex], p2 - p[worstVertex])/(Vec2f::dist(p1, p[worstVertex])*Vec2f::dist(p2, p[worstVertex])));
         //if (angle < 2 * PI / 3) coef3 /= 10;
      }

      //if (!isIntersec(x[worstVertex], y[worstVertex], x3, y3, x1, y1, x2, y2)) coef3 *= 10;
      if (rotateAngle[worstVertex] == 0) coef3 = -1;
      //printf("%5.5f %5.5f %5.5f\n", coef1, coef2, coef3);
      newPoint += (p[worstVertex] - p1)*coef1;
      newPoint += (p[worstVertex] - p2)*coef2;
      newPoint += (p[worstVertex] - p3)*coef3;

      if (do_dist) {
         if (profi) {
            for (int i = 0; i < ind; i++) if (i != worstVertex && (i + 1) % ind != worstVertex) {
               double dist = Vec2f::distPointSegment(p[worstVertex], p[i], p[(i + 1) % ind]);
               if (dist < 1 && dist > eps) {
                  Vec2f normal = (p[(i + 1) % ind] - p[i]);
                  normal.rotate(PI / 2);
                  double c = Vec2f::cross(p[i], p[(i + 1) % ind]);
                  double s = normal.length();

                  normal /= s;
                  c /= s;

                  double t = -c - Vec2f::dot(p[worstVertex], normal);

                  Vec2f pp;

                  if (s < eps) {
                     pp = p[i];
                  }
                  else {
                     pp = p[worstVertex] + normal * t;
                     if (Vec2f::dist(p[worstVertex], p[i]) < Vec2f::dist(p[worstVertex], pp)) {
                        pp = p[i];
                     }
                     if (Vec2f::dist(p[worstVertex], p[(i + 1) % ind]) < Vec2f::dist(p[worstVertex], pp)) {
                        pp = p[(i + 1) % ind];
                     }
                  }

                  double coef = (1 - dist) / dist;

                  newPoint += (p[worstVertex] - pp) * coef;
               }
            }
         }
         else {
            float good_distance = 1;
            for (int j = 0; j < ind; j++) {
               int nextj = (j + 1) % ind;
               Vec2f pp = p[j];
               Vec2f dpp = (p[nextj] - p[j]) / edgeLenght[j];

               for (int t = vertexNumber[j], s = 0; t != vertexNumber[nextj]; t = (t + 1) % molSize, s++) {
                  if (t != vertexNumber[worstVertex] && (t + 1) % molSize != vertexNumber[worstVertex] && t != (vertexNumber[worstVertex] + 1) % molSize) {
                     double distSqr = Vec2f::distSqr(pp, p[worstVertex]);
                     if (distSqr < good_distance && distSqr > eps2) {
                        double dist = sqrt(distSqr);
                        double coef = (good_distance - dist) / dist;
                        //printf("%5.5f \n", dist);
                        newPoint += (p[worstVertex] - pp)*coef;
                     }
                  }
                  pp += dpp;
               }
            }
         }
      }

      newPoint *= multiplier;

      p[worstVertex] += newPoint;
   }
   return worstVertex;
}

void MoleculeLayoutMacrocycles::smoothing(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p, bool profi) {

   Random rand(931170240);

   int iter_count = max(50 * molSize, 2000);

   for (int i = 0; i < iter_count; i++) improvement(ind, molSize, rotateAngle, edgeLenght, vertexNumber, p, profi, i >= iter_count/2, 0.1, rand.next(ind));
}

double MoleculeLayoutMacrocycles::badness(int ind, int molSize, int *rotateAngle, int *edgeLenght, int *vertexNumber, Vec2f *p, int diff) {
   double eps = 1e-9;
   double result = 0;
   int add = 0;
   // distances
   for (int i = 0; i < ind; i++) {
      double len = Vec2f::dist(p[i], p[(i + 1)%ind])/edgeLenght[i];
      if (len < eps) add++;
      else if (len < 1) result = max(result, (1/len - 1));
      else result = max(result, len - 1);
   }
   // angles
   for (int i = 0; i < ind; i++) {
      Vec2f vp1 = p[(i + 1) % ind] - p[i];
      Vec2f vp2 = p[(i + ind - 1) % ind] - p[i];
      double len1 = vp1.length();
      double len2 = vp2.length();
      vp1 /= len1;
      vp2 /= len2;

      double angle = acos(Vec2f::dot(vp1, vp2));
      if (Vec2f::cross(vp2, vp1) > 0) angle = -angle;
      angle /= _target_angle[vertexNumber[i]];
      if (angle * rotateAngle[i] <= 0) add += 1000;
      double angle_badness = abs((((abs(angle) > 1) ? angle : 1 / angle) - rotateAngle[i]) / 2) * _angle_importance[vertexNumber[i]];
      result = max(result, angle_badness);
   }


   vector<Vec2f> pp;
   for (int i = 0; i < ind; i++)
      for (int a = vertexNumber[i], t = 0; a != vertexNumber[(i + 1) % ind]; a = (a + 1) % molSize, t++) {
         pp.push_back((p[i] * (edgeLenght[i] - t) + p[(i + 1)%ind] * t)/edgeLenght[i]);
      }

   int size = pp.size();
   for (int i = 0; i < size; i++)
      for (int j = 0; j < size; j++) if (i != j && (i + 1) % size != j && i != (j + 1) % size) {
         int nexti = (i + 1) % size;
         int nextj = (j + 1) % size;
         double dist = Vec2f::distSegmentSegment(pp[i], pp[nexti], pp[j], pp[nextj]);

         if (abs(dist) < eps) {
            add++;
            //printf("%5.5f %5.5f %5.5f %5.5f %5.5f %5.5f %5.5f %5.5f \n", xx[i], yy[i], xx[nexti], yy[nexti], xx[j], yy[j], xx[nextj], yy[nextj]);
         }
         else if (dist < 1) result = max(result, 1/dist - 1);
      }
   // tails
   
   if (diff == 30000) {
      diff = 0;
      for (int i = 0; i < ind; i++) diff += (rotateAngle[i] == -1) * _vertex_weight[vertexNumber[i]];
   }
   else {
      for (int i = 0; i < ind; i++) diff -= (rotateAngle[i] != 0) && (rotateAngle[i] == rotateAngle[(i + 1) % ind]);
   }
   result += 1.0 * diff / molSize;

/*   double square = 0;
   for (int i = 1; i < ind - 1; i++)
      square += Vec2f::dot(p[i] - p[0], p[(i + 1) % ind] - p[0]);

   square = abs(square);

   for (int i = 0; i < ind; i++)
   if (_component_finish[vertexNumber[i]] != vertexNumber[i] && rotateAngle[i] == -1)
      square += _vertex_added_square[vertexNumber[i]] * (p[_component_finish[vertexNumber[i]]] - p[i]).lengthSqr();

   result -= square / molSize / molSize;*/

   //printf("%5.5f\n", result);
   return result + 1000.0 * add;

}

int MoleculeLayoutMacrocycles::get_diff_grid(int x, int y, int rot, int value) {
   int diffCoord;

   x -= init_x;
   y -= init_y;
   if (x * y >= 0) diffCoord = abs(x) + abs(y); // x and y both positive or negative, vector (y+x) is not neseccary
   else diffCoord = max(abs(x), abs(y)); // x and y are has got different signs, vector (y-x) is neseccary
   int diffRot = abs(abs(rot - (init_rot + 6)) - 1);

   return 2 * diffRot + 1 * diffCoord + value;
}

int MoleculeLayoutMacrocycles::get_diff_circle(int x, int y, int rot, int value) {
   int diffCoord;

   y -= init_y;
   if (x * y >= 0) diffCoord = abs(x) + abs(y); // x and y both positive or negative, vector (y+x) is not neseccary
   else diffCoord = max(abs(x), abs(y)); // x and y are has got different signs, vector (y-x) is neseccary
   int diffRot = abs(rot - (init_rot + 1));

   return 2 * diffRot + 1 * diffCoord + value;
}

void rotate(int* ar, int ar_length, int shift) {
   QS_DEF(Array<int>, temp);
   temp.clear_resize(ar_length);
   shift = (shift % ar_length + ar_length) % ar_length;
   memcpy(temp.ptr(), ar + shift, (ar_length - shift) * sizeof(int));
   memcpy(temp.ptr() + ar_length - shift, ar, shift * sizeof(int));
   memcpy(ar, temp.ptr(), ar_length * sizeof(int));
}

void rotate(double* ar, int ar_length, int shift) {
   QS_DEF(Array<double>, temp);
   temp.clear_resize(ar_length);
   shift = (shift % ar_length + ar_length) % ar_length;
   memcpy(temp.ptr(), ar + shift, (ar_length - shift) * sizeof(double));
   memcpy(temp.ptr() + ar_length - shift, ar, shift * sizeof(double));
   memcpy(ar, temp.ptr(), ar_length * sizeof(double));
}

double MoleculeLayoutMacrocycles::depictionMacrocycleGreed(bool profi)
{

   if (length >= max_size) return 1e9;
   signed short (&minRotates)[max_size][max_size][2][max_size][max_size] = data.minRotates;
   //first : number of edge
   //second : summary angle of rotation (in PI/3 times)
   //third : last rotation is contraclockwise
   //fourth : x-coordinate
   //fifth : y-coordinate
   //value : minimum number of vertexes sticked out + count of CIS-configurations

   int infinity = 30000;

   int shift = -1;
   int max_value = -infinity;

   for (int i = 0; i < length; i++) {
      if (_edge_stereo[i] != 2) {
         int value = _edge_stereo[i]
            + _vertex_weight[i] + _vertex_weight[(i + 1) % length]
            - _vertex_weight[(i + length - 1) % length]/2 - _vertex_weight[(i + 2) % length]/2;

         if (shift == -1 || value > max_value) {
            shift = i;
            max_value = value;
         }
      }
   }

   if (shift == -1) return 1e9;

   for (int i = 0; i < length; i++) 
      if (_vertex_weight[i] > 0) _vertex_weight[i]++;
   
   rotate(_vertex_weight.ptr(), length, shift);
   rotate(_vertex_stereo.ptr(), length, shift);
   rotate(_edge_stereo.ptr(), length, shift);
   rotate(_angle_importance.ptr(), length, shift);

   int dx[6];
   int dy[6];
   dx[0] = 1; dy[0] = 0;
   dx[1] = 0; dy[1] = 1;
   dx[2] = -1; dy[2] = 1;
   dx[3] = -1; dy[3] = 0;
   dx[4] = 0; dy[4] = -1;
   dx[5] = 1; dy[5] = -1;


   int x_left = max(init_x - length, 1);
   int x_right = min(init_x + length, max_size - 2);
   int y_left = max(init_y - length, 1);
   int y_right = min(init_y + length, max_size - 2);
   int rot_left = max(init_rot - length, 1);
   int rot_right = min(init_rot + length, max_size - 2);

   for (int i = 0; i <= length; i++)
      for (int j = rot_left - 1; j <= rot_right + 1; j++)
         for (int p = 0; p < 2; p++) 
            for (int k = x_left - 1; k <= x_right + 1; k++)
               for (int t = y_left - 1; t <= y_right + 1; t++)
                  minRotates[i][j][p][k][t] = infinity;

   minRotates[0][init_rot][1][init_x][init_y] = 0;
   minRotates[1][init_rot][1][init_x + 1][init_y] = 0;
   int max_dist = length;

   for (int k = 1; k < length; k++) {
      //printf("Step number %d\n", k);

      int not_this_p = -1;
      if (k == length - 1) {
         if (_edge_stereo[length - 1] == MoleculeCisTrans::CIS) not_this_p = 0;
         if (_edge_stereo[length - 1] == MoleculeCisTrans::TRANS) not_this_p = 1;
      }
      for (int rot = rot_left; rot <= rot_right; rot++) {
         if (!_vertex_stereo[k]) {
            int xchenge = dx[rot % 6];
            int ychenge = dy[rot % 6];
            for (int p = 0; p < 2; p++) if (p != not_this_p) {
               for (int x = x_left; x <= x_right; x++) {
                  signed short *ar1 = minRotates[k + 1][rot][p][x + xchenge] + ychenge;
                  signed short *ar2 = minRotates[k][rot][p][x];
                  for (int y = y_left; y <= y_right; y++) {
                     if (ar1[y] > ar2[y]) {
                        ar1[y] = ar2[y];
                     }
                  }
               }
            }
         } else {
            for (int p = 0; p < 2; p++) {
               // trying to rotate like CIS
               if (_edge_stereo[k - 1] != MoleculeCisTrans::TRANS) if (p != not_this_p) {
                  int nextRot = rot;
                  if (p) nextRot++;
                  else nextRot--;

                  int xchenge = dx[nextRot % 6];
                  int ychenge = dy[nextRot % 6];

                  int add = 1;
                  if (!p && _vertex_weight[k] > 2) add += _vertex_weight[k];

                  for (int x = x_left; x <= x_right; x++) {
                     signed short *ar1 = minRotates[k + 1][nextRot][p][x + xchenge] + ychenge;
                     signed short *ar2 = minRotates[k][rot][p][x];
                     for (int y = y_left; y <= y_right; y++) {
                        if (ar1[y] > ar2[y] + add) {
                           ar1[y] = ar2[y] + add;
                        }
                     }
                  }
               }
               // trying to rotate like TRANS
               if (_edge_stereo[k - 1] != MoleculeCisTrans::CIS) if ((p ^ 1) != not_this_p) {
                  int nextRot = rot;
                  if (p) nextRot--;
                  else nextRot++;

                  int add = 0;
                  if (p && _vertex_weight[k] > 2) add = _vertex_weight[k];

                  int xchenge = dx[nextRot % 6];
                  int ychenge = dy[nextRot % 6];

                  for (int x = x_left; x <= x_right; x++) {
                     signed short *ar1 = minRotates[k + 1][nextRot][p ^ 1][x + xchenge] + ychenge;
                     signed short *ar2 = minRotates[k][rot][p][x];
                     for (int y = y_left; y <= y_right; y++) {
                        if (ar1[y] > ar2[y] + add) {
                           ar1[y] = ar2[y] + add;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for (int rot = rot_left; rot <= rot_right; rot++)
      for (int x = x_left; x <= x_right; x++)
         for (int y = y_left; y <= y_right; y++)
            minRotates[length][rot][1][x][y]++;

   struct point {
      int diff;
      int x;
      int y;
      int p;
      int rot;
      int type;

      point(int _d, int _x, int _y, int _p, int _r, int _t) {
         diff = _d;
         x = _x;
         y = _y;
         p = _p;
         rot = _r;
         type = _t;
      }

      bool operator<(const point& p) const { return diff - p.diff; }
   };

   QS_DEF(ObjArray<point>, points);
   points.clear();

   int critical_diff_grid = infinity - 1;
   std::multiset<int> diff_set;

   int var_count = 100;

   enum {
      CIRCLE_TYPE,
      GRID_TYPE
   };

   for (int rot = init_rot; rot <= rot_right; rot++)
      for (int p = 0; p < 2; p++)
         for (int x = x_left; x <= x_right; x++)
            for (int y = y_left; y <= y_right; y++)
            if (minRotates[length][rot][p][x][y] <= infinity) {
               int curr_dif = get_diff_grid(x, y, rot, minRotates[length][rot][p][x][y]);
               if (curr_dif <= critical_diff_grid) {
                  diff_set.insert(curr_dif);
                  if (diff_set.size() > var_count) diff_set.erase(*diff_set.rbegin());
                  critical_diff_grid = *diff_set.rbegin();
               }
            }
   

   
   for (int rot = init_rot; rot <= rot_right; rot++) {
      for (int p = 0; p < 2; p++) {
         for (int x = x_left; x <= x_right; x++) {
            for (int y = y_left; y <= y_right; y++) {
               if (minRotates[length][rot][p][x][y] < infinity) {

                  int curdiff = get_diff_grid(x, y, rot, minRotates[length][rot][p][x][y]);

                  if (curdiff <= critical_diff_grid) {
                     point curr_point(curdiff, x, y, p, rot, GRID_TYPE);
                     points.push(curr_point);
                  }
               }
            }
         }
      }
   }

   diff_set.clear();
   int critical_diff_circle = infinity - 1;

   for (int rot = rot_left; rot <= rot_right; rot++)
      for (int p = 0; p < 2; p++)
         for (int x = x_left; x <= x_right; x++)
            for (int y = y_left; y <= y_right; y++)
               if (minRotates[length][rot][p][x][y] <= infinity) {
                  int curr_dif = get_diff_circle(x - init_x - length/2, y - length/2, rot, minRotates[length][rot][p][x][y]);
                  if (curr_dif <= critical_diff_circle) {
                     diff_set.insert(curr_dif);
                     if (diff_set.size() > var_count) diff_set.erase(*diff_set.rbegin());
                     critical_diff_circle = *diff_set.rbegin();
                  }
               }



   for (int rot = rot_left; rot <= rot_right; rot++) {
      for (int p = 0; p < 2; p++) {
         for (int x = x_left; x <= x_right; x++) {
            for (int y = y_left; y <= y_right; y++) {
               if (minRotates[length][rot][p][x][y] < infinity) {

                  int curdiff = get_diff_circle(x - init_x - length / 2, y - length/2, rot, minRotates[length][rot][p][x][y]);

                  if (curdiff <= critical_diff_circle) {
                     point curr_point(curdiff, x, y, p, rot, CIRCLE_TYPE);
                     points.push(curr_point);
                  }
               }
            }
         }
      }
   }

   for (int rot = rot_left; rot <= rot_right; rot++)
      for (int x = x_left; x <= x_right; x++)
         for (int y = y_left; y <= y_right; y++)
            minRotates[length][rot][1][x][y]--;



   int x_result[max_size + 1];
   int y_result[max_size + 1];
   int rot_result[max_size + 1];
   int p_result[max_size + 1];

   double bestBadness = 1e30;
   int bestIndex = 0;

   for (int index = 0; index < points.size(); index++) {
      
      x_result[length] = points[index].x;
      y_result[length] = points[index].y;
      rot_result[length] = points[index].rot;
      p_result[length] = points[index].p;

      for (int k = length - 1; k > 0; k--) {
         int xchenge = dx[rot_result[k + 1] % 6];
         int ychenge = dy[rot_result[k + 1] % 6];
         x_result[k] = x_result[k + 1] - xchenge;
         y_result[k] = y_result[k + 1] - ychenge;

         if (!_vertex_stereo[k]) {
            p_result[k] = p_result[k + 1];
            rot_result[k] = rot_result[k + 1];
         } else {
            if (p_result[k + 1]) rot_result[k] = rot_result[k + 1] - 1;
            else rot_result[k] = rot_result[k + 1] + 1;

            int add = 0;
            if (!p_result[k + 1] && _vertex_weight[k] > 2) add = _vertex_weight[k];


            double l = k * (sqrt(3.0) + 1.5) * PI / 12;
            Vec2f vec(y_result[k] - init_y, 0);
            vec.rotate(PI/3);
            vec += Vec2f(x_result[k] - init_x, 0);
            double x = vec.length();

            double eps = 1e-3;

            double alpha = 0;
            if (x > eps) {

               double L = eps;
               double R = 2*PI - eps;

               while (R - L > eps) {
                  double M = (L + R)/2;
                  if (M * x/ (2 * sin(M/2)) > l) R = M;
                  else L = M;
               }

               alpha = vec.tiltAngle2() + R/2;

            }
            
            p_result[k] = 2;
            int is_cis_better = (alpha < PI/3 * (rot_result[k] - init_rot) + PI/length) ^ (!p_result[k + 1]);

            if (!is_cis_better) {
               if (_edge_stereo[k - 1] != MoleculeCisTrans::TRANS) {
                  // try CIS
                  if (minRotates[k][rot_result[k]][p_result[k + 1]][x_result[k]][y_result[k]] + add + 1 == minRotates[k + 1][rot_result[k + 1]][p_result[k + 1]][x_result[k + 1]][y_result[k + 1]]) {
                     p_result[k] = p_result[k + 1];
                  }
               }
            }
            if (_edge_stereo[k - 1] != MoleculeCisTrans::CIS) {
                  // try TRANS
               if (minRotates[k][rot_result[k]][p_result[k + 1] ^ 1][x_result[k]][y_result[k]] + add == minRotates[k + 1][rot_result[k + 1]][p_result[k + 1]][x_result[k + 1]][y_result[k + 1]]) {
                  p_result[k] = p_result[k + 1] ^ 1;
               }
            }
            if (is_cis_better) {
               if (_edge_stereo[k - 1] != MoleculeCisTrans::TRANS) {
                  // try CIS
                  if (minRotates[k][rot_result[k]][p_result[k + 1]][x_result[k]][y_result[k]] + add + 1 == minRotates[k + 1][rot_result[k + 1]][p_result[k + 1]][x_result[k + 1]][y_result[k + 1]]) {
                     p_result[k] = p_result[k + 1];
                  }
               }
            }

            if (p_result[k] == 2) {
               throw Error("Path not find (%d): %d.", length, minRotates[k + 1][rot_result[k + 1]][p_result[k + 1]][x_result[k + 1]][y_result[k + 1]]);
            }
         }
      }
      x_result[0] = init_x;
      y_result[0] = init_y;

      int rotateAngle[max_size];
      int edgeLenght[max_size];
      int vertexNumber[max_size];

      int ind = 0;
      for (int i = 0; i < length; i++) {
         if (_vertex_stereo[i]) vertexNumber[ind++] = i;
      }

      if (ind < 3) {
         ind = 0;
         for (int i = 0; i < length; i++) vertexNumber[ind++] = i;
      }
      vertexNumber[ind] = length;

      for (int i = 0; i < ind - 1; i++) edgeLenght[i] = vertexNumber[i + 1] - vertexNumber[i];
      edgeLenght[ind - 1] = vertexNumber[0] - vertexNumber[ind - 1] + length;

      for (int i = 0; i < ind; i++) 
         if (vertexNumber[i] != 0) {
            rotateAngle[i] = rot_result[vertexNumber[i] + 1] > rot_result[vertexNumber[i]] ? 1 : rot_result[vertexNumber[i] + 1] == rot_result[vertexNumber[i]] ? 0 : -1;
         }
      if (vertexNumber[0] == 0) {
         rotateAngle[0] = 1;
      }
      
      //rotateAngle[0] = -1;
      Vec2f p[max_size];
      for (int i = 0; i <= ind; i++) {
         p[i] = Vec2f(y_result[vertexNumber[i]], 0);
         p[i].rotate(PI/3);
         p[i] += Vec2f(x_result[vertexNumber[i]], 0);
      }


      for (int i = ind; i >= 0; i--)
         p[i] -= p[0];
      if (points[index].type == CIRCLE_TYPE) {
         Vec2f rotate_vector(p[ind]/p[ind].length());
         for (int i = 0; i <= ind; i++) {
            p[i].rotateL(rotate_vector);
         }
         double max_x = p[ind].x;
         if (max_x <= 0) max_x = 1;
         double radii = max_x / (2 * PI);

         for (int i = 0; i <= ind; i++) {
            double angle = 2 * PI * (p[i].x) / max_x;
            p[i].set(radii - p[i].y/2, 0);
            p[i].rotate(angle);
         }
      }
      Vec2f last_vector(p[0] - p[ind]);
      for (int i = 0; i <= ind; i++) p[i] += (last_vector * vertexNumber[i]) / length;

      smoothing(ind, length, rotateAngle, edgeLenght, vertexNumber, p, profi);

      double newBadness = badness(ind, length, rotateAngle, edgeLenght, vertexNumber, p, points[index].diff);

      if (newBadness < bestBadness) {
         bestBadness = newBadness;
         bestIndex = index;

         for (int i = 0; i < ind; i++) {
            int nexti = (i + 1) % ind;

            for (int j = vertexNumber[i], t = 0; j != vertexNumber[nexti]; j = (j + 1) % length, t++) {
               _positions[j] = (p[i] * (edgeLenght[i] - t) + p[nexti] * t) / edgeLenght[i];
            }
         }
      }

   }

   Vec2f shifted_positons[max_size];
   for (int i = 0; i < length; i++) shifted_positons[(i + shift) % length] = _positions[i];
   for (int i = 0; i < length; i++) _positions[i] = shifted_positons[i];

   rotate(_vertex_weight.ptr(), length, -shift);
   rotate(_vertex_stereo.ptr(), length, -shift);
   rotate(_edge_stereo.ptr(), length, -shift);
   rotate(_angle_importance.ptr(), length, -shift);

   for (int i = 0; i < length; i++) 
      if (_vertex_weight[i] > 0) _vertex_weight[i]--;

   return bestBadness;
}

double MoleculeLayoutMacrocycles::depictionCircle() {

   int cisCount = 0;
   for (int i = 0; i < length; i++) if (_edge_stereo[i] == MoleculeCisTrans::CIS) cisCount++;

   int zero_edge_stereo_count = 0;
   for (int i = 0; i < length; i++)
      if (_edge_stereo[i] == 0) zero_edge_stereo_count++;
   if (zero_edge_stereo_count == 0) return 1000000;

   QS_DEF(Array<bool>, up);
   QS_DEF(Array<bool>, only_up);
   up.clear_resize(length + 1);
   only_up.clear_resize(length + 1);
   up.zerofill();
   only_up.zerofill();

   for (int i = 0; i < length; i++)
      if (_edge_stereo[i] == MoleculeCisTrans::CIS && _edge_stereo[(i + length - 1) % length] == MoleculeCisTrans::CIS) {
         only_up[(i + 1) % length] = 1;
         only_up[i] = 1;
         only_up[(i + length - 1) % length] = 1;
      }

   for (int i = 0; i < length; i++) up[i] = only_up[i];

   QS_DEF(Array<int>, free);
   free.clear_resize(length);

   bool exist_precalc = false;
   for (int i = 0; i < length; i++) exist_precalc |= only_up[i];

   if (exist_precalc) {
      int index_start = 0;
      int index_end = 0;
      int start = 0;
      for (int i = 0; i < length; i++) if (!only_up[i] && only_up[(i + length - 1) % length]) {
         index_start = i;
         index_end = index_start;
         while (!only_up[index_end]) index_end = (index_end + 1) % length;
         
         for (int j = index_start; j != index_end; j = (j + 1) % length)
            if (_edge_stereo[(j - 1 + length) % length] == MoleculeCisTrans::CIS) up[j] = up[(j - 1 + length) % length];
            else up[j] = !up[(j - 1 + length) % length];

         if (up[(index_end - 1 + length) % length]) {

            int index_flip = -1;
            for (int j = index_start; j != index_end; j = (j + 1) % length) 
               if (_edge_stereo[j] != MoleculeCisTrans::CIS && _edge_stereo[(j + length - 1) % length] != MoleculeCisTrans::CIS) index_flip = j;
            if (index_flip == -1) {
               free.zerofill();
               int index_free = 0;
               for (int j = index_start; j != index_end; j = (j + 1) % length)
                  if (_edge_stereo[(j - 1 + length) % length] == 0 || _edge_stereo[j] == 0) free[index_free++] = j;
               if (index_free > 0) index_flip = free[index_free/2];
               else index_flip = index_start;
            }

            for (int j = index_flip; j != index_end; j = (j + 1) % length) up[j] = !up[j];
         }
      }
   } else {
      for (int i = 0; i < length; i++)
      {
         if (_edge_stereo[i] == MoleculeCisTrans::CIS) up[i + 1] = up[i];
         else up[i + 1] = !up[i];
      }
   
      if ((cisCount + length) % 2 == 0)
      {
         // if first and last points on the same level
         int upCisCount = 0;
         int downCisCount = 0;

         for (int i = 0; i < length; i++)
         {
            if (_edge_stereo[i] == MoleculeCisTrans::CIS && up[i]) upCisCount++;
            if (_edge_stereo[i] == MoleculeCisTrans::CIS && !up[i]) downCisCount++;
         }
         if (downCisCount > upCisCount) {
            for (int i = 0; i <= length; i++) up[i] = !up[i];
         }
      } else {
         // if first and last points on the different levels
         if (cisCount == 0)
         {
            int index = 0;
            if (_edge_stereo[0] != 0 || _edge_stereo[length - 1] != 0 || _vertex_stereo[0] == 0) {
               for (int i = 1; i < length; i++) if (_edge_stereo[i] != 0 || _edge_stereo[i - 1] != 0 || _vertex_stereo[i] == 1) index = i;
            }
            for (int i = index; i <= length; i++) up[i] = !up[i];
            if (!up[index]) for (int i = 0; i <= length; i++) up[i] = !up[i];
         } else {
            int bestIndex = 0;
            int bestDiff = -1;
            for (int i = 0; i < length; i++) if (_edge_stereo[i] == MoleculeCisTrans::CIS || _edge_stereo[(i - 2 + length) % length] == MoleculeCisTrans::CIS){
               int diff = 0;
               for (int j = 0; j < length; j++) 
               {
                  if (_edge_stereo[i] == MoleculeCisTrans::CIS && ((up[i] && j < i) || (!up[i] && j >= i))) diff++;
                  if (_edge_stereo[i] == MoleculeCisTrans::CIS && !((up[i] && j < i) || (!up[i] && j >= i))) diff--;
               }
               if (up[i]) diff = -diff;
               if (diff > bestDiff) 
               {
                  bestDiff = diff;
                  bestIndex = i;
               }
            }

            for (int i = bestIndex; i <= length; i++) up[i] = !up[i];
         
            if (!up[bestIndex]) {
               for (int i = 0; i <= length; i++) up[i] = !up[i];
            }
         }
      }
   }

   int diff = 0;
   for (int i = 0; i < length; i++)
      diff += _vertex_weight[i] * (!up[i] && (up[(i + 1) % length] || up[(i + length - 1) % length]));

   for (int i = 0; i < length; i++)
      diff += (up[i] && up[(i + 1) % length]) || (!up[i] && !up[(i + 1) % length] && (up[(i - 1 + length) % length] == up[(i + 2) % length]));

   double r = length * sqrt(3.0)/2 / (2 * PI);

   QS_DEF(Array<Vec2f>, p);
   p.clear_resize(length);

   for (int i = 0; i < length; i++) {
      double rr = r;
      if (up[i]) rr += 0.25;
      else rr -= 0.25;

      p[i] = Vec2f(rr, 0);
      p[i].rotate(2*PI/length*i);
   }

   QS_DEF(Array<int>, rotateAngle);
   rotateAngle.clear_resize(length);

   for (int i = 0; i < length; i++) rotateAngle[i] = -1;
   int i = 0;
   while (_edge_stereo[(i - 1 + length) % length] != 0 && _edge_stereo[i] != 0) i++;
   for (; rotateAngle[i] == -1; i = (i + 1) % length) 
      if (_edge_stereo[(i - 1 + length) % length] == MoleculeCisTrans::CIS) rotateAngle[i] = rotateAngle[(i - 1 + length) % length];
      else if (_edge_stereo[(i - 1 + length) % length] == MoleculeCisTrans::TRANS) rotateAngle[i] = -rotateAngle[(i - 1 + length) % length];
      else rotateAngle[i] = up[i] ? 1 : (up[(i + 1) % length] || up[(i + length - 1) % length]) ? -1 : 1;

   QS_DEF(Array<int>, edgeLength);
   edgeLength.clear_resize(length);
   for (i = 0; i < length; i++) edgeLength[i] = 1;

   QS_DEF(Array<int>, vertexNumber);
   vertexNumber.clear_resize(length);
   for (i = 0; i < length; i++) vertexNumber[i] = i;

   /*double angle = PI * 2 * rand() / RAND_MAX;
   double sn = sin(angle);
   double cs = cos(angle);
   for (int i = 0; i < molSize; i++) {
      double xx = cs * x[i] - sn * y[i];
      double yy = sn * x[i] + cs * y[i];
      x[i] = xx;
      y[i] = yy;
   }*/

   smoothing(length, length, rotateAngle.ptr(), edgeLength.ptr(), vertexNumber.ptr(), p.ptr(), false);

   
   for (i = 0; i < length; i++)
   {
/*      Vec3f &pos = mol.getAtomXyz(vert[i]);
      pos.x = x[i];
      pos.y = y[i];
      pos.z = 0;*/
//      _positions[i].x = x[i];
  //    _positions[i].y = y[i];
      _positions[i] = p[i];
   }
   
   return badness(length, length, rotateAngle.ptr(), edgeLength.ptr(), vertexNumber.ptr(), p.ptr(), diff);
}
