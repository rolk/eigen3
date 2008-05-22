// This file is part of Eigen, a lightweight C++ template library
// for linear algebra. Eigen itself is part of the KDE project.
//
// Copyright (C) 2008 Gael Guennebaud <g.gael@free.fr>
// Copyright (C) 2006-2008 Benoit Jacob <jacob@math.jussieu.fr>
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

#include "main.h"
#include <functional>

using namespace std;

template<typename Scalar> struct AddIfNull {
    const Scalar operator() (const Scalar a, const Scalar b) const {return a<=1e-3 ? b : a;}
    enum { Cost = NumTraits<Scalar>::AddCost };
};

template<typename MatrixType> void cwiseops(const MatrixType& m)
{
  typedef typename MatrixType::Scalar Scalar;
  typedef Matrix<Scalar, MatrixType::RowsAtCompileTime, 1> VectorType;

  int rows = m.rows();
  int cols = m.cols();

  MatrixType m1 = MatrixType::random(rows, cols),
             m2 = MatrixType::random(rows, cols),
             m3(rows, cols),
             mzero = MatrixType::zero(rows, cols),
             mones = MatrixType::ones(rows, cols),
             identity = Matrix<Scalar, MatrixType::RowsAtCompileTime, MatrixType::RowsAtCompileTime>
                              ::identity(rows, rows),
             square = Matrix<Scalar, MatrixType::RowsAtCompileTime, MatrixType::RowsAtCompileTime>
                              ::random(rows, rows);
  VectorType v1 = VectorType::random(rows),
             v2 = VectorType::random(rows),
             vzero = VectorType::zero(rows);

  m2 = m2.template cwise<AddIfNull<Scalar> >(mones);

  VERIFY_IS_APPROX(               mzero,    m1-m1);
  VERIFY_IS_APPROX(               m2,       m1+m2-m1);
  VERIFY_IS_APPROX(               mones,    m2.cwiseQuotient(m2));
  VERIFY_IS_APPROX(               m1.cwiseProduct(m2),    m2.cwiseProduct(m1));

  VERIFY( m1.cwiseLessThan(m1.cwise(bind2nd(plus<Scalar>(), Scalar(1)))).all() );
  VERIFY( !m1.cwiseLessThan(m1.cwise(bind2nd(minus<Scalar>(), Scalar(1)))).all() );
  VERIFY( !m1.cwiseGreaterThan(m1.cwise(bind2nd(plus<Scalar>(), Scalar(1)))).any() );
  //VERIFY_IS_APPROX(               m1,       m2.cwiseProduct(m1).cwiseQuotient(m2));

//   VERIFY_IS_APPROX( cwiseMin(m1,m2), cwiseMin(m2,m1) );
//   VERIFY_IS_APPROX( cwiseMin(m1,m1+mones), m1 );
//   VERIFY_IS_APPROX( cwiseMin(m1,m1-mones), m1-mones );
}

void test_cwiseop()
{
  for(int i = 0; i < g_repeat ; i++) {
    CALL_SUBTEST( cwiseops(Matrix<float, 1, 1>()) );
    CALL_SUBTEST( cwiseops(Matrix4d()) );
    CALL_SUBTEST( cwiseops(MatrixXf(3, 3)) );
    CALL_SUBTEST( cwiseops(MatrixXi(8, 12)) );
    CALL_SUBTEST( cwiseops(MatrixXd(20, 20)) );
  }
}
