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

#include "main.h"
#include <Eigen/Geometry>
#include <Eigen/LU>

template<typename HyperplaneType> void hyperplane(const HyperplaneType& _plane)
{
  /* this test covers the following files:
     Hyperplane.h
  */

  const int dim = _plane.dim();
  typedef typename HyperplaneType::Scalar Scalar;
  typedef typename NumTraits<Scalar>::Real RealScalar;
  typedef Matrix<Scalar, HyperplaneType::AmbientDimAtCompileTime, 1> VectorType;

  VectorType p0 = VectorType::Random(dim);
  VectorType p1 = VectorType::Random(dim);

  VectorType n0 = VectorType::Random(dim).normalized();
  VectorType n1 = VectorType::Random(dim).normalized();
  
  HyperplaneType pl0(n0, p0);
  HyperplaneType pl1(n1, p1);

  Scalar s0 = ei_random<Scalar>();
  Scalar s1 = ei_random<Scalar>();

  VERIFY_IS_APPROX( n1.dot(n1), Scalar(1) );
  VERIFY_IS_APPROX( n1.dot(n1), Scalar(1) );
  
  VERIFY_IS_MUCH_SMALLER_THAN( pl0.absDistance(p0), Scalar(1) );
  VERIFY_IS_APPROX( pl1.signedDistance(p1 + n1 * s0), s0 );
  VERIFY_IS_MUCH_SMALLER_THAN( pl1.signedDistance(pl1.projection(p0)), Scalar(1) );
  VERIFY_IS_MUCH_SMALLER_THAN( pl1.absDistance(p1 +  pl1.normal().unitOrthogonal() * s1), Scalar(1) );
}

template<typename Scalar> void lines()
{
  typedef Hyperplane<Scalar, 2> HLine;
  typedef ParametrizedLine<Scalar, 2> PLine;
  typedef Matrix<Scalar,2,1> Vector;
  typedef Matrix<Scalar,3,1> CoeffsType;

  for(int i = 0; i < 10; i++)
  {
    Vector center = Vector::Random();
    Vector u = Vector::Random();
    Vector v = Vector::Random();
    Scalar a = ei_random<Scalar>();

    HLine line_u = HLine::Through(center + u, center + a*u);
    HLine line_v = HLine::Through(center + v, center + a*v);

    // the line equations should be normalized so that a^2+b^2=1
    VERIFY_IS_APPROX(line_u.normal().norm(), Scalar(1));
    VERIFY_IS_APPROX(line_v.normal().norm(), Scalar(1));

    Vector result = line_u.intersection(line_v);

    // the lines should intersect at the point we called "center"
    VERIFY_IS_APPROX(result, center);

    // check conversions between two types of lines
    CoeffsType converted_coeffs(HLine(PLine(line_u)).coeffs());
    converted_coeffs *= line_u.coeffs()(0)/converted_coeffs(0);
    VERIFY(line_u.coeffs().isApprox(converted_coeffs));
  }
}

void test_hyperplane()
{
  for(int i = 0; i < g_repeat; i++) {
    CALL_SUBTEST( hyperplane(Hyperplane<float,2>()) );
    CALL_SUBTEST( hyperplane(Hyperplane<float,3>()) );
    CALL_SUBTEST( hyperplane(Hyperplane<double,4>()) );
    CALL_SUBTEST( hyperplane(Hyperplane<std::complex<double>,5>()) );
    CALL_SUBTEST( lines<float>() );
    CALL_SUBTEST( lines<double>() );
  }
}
