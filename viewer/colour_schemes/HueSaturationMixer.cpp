#include "debug.h"
#include "HueSaturationMixer.h"
#include <cassert>

#include <dStorm/image/constructors.h>
#include <dStorm/image/contains.h>

using namespace boost::units;


namespace dStorm {
namespace viewer {
namespace colour_schemes {

void HueSaturationMixer::set_tone( float hue ) {
    DEBUG("Setting tone to hue " << hue);
    float saturation = 0;
    current_tone = base_tone + ColourVector( hue, saturation );
    tone_point.x() = cosf( 2 * M_PI * current_tone[0] ) 
                        * current_tone[1];
    tone_point.y() = sinf( 2 * M_PI * current_tone[0] ) 
                        * current_tone[1];
}

void HueSaturationMixer::merge_tone( const Im::Position& pos,
                    float old_data_weight, float new_data_weight )
{
    assert( contains( colours, pos ) );
    ColourVector hs;
    if ( old_data_weight < 1E-3 ) {
        colours(pos) = tone_point;
        hs = current_tone;
        DEBUG("Used tone point " << colours(pos).transpose() << " for new point");
    } else {
        colours(pos) = 
            ( colours(pos) * old_data_weight + 
                tone_point * new_data_weight ) /
                ( old_data_weight + new_data_weight );
        convert_xy_tone_to_hue_sat
            ( colours(pos).x(), colours(pos).y(), hs[0], hs[1] );
        DEBUG("Merged tone point " << colours(pos).transpose() << " from " << tone_point << " for new point");
    }
    rgb_weights_from_hue_saturation( hs[0], hs[1], rgb_weights(pos) );
}

HueSaturationMixer::HueSaturationMixer( double base_hue, double base_saturation ) {
    base_tone[0] = base_hue;
    base_tone[1] = base_saturation;
}
HueSaturationMixer::~HueSaturationMixer() {}

void HueSaturationMixer::setSize(const dStorm::Image<ColourVector,Im::Dim>::Size& size) {
    colours.invalidate();
    rgb_weights.invalidate();
    colours = dStorm::Image<ColourVector,Im::Dim>(size);
    rgb_weights = dStorm::Image<RGBWeight,Im::Dim>(size);
    RGBWeight zero;
    std::fill( zero.begin(), zero.end(), 0 );
    rgb_weights.fill(zero);
}

}
}
}
