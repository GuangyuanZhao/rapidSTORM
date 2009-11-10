#include "config.h"

#include <dStorm/input/Config.h>
#include <CImg.h>
#include <dStorm/input/LocalizationFileReader.h>
#include "AndorSIF.h"
#include "TIFF.h"
#ifdef HAVE_LIBATMCD32D
#include "AndorCamera/AndorDirect.h"
#endif

#include <simparm/ChoiceEntry_Impl.hh>

namespace dStorm {

using engine::StormPixel;

void basic_inputs( input::Config* inputConfig ) {
    inputConfig->inputMethod.addChoice( 
        new input::LocalizationFileReader::Config( *inputConfig ) );
#ifdef HAVE_LIBREADSIF
    inputConfig->inputMethod.addChoice( 
        new input::AndorSIF::Config<StormPixel>( *inputConfig ) );
#endif
#ifdef HAVE_TIFFIO_H
    inputConfig->inputMethod.addChoice( 
        new TIFF::Config<StormPixel>( *inputConfig ) );
#endif
#ifdef HAVE_LIBATMCD32D
    inputConfig->inputMethod.addChoice( 
        new AndorDirect::Config( *inputConfig ) );
#endif
    
}

}