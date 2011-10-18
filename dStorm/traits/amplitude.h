#ifndef DSTORM_TRAITS_AMPLITUDE_H
#define DSTORM_TRAITS_AMPLITUDE_H

#include "base.h"
#include <boost/units/systems/camera/intensity.hpp>

namespace dStorm {
namespace traits {

struct Amplitude;
template <> struct value< Amplitude > :
    public Value<  quantity< camera::intensity, float > > {};

struct Amplitude 
: public value<Amplitude>,
  public Resolution< Amplitude, divide_typeof_helper< float,
    quantity< camera::intensity, float > >::type >,
  public Range<Amplitude>
{
    static std::string get_ident();
    static std::string get_desc();
    static std::string get_shorthand();
    static const ValueType default_value;
};

}
}

#endif
