#ifndef DSTORM_IMAGETRAITS_IMPL_H
#define DSTORM_IMAGETRAITS_IMPL_H

#include "ImageTraits.h"
#include <stdint.h>

namespace dStorm {
namespace input {

#if 0
template <typename Pixel, int Dim>
void
Traits< dStorm::Image<Pixel,Dim> >::
compute_resolution( const Config& config ) {
    this->resolution = typename SizeTraits<Dim>::Resolution::value_type( 
        cs_units::camera::pixels_per_meter /
            (config.pixel_size_in_nm() / 1E9) );
}
#endif

}
}

#endif
