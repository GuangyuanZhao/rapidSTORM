#ifndef DSTORM_VIEWER_CONFIG_H
#define DSTORM_VIEWER_CONFIG_H

#include "Config_decl.h"

#include <simparm/BoostUnits.hh>
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

    simparm::BoolEntry showOutput;
    output::BasenameAdjustedFileEntry outputFile;
    outputs::DimensionSelector<Im::Dim> binned_dimensions;
    simparm::Entry<double> histogramPower;
    simparm::NodeChoiceEntry<ColourScheme> colourScheme;
    simparm::BoolEntry invert, save_with_key, save_scale_bar, close_on_completion;
    dStorm::IntPixelEntry border;

    _Config();
    ~_Config();

    void registerNamedEntries();
    static bool can_work_with(output::Capabilities) { return true; }
};

}
}

#endif
