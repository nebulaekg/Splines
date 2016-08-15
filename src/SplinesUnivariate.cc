/*--------------------------------------------------------------------------*\
 |                                                                          |
 |  Copyright (C) 1998                                                      |
 |                                                                          |
 |         , __                 , __                                        |
 |        /|/  \               /|/  \                                       |
 |         | __/ _   ,_         | __/ _   ,_                                | 
 |         |   \|/  /  |  |   | |   \|/  /  |  |   |                        |
 |         |(__/|__/   |_/ \_/|/|(__/|__/   |_/ \_/|/                       |
 |                           /|                   /|                        |
 |                           \|                   \|                        |
 |                                                                          |
 |      Enrico Bertolazzi                                                   |
 |      Dipartimento di Ingegneria Industriale                              |
 |      Universita` degli Studi di Trento                                   |
 |      email: enrico.bertolazzi@unitn.it                                   |
 |                                                                          |
\*--------------------------------------------------------------------------*/

#include "Splines.hh"
#include <cmath>
#include <iomanip>

/**
 * 
 */

#ifndef isnan
#define isnan(A) ((A) != (A))
#endif

#ifndef isfinite
#define isfinite(A) ((A*0.0) == 0.0)
#endif

//#ifndef isregular
//#define isregular(A) ( !isnan(A) && isfinite(A) )
//#endif

namespace Splines {
  //! Associate a number for each type of splines implemented

  /*
  //    ____  ____   ____                               _
  //   / ___|/ ___| / ___| _   _ _ __  _ __   ___  _ __| |_
  //  | |  _| |     \___ \| | | | '_ \| '_ \ / _ \| '__| __|
  //  | |_| | |___   ___) | |_| | |_) | |_) | (_) | |  | |_
  //   \____|\____| |____/ \__,_| .__/| .__/ \___/|_|   \__|
  //                            |_|   |_|
  */

  #ifdef SPLINES_USE_GENERIC_CONTAINER

  using GenericContainerNamespace::GC_VEC_REAL ;
  using GenericContainerNamespace::GC_VEC_INTEGER ;
  using GenericContainerNamespace::vec_int_type ;
  using GenericContainerNamespace::vec_real_type ;

  void
  Spline::setup( GenericContainer const & gc ) {
    /*
    // gc["x"]
    // gc["y"]
    //
    */
    SPLINE_ASSERT( gc.exists("x"), "[" << type_name() << "[" << _name << "]::setup] missing `x` field!") ;
    SPLINE_ASSERT( gc.exists("y"), "[" << type_name() << "[" << _name << "]::setup] missing `y` field!") ;

    GenericContainer const & gc_x = gc("x") ;
    GenericContainer const & gc_y = gc("y") ;

    vec_real_type x, y ;
    gc_x.copyto_vec_real( x, "Spline::setup, field `x`" ) ;
    gc_y.copyto_vec_real( y, "Spline::setup, field `y`" ) ;
    build( x, y ) ;
  }
  #endif

  /*       _               _    _   _       _   _
  //   ___| |__   ___  ___| | _| \ | | __ _| \ | |
  //  / __| '_ \ / _ \/ __| |/ /  \| |/ _` |  \| |
  // | (__| | | |  __/ (__|   <| |\  | (_| | |\  |
  //  \___|_| |_|\___|\___|_|\_\_| \_|\__,_|_| \_|
  */
  //! check if the vector `pv` os size `DIM` contains only regular floats. If not an error is issued
  void
  checkNaN( valueType const pv[],
            char      const v_name[],
            sizeType  const DIM ) {
    for ( sizeType i = 0 ; i < DIM ; ++i ) {
      if ( isnan(pv[i]) ) {
        std::ostringstream ost ;
        ost << "\nfound NaN at " << v_name << "[" << i << "]" ;
        throw std::runtime_error(ost.str()) ;
      }
      if ( !isfinite(pv[i]) ) {
        std::ostringstream ost ;
        ost << "\nfound Infinity at " << v_name << "[" << i << "]" ;
        throw std::runtime_error(ost.str()) ;
      }
    }
  }
  
  void
  Spline::pushBack( valueType x, valueType y ) {
    if ( npts > 0 ) {
      SPLINE_ASSERT( x >= X[npts-1], // ammetto punti doppi
                     "Spline::pushBack, non monotone insert at insert N. " << npts <<
                     "\nX[ " << npts-1 << "] = " << X[npts-1] <<
                     "\nX[ " << npts   << "] = " << x ) ;
    }
    if ( npts_reserved == 0 ) {
      reserve( 2 ) ;
    } else if ( npts >= npts_reserved ) {
      // riallocazione & copia
      sizeType saved_npts = npts ; // salvo npts perche reserve lo azzera
      vector<valueType> Xsaved(npts) ;
      vector<valueType> Ysaved(npts) ;

      std::copy( X, X+npts, Xsaved.begin() ) ;
      std::copy( Y, Y+npts, Ysaved.begin() ) ;
      reserve( (npts+1) * 2 ) ;
      npts = saved_npts ;
      std::copy( Xsaved.begin(), Xsaved.end(), X ) ;
      std::copy( Ysaved.begin(), Ysaved.end(), Y ) ;
    }
    X[npts] = x ;
    Y[npts] = y ;
    ++npts ;
  }

  sizeType
  Spline::search( valueType x ) const {
    SPLINE_ASSERT( npts > 0, "\nsearch(" << x << ") empty spline");
    if ( lastInterval+1 >= npts || X[lastInterval] < x || X[lastInterval+1] > x ) {
      if ( _check_range ) {
        SPLINE_ASSERT( x >= X[0] && x <= X[npts-1],
                       "method search( " << x << " ) out of range: [" <<
                       X[0] << ", " << X[npts-1] << "]" ) ;
      }
      lastInterval = sizeType(lower_bound( X, X+npts, x ) - X) ;
      if ( lastInterval > 0 ) --lastInterval ;
      if ( X[lastInterval] == X[lastInterval+1] ) ++lastInterval ; // degenerate interval for duplicated nodes
      if ( lastInterval+1 >= npts ) lastInterval = npts-2 ;
    }
    return lastInterval ;
  }

  ///////////////////////////////////////////////////////////////////////////
  void
  Spline::setOrigin( valueType x0 ) {
    valueType Tx = x0 - X[0] ;
    valueType *ix = X ;
    while ( ix < X+npts ) *ix++ += Tx ;
  }

  void
  Spline::setRange( valueType xmin, valueType xmax ) {
    SPLINE_ASSERT( xmax > xmin, "Spline::setRange( " << xmin << " , " << xmax << " ) bad range ") ;
    valueType S  = (xmax - xmin) / ( X[npts-1] - X[0] ) ;
    valueType Tx = xmin - S * X[0] ;
    for( valueType *ix = X ; ix < X+npts ; ++ix ) *ix = *ix * S + Tx ;
  }

  ///////////////////////////////////////////////////////////////////////////
  void
  Spline::dump( ostream & s, sizeType nintervals, char const header[] ) const {
    s << header << '\n' ;
    valueType dx = (xMax()-xMin())/nintervals ;
    for ( sizeType i = 0 ; i <= nintervals ; ++i ) {
      valueType x = xMin() + i*dx ;
      s << x << '\t' << (*this)(x) << '\n' ;
    }
  }

}
