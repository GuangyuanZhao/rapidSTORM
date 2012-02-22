#include "TerminalCache.h"
#include "debug.h"
#include <boost/units/io.hpp>
#include "Display.h"

namespace dStorm {
namespace viewer {

TerminalCache::TerminalCache()
{
}

void TerminalCache::setSize( 
    const display::ResizeChange& size
)
{
    this->size = size;
}

void TerminalCache::setSize(
    const Im::MetaInfo& traits
) {
    DEBUG("Setting size of image to " << traits.size.x() << " " <<traits.size.y());
    size.set_size( Im::Size(traits.size) );
    for (int i = 0; i < std::min(display::Image::Dim, Im::Dim); ++i)
        if ( traits.has_resolution() )
            size.pixel_sizes[i] = traits.resolution(i);
        else
            size.pixel_sizes[i].value = -1 / camera::pixel;
}

}
}
