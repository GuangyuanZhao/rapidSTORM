#include "ColourScheme.h"
#include "debug.h"
#include "Config.h"
#include <dStorm/doc/context.h>
#include <simparm/ChoiceEntry_Impl.hh>
#include "colour_schemes/decl.h"

namespace dStorm {
namespace viewer {

_Config::_Config()
: simparm::Object("Image", "Image display"),
  showOutput("ShowOutput", "Display dSTORM result image"),
  outputFile("ToFile", "Save image to", ".png"),
  histogramPower("HistogramPower", "Extent of histogram normalization",
                 0.3),
  colourScheme("ColourScheme", "Colour palette for display"),
  invert("InvertColours", "Invert colours", false),
  save_with_key("SaveWithKey", "Save output image with key", true),
  save_scale_bar("SaveScaleBar", "Save output image with scale bar", true),
  close_on_completion("CloseOnCompletion", 
                      "Close display on job completion"),
  border("Border", "Width of border to chop", 0 * camera::pixel)
{
    DEBUG("Building Viewer Config");

    outputFile.make_optional();
    outputFile.optional_given = true;
    outputFile.helpID = HELP_Viewer_ToFile;
    outputFile.setUserLevel(simparm::Object::Beginner);

    showOutput.helpID = HELP_Viewer_ShowOutput;
    showOutput.setUserLevel(simparm::Object::Beginner);

    histogramPower.setMin(0);
    histogramPower.setMax(1);
    /* This level is reset in carStarted() */
    histogramPower.setUserLevel(simparm::Object::Expert);

    colourScheme.helpID = HELP_Viewer_ColorScheme;
#define DISC_INSTANCE(Scheme) \
    colourScheme.addChoice( ColourScheme::config_for<Scheme>() );
#include "colour_schemes/instantiate.h"

    invert.helpID = HELP_Viewer_InvertColors;

    close_on_completion.setUserLevel(simparm::Object::Debug);
    save_with_key.setUserLevel(simparm::Object::Intermediate);
    save_scale_bar.setUserLevel(simparm::Object::Intermediate);
    border.setUserLevel(simparm::Object::Intermediate);

    DEBUG("Built Viewer Config");
}

void _Config::registerNamedEntries() {
   push_back(outputFile);
   push_back(save_with_key);
   push_back(save_scale_bar);
   push_back(showOutput);
   push_back(binned_dimensions);
   push_back(histogramPower);
   push_back(colourScheme);
   push_back(invert);
   push_back(close_on_completion);
   push_back(border);
}

_Config::~_Config() {}

}
}