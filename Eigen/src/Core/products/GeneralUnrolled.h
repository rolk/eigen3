// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2006-2008 Benoit Jacob <jacob.benoit.1@gmail.com>
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

#ifndef EIGEN_GENERAL_UNROLLED_PRODUCT_H
#define EIGEN_GENERAL_UNROLLED_PRODUCT_H

/*********************************************************************************
*  Specialization of GeneralProduct<> for products with small fixed sizes
*********************************************************************************/

/* Since the all the dimensions of the product are small, here we can rely
 * on the generic Assign mechanism to evaluate the product per coeff (or packet).
 *
 * Note that here the inner-loops should always be unrolled.
 */

template<int VectorizationMode, int Index, typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl;

template<int StorageOrder, int Index, typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl;

template<typename LhsNested, typename RhsNested>
struct ei_traits<GeneralProduct<LhsNested,RhsNested,UnrolledProduct> >
{
  typedef typename ei_cleantype<LhsNested>::type _LhsNested;
  typedef typename ei_cleantype<RhsNested>::type _RhsNested;
  typedef typename ei_scalar_product_traits<typename _LhsNested::Scalar, typename _RhsNested::Scalar>::ReturnType Scalar;
  typedef typename ei_promote_storage_type<typename ei_traits<_LhsNested>::StorageType,
                                           typename ei_traits<_RhsNested>::StorageType>::ret StorageType;

  enum {
      LhsCoeffReadCost = _LhsNested::CoeffReadCost,
      RhsCoeffReadCost = _RhsNested::CoeffReadCost,
      LhsFlags = _LhsNested::Flags,
      RhsFlags = _RhsNested::Flags,

      RowsAtCompileTime = _LhsNested::RowsAtCompileTime,
      ColsAtCompileTime = _RhsNested::ColsAtCompileTime,
      InnerSize = EIGEN_ENUM_MIN(_LhsNested::ColsAtCompileTime, _RhsNested::RowsAtCompileTime),

      MaxRowsAtCompileTime = _LhsNested::MaxRowsAtCompileTime,
      MaxColsAtCompileTime = _RhsNested::MaxColsAtCompileTime,

      LhsRowMajor = LhsFlags & RowMajorBit,
      RhsRowMajor = RhsFlags & RowMajorBit,

      CanVectorizeRhs = RhsRowMajor && (RhsFlags & PacketAccessBit)
                      && (ColsAtCompileTime == Dynamic || (ColsAtCompileTime % ei_packet_traits<Scalar>::size) == 0),

      CanVectorizeLhs = (!LhsRowMajor) && (LhsFlags & PacketAccessBit)
                      && (RowsAtCompileTime == Dynamic || (RowsAtCompileTime % ei_packet_traits<Scalar>::size) == 0),

      EvalToRowMajor = RhsRowMajor && (!CanVectorizeLhs),

      RemovedBits = ~(EvalToRowMajor ? 0 : RowMajorBit),

      Flags = ((unsigned int)(LhsFlags | RhsFlags) & HereditaryBits & RemovedBits)
            | EvalBeforeAssigningBit
            | EvalBeforeNestingBit
            | (CanVectorizeLhs || CanVectorizeRhs ? PacketAccessBit : 0)
            | (LhsFlags & RhsFlags & AlignedBit),

      CoeffReadCost = InnerSize == Dynamic ? Dynamic
                    : InnerSize * (NumTraits<Scalar>::MulCost + LhsCoeffReadCost + RhsCoeffReadCost)
                      + (InnerSize - 1) * NumTraits<Scalar>::AddCost,

      /* CanVectorizeInner deserves special explanation. It does not affect the product flags. It is not used outside
      * of Product. If the Product itself is not a packet-access expression, there is still a chance that the inner
      * loop of the product might be vectorized. This is the meaning of CanVectorizeInner. Since it doesn't affect
      * the Flags, it is safe to make this value depend on ActualPacketAccessBit, that doesn't affect the ABI.
      */
      CanVectorizeInner = LhsRowMajor && (!RhsRowMajor) && (LhsFlags & RhsFlags & ActualPacketAccessBit)
                        && (InnerSize % ei_packet_traits<Scalar>::size == 0)
    };
};

template<typename LhsNested, typename RhsNested> class GeneralProduct<LhsNested,RhsNested,UnrolledProduct>
  : ei_no_assignment_operator,
    public MatrixBase<GeneralProduct<LhsNested, RhsNested, UnrolledProduct> >
{
  public:

    EIGEN_GENERIC_PUBLIC_INTERFACE(GeneralProduct)

  private:

    typedef typename ei_traits<GeneralProduct>::_LhsNested _LhsNested;
    typedef typename ei_traits<GeneralProduct>::_RhsNested _RhsNested;

    enum {
      PacketSize = ei_packet_traits<Scalar>::size,
      InnerSize  = ei_traits<GeneralProduct>::InnerSize,
      Unroll = CoeffReadCost <= EIGEN_UNROLLING_LIMIT,
      CanVectorizeInner = ei_traits<GeneralProduct>::CanVectorizeInner
    };

    typedef ei_product_coeff_impl<CanVectorizeInner ? InnerVectorization : NoVectorization,
                                  Unroll ? InnerSize-1 : Dynamic,
                                  _LhsNested, _RhsNested, Scalar> ScalarCoeffImpl;

  public:

    template<typename Lhs, typename Rhs>
    inline GeneralProduct(const Lhs& lhs, const Rhs& rhs)
      : m_lhs(lhs), m_rhs(rhs)
    {
      // we don't allow taking products of matrices of different real types, as that wouldn't be vectorizable.
      // We still allow to mix T and complex<T>.
      EIGEN_STATIC_ASSERT((ei_is_same_type<typename Lhs::RealScalar, typename Rhs::RealScalar>::ret),
        YOU_MIXED_DIFFERENT_NUMERIC_TYPES__YOU_NEED_TO_USE_THE_CAST_METHOD_OF_MATRIXBASE_TO_CAST_NUMERIC_TYPES_EXPLICITLY)
      ei_assert(lhs.cols() == rhs.rows()
        && "invalid matrix product"
        && "if you wanted a coeff-wise or a dot product use the respective explicit functions");
    }

    EIGEN_STRONG_INLINE int rows() const { return m_lhs.rows(); }
    EIGEN_STRONG_INLINE int cols() const { return m_rhs.cols(); }

    EIGEN_STRONG_INLINE const Scalar coeff(int row, int col) const
    {
      Scalar res;
      ScalarCoeffImpl::run(row, col, m_lhs, m_rhs, res);
      return res;
    }

    /* Allow index-based non-packet access. It is impossible though to allow index-based packed access,
     * which is why we don't set the LinearAccessBit.
     */
    EIGEN_STRONG_INLINE const Scalar coeff(int index) const
    {
      Scalar res;
      const int row = RowsAtCompileTime == 1 ? 0 : index;
      const int col = RowsAtCompileTime == 1 ? index : 0;
      ScalarCoeffImpl::run(row, col, m_lhs, m_rhs, res);
      return res;
    }

    template<int LoadMode>
    EIGEN_STRONG_INLINE const PacketScalar packet(int row, int col) const
    {
      PacketScalar res;
      ei_product_packet_impl<Flags&RowMajorBit ? RowMajor : ColMajor,
                                   Unroll ? InnerSize-1 : Dynamic,
                                   _LhsNested, _RhsNested, PacketScalar, LoadMode>
        ::run(row, col, m_lhs, m_rhs, res);
      return res;
    }

  protected:
    const LhsNested m_lhs;
    const RhsNested m_rhs;
};


/***************************************************************************
* Normal product .coeff() implementation (with meta-unrolling)
***************************************************************************/

/**************************************
*** Scalar path  - no vectorization ***
**************************************/

template<int Index, typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<NoVectorization, Index, Lhs, Rhs, RetScalar>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, RetScalar &res)
  {
    ei_product_coeff_impl<NoVectorization, Index-1, Lhs, Rhs, RetScalar>::run(row, col, lhs, rhs, res);
    res += lhs.coeff(row, Index) * rhs.coeff(Index, col);
  }
};

template<typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<NoVectorization, 0, Lhs, Rhs, RetScalar>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, RetScalar &res)
  {
    res = lhs.coeff(row, 0) * rhs.coeff(0, col);
  }
};

template<typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<NoVectorization, Dynamic, Lhs, Rhs, RetScalar>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, RetScalar& res)
  {
    ei_assert(lhs.cols()>0 && "you are using a non initialized matrix");
    res = lhs.coeff(row, 0) * rhs.coeff(0, col);
      for(int i = 1; i < lhs.cols(); ++i)
        res += lhs.coeff(row, i) * rhs.coeff(i, col);
  }
};

// prevent buggy user code from causing an infinite recursion
template<typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<NoVectorization, -1, Lhs, Rhs, RetScalar>
{
  EIGEN_STRONG_INLINE static void run(int, int, const Lhs&, const Rhs&, RetScalar&) {}
};

/*******************************************
*** Scalar path with inner vectorization ***
*******************************************/

template<int Index, typename Lhs, typename Rhs, typename PacketScalar>
struct ei_product_coeff_vectorized_unroller
{
  enum { PacketSize = ei_packet_traits<typename Lhs::Scalar>::size };
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, typename Lhs::PacketScalar &pres)
  {
    ei_product_coeff_vectorized_unroller<Index-PacketSize, Lhs, Rhs, PacketScalar>::run(row, col, lhs, rhs, pres);
    pres = ei_padd(pres, ei_pmul( lhs.template packet<Aligned>(row, Index) , rhs.template packet<Aligned>(Index, col) ));
  }
};

template<typename Lhs, typename Rhs, typename PacketScalar>
struct ei_product_coeff_vectorized_unroller<0, Lhs, Rhs, PacketScalar>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, typename Lhs::PacketScalar &pres)
  {
    pres = ei_pmul(lhs.template packet<Aligned>(row, 0) , rhs.template packet<Aligned>(0, col));
  }
};

template<int Index, typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<InnerVectorization, Index, Lhs, Rhs, RetScalar>
{
  typedef typename Lhs::PacketScalar PacketScalar;
  enum { PacketSize = ei_packet_traits<typename Lhs::Scalar>::size };
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, RetScalar &res)
  {
    PacketScalar pres;
    ei_product_coeff_vectorized_unroller<Index+1-PacketSize, Lhs, Rhs, PacketScalar>::run(row, col, lhs, rhs, pres);
    ei_product_coeff_impl<NoVectorization,Index,Lhs,Rhs,RetScalar>::run(row, col, lhs, rhs, res);
    res = ei_predux(pres);
  }
};

template<typename Lhs, typename Rhs, int LhsRows = Lhs::RowsAtCompileTime, int RhsCols = Rhs::ColsAtCompileTime>
struct ei_product_coeff_vectorized_dyn_selector
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, typename Lhs::Scalar &res)
  {
    res = ei_dot_impl<
      Block<Lhs, 1, ei_traits<Lhs>::ColsAtCompileTime>,
      Block<Rhs, ei_traits<Rhs>::RowsAtCompileTime, 1>,
      LinearVectorization, NoUnrolling>::run(lhs.row(row), rhs.col(col));
  }
};

// NOTE the 3 following specializations are because taking .col(0) on a vector is a bit slower
// NOTE maybe they are now useless since we have a specialization for Block<Matrix>
template<typename Lhs, typename Rhs, int RhsCols>
struct ei_product_coeff_vectorized_dyn_selector<Lhs,Rhs,1,RhsCols>
{
  EIGEN_STRONG_INLINE static void run(int /*row*/, int col, const Lhs& lhs, const Rhs& rhs, typename Lhs::Scalar &res)
  {
    res = ei_dot_impl<
      Lhs,
      Block<Rhs, ei_traits<Rhs>::RowsAtCompileTime, 1>,
      LinearVectorization, NoUnrolling>::run(lhs, rhs.col(col));
  }
};

template<typename Lhs, typename Rhs, int LhsRows>
struct ei_product_coeff_vectorized_dyn_selector<Lhs,Rhs,LhsRows,1>
{
  EIGEN_STRONG_INLINE static void run(int row, int /*col*/, const Lhs& lhs, const Rhs& rhs, typename Lhs::Scalar &res)
  {
    res = ei_dot_impl<
      Block<Lhs, 1, ei_traits<Lhs>::ColsAtCompileTime>,
      Rhs,
      LinearVectorization, NoUnrolling>::run(lhs.row(row), rhs);
  }
};

template<typename Lhs, typename Rhs>
struct ei_product_coeff_vectorized_dyn_selector<Lhs,Rhs,1,1>
{
  EIGEN_STRONG_INLINE static void run(int /*row*/, int /*col*/, const Lhs& lhs, const Rhs& rhs, typename Lhs::Scalar &res)
  {
    res = ei_dot_impl<
      Lhs,
      Rhs,
      LinearVectorization, NoUnrolling>::run(lhs, rhs);
  }
};

template<typename Lhs, typename Rhs, typename RetScalar>
struct ei_product_coeff_impl<InnerVectorization, Dynamic, Lhs, Rhs, RetScalar>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, typename Lhs::Scalar &res)
  {
    ei_product_coeff_vectorized_dyn_selector<Lhs,Rhs>::run(row, col, lhs, rhs, res);
  }
};

/*******************
*** Packet path  ***
*******************/

template<int Index, typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<RowMajor, Index, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar &res)
  {
    ei_product_packet_impl<RowMajor, Index-1, Lhs, Rhs, PacketScalar, LoadMode>::run(row, col, lhs, rhs, res);
    res =  ei_pmadd(ei_pset1(lhs.coeff(row, Index)), rhs.template packet<LoadMode>(Index, col), res);
  }
};

template<int Index, typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<ColMajor, Index, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar &res)
  {
    ei_product_packet_impl<ColMajor, Index-1, Lhs, Rhs, PacketScalar, LoadMode>::run(row, col, lhs, rhs, res);
    res =  ei_pmadd(lhs.template packet<LoadMode>(row, Index), ei_pset1(rhs.coeff(Index, col)), res);
  }
};

template<typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<RowMajor, 0, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar &res)
  {
    res = ei_pmul(ei_pset1(lhs.coeff(row, 0)),rhs.template packet<LoadMode>(0, col));
  }
};

template<typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<ColMajor, 0, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar &res)
  {
    res = ei_pmul(lhs.template packet<LoadMode>(row, 0), ei_pset1(rhs.coeff(0, col)));
  }
};

template<typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<RowMajor, Dynamic, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar& res)
  {
    ei_assert(lhs.cols()>0 && "you are using a non initialized matrix");
    res = ei_pmul(ei_pset1(lhs.coeff(row, 0)),rhs.template packet<LoadMode>(0, col));
      for(int i = 1; i < lhs.cols(); ++i)
        res =  ei_pmadd(ei_pset1(lhs.coeff(row, i)), rhs.template packet<LoadMode>(i, col), res);
  }
};

template<typename Lhs, typename Rhs, typename PacketScalar, int LoadMode>
struct ei_product_packet_impl<ColMajor, Dynamic, Lhs, Rhs, PacketScalar, LoadMode>
{
  EIGEN_STRONG_INLINE static void run(int row, int col, const Lhs& lhs, const Rhs& rhs, PacketScalar& res)
  {
    ei_assert(lhs.cols()>0 && "you are using a non initialized matrix");
    res = ei_pmul(lhs.template packet<LoadMode>(row, 0), ei_pset1(rhs.coeff(0, col)));
      for(int i = 1; i < lhs.cols(); ++i)
        res =  ei_pmadd(lhs.template packet<LoadMode>(row, i), ei_pset1(rhs.coeff(i, col)), res);
  }
};

#endif // EIGEN_GENERAL_UNROLLED_PRODUCT_H
