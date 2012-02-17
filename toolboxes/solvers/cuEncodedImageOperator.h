#pragma once

#include "encodedImageOperator.h"
#include "cuMatrixOperator_macros.h"
#include "cuNDArray.h"

template <class REAL, class T> 
class cuEncodedImageOperator : public encodedImageOperator< REAL, cuNDArray<REAL>, cuNDArray<T> >
{

 public:

  cuEncodedImageOperator( int device = -1 ) : encodedImageOperator< REAL, cuNDArray<REAL>, cuNDArray<T> >() { set_device(device); }
  virtual ~cuEncodedImageOperator() {}

  virtual int compute( cuNDArray<T> *image )
  {
    set_device();
    int res = encodedImageOperator< REAL, cuNDArray<REAL>, cuNDArray<T> >::compute( image );
    restore_device();
    return res;
  }

  virtual int mult_MH_M( cuNDArray<T> *in, cuNDArray<T> *out, bool accumulate = false )
  {
    set_device();
    int res = encodedImageOperator< REAL, cuNDArray<REAL>, cuNDArray<T> >::mult_MH_M( in, out, accumulate );
    restore_device();
    return res;
  }
  
  virtual void operator_scal( REAL scale, cuNDArray<T> *x )
  {
    cuNDA_scal<T>( T(scale), x );
  }

  virtual void operator_reciprocal( cuNDArray<REAL> *x )
  {
    cuNDA_reciprocal<REAL>(x);
  }

  virtual REAL operator_asum( cuNDArray<T> *x )
  {
    return cuNDA_asum<REAL,T>(x);
  }

  virtual boost::shared_ptr< cuNDArray<REAL> > operator_norm_squared( cuNDArray<T> *x )
  {
    return cuNDA_cNorm<REAL,T>( x, CUNDA_NDARRAY_DEVICE, CUNDA_NDARRAY_DEVICE);
  }
  
  virtual bool operator_clear(  cuNDArray<T> *x )
  {
    return cuNDA_clear(x);
  }
 
  virtual bool operator_axpy( cuNDArray<REAL> *a,  cuNDArray<T> *x,  cuNDArray<T> *y )
  {
    return cuNDA_axpy( a, x, y );
  }



  DECLARE_MATRIX_OPERATOR_DEVICE_SUPPORT(cuEncodedImageOperator)
};
