#ifndef DSTORM_OUTPUTS_BINNEDLOCALIZATIONS_STRATEGIES_CONFIG_H
#define DSTORM_OUTPUTS_BINNEDLOCALIZATIONS_STRATEGIES_CONFIG_H

#include <simparm/Object.hh>
#include <simparm/ChoiceEntry.hh>
#include "../output/binning/config_decl.h"
#include "../output/binning/binning_decl.h"
#include <boost/ptr_container/ptr_array.hpp>
#include "BinnedLocalizations_decl.h"
#include "../Localization.h"

namespace dStorm {
namespace outputs {

class DimensionSelector
: public simparm::Object
{
    simparm::BoolEntry invert_y_axis;

    boost::ptr_array< output::binning::FieldChoice, 3 > components;
    void init();

  public:
    DimensionSelector();
    DimensionSelector(const DimensionSelector&);
    ~DimensionSelector();
    DimensionSelector* clone() const { return new DimensionSelector(); }

    std::auto_ptr< BinningStrategy > make() const;
    std::auto_ptr< output::binning::Unscaled > make_unscaled(int field) const;
    void set_visibility(const input::Traits<Localization>&);
};

}
}

#endif