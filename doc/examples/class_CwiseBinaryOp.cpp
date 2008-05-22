#include <Eigen/Core>
USING_PART_OF_NAMESPACE_EIGEN
using namespace std;

// define a custom template binary functor
template<typename Scalar> struct MakeComplexOp EIGEN_EMPTY_STRUCT {
  typedef complex<Scalar> result_type;
  complex<Scalar> operator()(const Scalar& a, const Scalar& b) const { return complex<Scalar>(a,b); }
  enum { Cost = 0 };
};

int main(int, char**)
{
  Matrix4d m1 = Matrix4d::random(), m2 = Matrix4d::random();
  cout << m1.cwise(m2, MakeComplexOp<double>()) << endl;
  return 0;
}
