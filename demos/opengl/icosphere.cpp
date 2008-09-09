// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#include "icosphere.h"

#include <GL/gl.h>

using namespace Eigen;

//--------------------------------------------------------------------------------
// icosahedron data
//--------------------------------------------------------------------------------
#define X .525731112119133606
#define Z .850650808352039932

static GLfloat vdata[12][3] = {
   {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
   {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
   {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}
};

static GLint tindices[20][3] = {
   {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
   {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
   {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
   {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
//--------------------------------------------------------------------------------

IcoSphere::IcoSphere(unsigned int levels)
{
  // init with an icosahedron
  for (int i = 0; i < 12; i++)
    mVertices.push_back(Map<Vector3f>(vdata[i]));
  mIndices.push_back(new std::vector<int>);
  std::vector<int>& indices = *mIndices.back();
  for (int i = 0; i < 20; i++)
  {
    for (int k = 0; k < 3; k++)
      indices.push_back(tindices[i][k]);
  }
  mListIds.push_back(0);

  while(mIndices.size()<levels)
    _subdivide();
}

const std::vector<int>& IcoSphere::indices(int level) const
{
  while (level>=int(mIndices.size()))
    const_cast<IcoSphere*>(this)->_subdivide();
  return *mIndices[level];
}

void IcoSphere::_subdivide(void)
{
  const std::vector<int>& indices = *mIndices.back();
  mIndices.push_back(new std::vector<int>);
  std::vector<int>& refinedIndices = *mIndices.back();
  int end = indices.size();
  for (int i=0; i<end; i+=3)
  {
    int i0, i1, i2;
    Vector3f v0 = mVertices[i0=indices[i+0]];
    Vector3f v1 = mVertices[i1=indices[i+1]];
    Vector3f v2 = mVertices[i2=indices[i+2]];
    int start = mVertices.size();
    mVertices.push_back( (v0+v1).normalized() );
    mVertices.push_back( (v1+v2).normalized() );
    mVertices.push_back( (v2+v0).normalized() );
    refinedIndices.push_back(i0); refinedIndices.push_back(start+0); refinedIndices.push_back(start+2);
    refinedIndices.push_back(i1); refinedIndices.push_back(start+1); refinedIndices.push_back(start+0);
    refinedIndices.push_back(i2); refinedIndices.push_back(start+2); refinedIndices.push_back(start+1);
    refinedIndices.push_back(start+0); refinedIndices.push_back(start+1); refinedIndices.push_back(start+2);
  }
  mListIds.push_back(0);
}

void IcoSphere::draw(int level)
{
  while (level>=int(mIndices.size()))
    const_cast<IcoSphere*>(this)->_subdivide();
  if (mListIds[level]==0)
  {
    mListIds[level] = glGenLists(1);
    glNewList(mListIds[level], GL_COMPILE);
      glVertexPointer(3, GL_FLOAT, 0, mVertices[0].data());
      glNormalPointer(GL_FLOAT, 0, mVertices[0].data());
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_NORMAL_ARRAY);
      glDrawElements(GL_TRIANGLES, mIndices[level]->size(), GL_UNSIGNED_INT, &(mIndices[level]->at(0)));
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_NORMAL_ARRAY);
    glEndList();
  }
  glCallList(mListIds[level]);
}


