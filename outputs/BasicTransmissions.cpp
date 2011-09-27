#define CImgBuffer_TIFFLOADER_CPP
#include "BasicTransmissions.h"

#include "../viewer/plugin.h"
#include "LocalizationCounter.h"
#include "ProgressMeter.h"
#include "AverageImage.h"
#include <dStorm/outputs/TraceFilter.h>
#include <dStorm/expression/Source_decl.h>
#include "Slicer.h"
#include "RawImageFile.h"
#include "MemoryCache_decl.h"

using namespace std;
using namespace dStorm::outputs;

namespace dStorm {
namespace output {

void basic_outputs( Config* o ) {
    outputs::add_viewer( *o );
    o->addChoice( new ProgressMeter::Source() );
    o->addChoice( new LocalizationCounter::Source() );
    o->addChoice( new AverageImage::Source() );
    o->addChoice( make_output_source<MemoryCache>().release() );
    o->addChoice( new TraceCountFilter::Source() );
    o->addChoice( new Slicer::Source() );
    o->addChoice( new RawImageFile::Source() );
    o->addChoice( make_output_source<expression::Source>().release() );
}

}
}
