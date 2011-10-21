#ifndef DSTORM_UNITENTRIES_NANOMETRE_H
#define DSTORM_UNITENTRIES_NANOMETRE_H

#include <simparm/BoostUnits.hh>
#include <simparm/Entry.hh>
#include <boost/units/systems/si/length.hpp>
#include "../units/nanolength.h"

namespace dStorm {
    typedef simparm::Entry< boost::units::quantity< boost::units::si::nanolength, float > > 
        FloatNanometreEntry;
}

#endif