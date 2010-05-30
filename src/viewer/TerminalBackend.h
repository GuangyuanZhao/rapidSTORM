#ifndef DSTORM_VIEWER_TERMINALBACKEND_H
#define DSTORM_VIEWER_TERMINALBACKEND_H

#include "ColourDisplay.h"
#include "ImageDiscretizer.h"
#include "Display.h"
#include "Backend.h"
#include "TerminalCache.h"

#include <dStorm/outputs/BinnedLocalizations.h>
#include <dStorm/helpers/DisplayManager.h>

namespace dStorm {
namespace viewer {

template <int Hueing>
class TerminalBackend 
: 
  public Backend
{
  private:
    typedef HueingColorizer<Hueing> Colorizer;
    typedef TerminalCache<Colorizer> Cache;
    typedef Discretizer<Cache> MyDiscretizer;
    typedef outputs::BinnedLocalizations<MyDiscretizer> Accumulator;

    typedef dStorm::Image<dStorm::Pixel,2> Im;

    /** Binned image with all localizations in localizationsStore.*/
    Accumulator image;
    Colorizer colorizer;
    /** Discretized version of \c image. */
    MyDiscretizer discretization;
    Cache cache;

  public:
    TerminalBackend(const Config& config);
    ~TerminalBackend() ;

    output::Output& getForwardOutput() { return image; }
    void save_image(std::string filename, bool with_key);

    void set_histogram_power(float power);
    void set_resolution_enhancement(float re);
};

}
}

#endif

