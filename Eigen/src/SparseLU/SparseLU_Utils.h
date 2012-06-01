// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2012 Désiré Nuentsa-Wakam <desire.nuentsa_wakam@inria.fr>
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

#ifdef EIGEN_SPARSELU_UTILS_H
#define EIGEN_SPARSELU_UTILS_H

// Number of marker arrays used in the symbolic factorization  each of size n 
#define LU_NO_MARKER 3
#define LU_NUM_TEMPV(m,w,t,b) (std::max(m, (t+b)*w)  )
#define IND_EMPTY (-1)

void SparseLU::LU_countnz(const int n, VectorXi& xprune, int& nnzL, int& nnzU, GlobalLU_t& Glu)
{
 VectorXi& xsup = Glu.xsup; 
 VectorXi& xlsub = Glu.xlsub; 
 nnzL = 0; 
 nnzU = (Glu.xusub)(n); 
 int nnzL0 = 0; 
 int nsuper = (Glu.supno)(n); 
 int jlen, irep; 
 
 if (n <= 0 ) return; 
 // For each supernode
 for (i = 0; i <= nsuper; i++)
 {
   fsupc = xsup(i); 
   jlen = xlsub(fsupc+1) - xlsub(fsupc); 
   
   for (j = fsupc; j < xsup(i+1); j++)
   {
     nnzL += jlen; 
     nnzLU += j - fsupc + 1; 
     jlen--; 
   }
   irep = xsup(i+1) - 1; 
   nnzL0 += xprune(irep) - xlsub(irep); 
 }
 
}
/**
 * \brief Fix up the data storage lsub for L-subscripts. 
 * 
 * It removes the subscripts sets for structural pruning, 
 * and applies permutation to the remaining subscripts
 * 
 */
void SparseLU::LU_fixupL(const int n, const VectorXi& perm_r, GlobalLU_t& Glu)
{
  int nsuper, fsupc, i, j, k, jstart; 
  VectorXi& xsup = GLu.xsup; 
  VectorXi& lsub = Glu.lsub; 
  VectorXi& xlsub = Glu.xlsub; 
  
  int nextl = 0; 
  int nsuper = (Glu.supno)(n); 
  
  // For each supernode 
  for (i = 0; i <= nsuper; i++)
  {
    fsupc = xsup(i); 
    jstart = xlsub(fsupc); 
    xlsub(fsupc) = nextl; 
    for (j = jstart; j < xlsub(fsupc + 1); j++)
    {
      lsub(nextl) = perm_r(lsub(j)); // Now indexed into P*A
      nextl++
    }
    for (k = fsupc+1; k < xsup(i+1); k++)
      xlsub(k) = nextl; // other columns in supernode i
  }
  
  xlsub(n) = nextl; 
}

#endif
