#ifndef NONLINFIT_IMAGE_PLANEEVALUATOR_H
#define NONLINFIT_IMAGE_PLANEEVALUATOR_H

#include <Eigen/Core>
#include <dStorm/Image.h>
#include <dStorm/ImageTraits.h>
#include <nonlinfit/plane/fwd.h>
#include "Spot.h"
#include <nonlinfit/plane/Joint.h>
#include <nonlinfit/plane/Disjoint.h>
#include <dStorm/Image.h>

namespace dStorm {
namespace guf {

template <int Dimensions> struct Statistics;

template <typename LengthUnit>
struct TransformedImage {
    typedef boost::units::quantity<LengthUnit> Length;
    typedef dStorm::traits::Optics<2> Optics;
    typedef ImageTypes<2>::Size ImageSize;
    typedef Eigen::Array< boost::units::quantity<boost::units::camera::length, int>, 2, 2,
        Eigen::DontAlign >
        Bounds;
    const Optics optics;
    const Spot max_distance;

  private:
    void set_generic_data( nonlinfit::plane::GenericData<LengthUnit>&, const Spot& center ) const;
  public:
    TransformedImage( const Spot& max_distance, const Optics& optics );
    const Spot& get_max_distance() const { return max_distance; }
    Bounds cut_region( const Spot& center, const ImageSize& upper_bound ) const; 

    template <typename PixelType, typename Num, int ChunkSize, typename Transform >
    Statistics<2> set_data( 
      nonlinfit::plane::DisjointData< Num,LengthUnit,ChunkSize >& target,
      const dStorm::Image< PixelType, 2 >& image, const Spot& center,
      const Transform& ) const;
    template <typename PixelType, typename Num, int ChunkSize, typename Transform >
    Statistics<2> set_data( 
      nonlinfit::plane::JointData< Num, LengthUnit,ChunkSize >& target,
      const dStorm::Image< PixelType, 2 >& image, const Spot& center,
      const Transform& ) const;
};

}
}

#endif
