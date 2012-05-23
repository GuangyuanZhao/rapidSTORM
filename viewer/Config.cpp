#include "Config.h"
#include "ColourScheme.h"
#include "debug.h"
#include <simparm/ChoiceEntry_Impl.hh>
#include "colour_schemes/decl.h"
#include <simparm/Entry_Impl.hh>

namespace dStorm {
namespace viewer {

Config::Config()
: showOutput("ShowOutput", "Display dSTORM result image"),
  density_matrix_given("SaveDensityMatrix", "Save density matrix", false ),
  outputFile("ToFile", "Save image to", ".png"),
  density_matrix("DensityMatrixFile", "Save density matrix to", "-density.txt"),
  histogramPower("HistogramPower", "Extent of histogram normalization", 0.3),
  top_cutoff("IntensityCutoff", "Intensity cutoff", 1.0),
  colourScheme("ColourScheme", "Colour palette for display"),
  invert("InvertColours", "Invert colours", false),
  save_with_key("SaveWithKey", "Save output image with key", true),
  save_scale_bar("SaveScaleBar", "Save output image with scale bar", true),
  close_on_completion("CloseOnCompletion", 
                      "Close display on job completion"),
  border("Border", "Width of border to chop", CropBorder::Constant(0 * camera::pixel)),
  scale_bar_length("ScaleBarLength", "Length of scale bar", 5 * boost::units::si::micrometer)
{
    DEBUG("Building Viewer Config");

    outputFile.setUserLevel(simparm::Object::Beginner);

    density_matrix_given.setUserLevel(simparm::Object::Expert);
    density_matrix.setUserLevel(simparm::Object::Expert);
    density_matrix.help = "Save a text file with the unnormalized intensities of the result image";

    scale_bar_length.setUserLevel( simparm::Object::Intermediate );

    showOutput.setUserLevel(simparm::Object::Beginner);

    histogramPower.min = (0);
    histogramPower.max = (1);
    /* This level is reset in carStarted() */
    histogramPower.setUserLevel(simparm::Object::Expert);
    top_cutoff.min = 0;
    top_cutoff.max = 1.0;
    top_cutoff.userLevel = simparm::Object::Expert;
    top_cutoff.help = "Maximum displayed intensity as a fraction of the "
       "maximum found intensity";

#define DISC_INSTANCE(Scheme) \
    colourScheme.addChoice( ColourScheme::config_for<Scheme>() );
#include "colour_schemes/instantiate.h"

    close_on_completion.setUserLevel(simparm::Object::Debug);
    save_with_key.setUserLevel(simparm::Object::Intermediate);
    save_scale_bar.setUserLevel(simparm::Object::Intermediate);
    border.setUserLevel(simparm::Object::Intermediate);
    invert.userLevel = simparm::Object::Intermediate;

    outputFile.helpID = "#Viewer_ToFile";
    showOutput.helpID = "#Viewer_ShowOutput";
    colourScheme.helpID = "#Viewer_ColorScheme";
    invert.helpID = "#Viewer_InvertColors";
    top_cutoff.helpID = "#Viewer_TopCutoff";

    DEBUG("Built Viewer Config");
}

void Config::attach_ui( simparm::NodeHandle n ) {
    listening[3] = colourScheme.value.notify_on_value_change( boost::ref(some_value_changed) );
    listening[4] = invert.value.notify_on_value_change( boost::ref(some_value_changed) );
    listening[5] = border.value.notify_on_value_change( boost::ref(some_value_changed) );

   outputFile.attach_ui(n);
   save_with_key.attach_ui(n);
   save_scale_bar.attach_ui(n);
   showOutput.attach_ui(n);
   density_matrix_given.attach_ui(n);
   density_matrix.attach_ui(n);

   binned_dimensions.attach_ui(n);
   histogramPower.attach_ui(n);
   top_cutoff.attach_ui(n);
   colourScheme.attach_ui(n);
   invert.attach_ui(n);
   close_on_completion.attach_ui(n);
   border.attach_ui(n);
   scale_bar_length.attach_ui(n);
}

void Config::backend_needs_changing( simparm::BaseAttribute::Listener l ) {
    some_value_changed.connect( l );
    binned_dimensions.add_listener(l);

    for ( simparm::ManagedChoiceEntry<ColourScheme>::iterator i = colourScheme.begin(); i != colourScheme.end(); ++i)
        i->add_listener( l );
}

Config::~Config() {}

Config::CropBorder
Config::crop_border() const
{
    CropBorder rv = border();
    if ( ! binned_dimensions.is_3d() )
        rv[2] = 0 * camera::pixel;
    return rv;
}

}
}

namespace simparm { template class Entry< dStorm::viewer::Config::CropBorder >; }
