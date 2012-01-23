#ifndef DSTORM_VIEWER_CONFIG_H
#define DSTORM_VIEWER_CONFIG_H

#include "Config_decl.h"

#include <simparm/Eigen_decl.hh>
#include <simparm/BoostUnits.hh>
#include <simparm/Eigen.hh>
#include <simparm/ChoiceEntry.hh>
#include <simparm/Structure.hh>
#include <dStorm/output/BasenameAdjustedFileEntry.h>
#include <simparm/Entry.hh>
#include <dStorm/output/Output.h>

#include <dStorm/UnitEntries/PixelEntry.h>
#include <dStorm/outputs/BinnedLocalizations_strategies_config.h>
#include "ColourScheme.h"
#include "Image.h"

namespace dStorm {
namespace viewer {

class _Config : public simparm::Object {
  public:
    typedef Eigen::Matrix< boost::units::quantity<boost::units::camera::length,int>, 3, 1 >
        CropBorder;

    simparm::BoolEntry showOutput;
    output::BasenameAdjustedFileEntry outputFile;
    outputs::DimensionSelector<Im::Dim> binned_dimensions;
    simparm::Entry<double> histogramPower;
    simparm::NodeChoiceEntry<ColourScheme> colourScheme;
    simparm::BoolEntry invert, save_with_key, save_scale_bar, close_on_completion;
    simparm::Entry< CropBorder > border;

    _Config();
    ~_Config();

    void registerNamedEntries() { registerNamedEntries(*this); }
    void registerNamedEntries( simparm::Node& at );
    static bool can_work_with(output::Capabilities) { return true; }
    
    CropBorder crop_border() const;
};

}
}

#endif
