#define DSTORM_TIFFLOADER_CPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_TIFFIO_H

#include "debug.h"

#include <stdexcept>
#include <cassert>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <stdint.h>

#include <simparm/ChoiceEntry_Impl.hh>

#include <dStorm/Image.h>
#include <tiffio.h>

#include "TIFF.h"
#include <dStorm/input/Source.h>
#include <dStorm/engine/Image.h>
#include <dStorm/image/MetaInfo.h>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/units/base_units/us/inch.hpp>

#include "TIFFOperation.h"
#include <dStorm/signals/InputFileNameChange.h>
#include <dStorm/input/InputMutex.h>

#include "dejagnu.h"

using namespace std;

namespace dStorm {
namespace TIFF {

const std::string test_file_name = "special-debug-value-rapidstorm:file.tif";

Source::Source( boost::shared_ptr<OpenFile> file )
: simparm::Set("TIFF", "TIFF image reader"),
  file(file)
{
}

class Source::iterator 
: public boost::iterator_facade<iterator,Image,std::random_access_iterator_tag>
{
    mutable OpenFile* src;
    mutable simparm::Node* msg;
    int directory;
    mutable boost::optional<Image> img;

    void go_to_position(TIFFOperation& op) const {
        int rv = TIFFSetDirectory(src->tiff, directory);
        if ( rv == 1 ) {
            src->current_directory = directory;
        } else {
            op.throw_exception_for_errors();
        }
    }
    void check_position(TIFFOperation& op) const {
        if ( src->current_directory != directory )
            go_to_position(op);
    }
    void check_params() const;

  public:
    iterator() : src(NULL), msg(NULL) {}
    iterator(Source &s) : src(s.file.get()), msg(&s), directory(0) {}

    Image& dereference() const; 
    bool equal(const iterator& i) const {
        DEBUG( "Comparing " << src << " " << i.src << " " << directory << " " << i.directory );
        return (src == i.src) && (src == NULL || i.src == NULL || directory == i.directory);
    }
    void increment() { 
        TIFFOperation op( "in reading TIFF file",
                          *msg, src->ignore_warnings );
        check_position(op);
        img.reset(); 
        if ( TIFFReadDirectory(src->tiff) != 1 ) {
            op.throw_exception_for_errors();
            /* Code from here only executed when no error
             * was encountered */
            DEBUG( "Setting iterator to NULL" );
            src = NULL;
        } else {
            directory++; 
            src->current_directory = directory;
        }
    }
    void decrement() { 
        img.reset(); 
        if ( directory == 0 ) 
            src = NULL; 
        else {
            --directory;
            TIFFOperation op( "in reading TIFF file",
                            *msg, src->ignore_warnings );
            go_to_position(op);
        }
    }
    void advance(int n) { 
        if (n) {
            TIFFOperation op( "in reading TIFF file",
                            *msg, src->ignore_warnings );
            img.reset(); 
            directory += n;
            go_to_position(op);
        }
    }
    int distance_to(const iterator& i) {
        return i.directory - directory;
    }
};

void Source::iterator::check_params() const
{
    ::TIFF *tiff = src->tiff;
    uint32_t width, height, depth;
    uint16_t bitspersample, colors;
    TIFFGetField( tiff, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField( tiff, TIFFTAG_IMAGELENGTH, &height );
    if ( ! TIFFGetField( tiff, TIFFTAG_IMAGEDEPTH, &depth ) )
        depth = 1;
    if ( TIFFGetField( tiff, TIFFTAG_SAMPLESPERPIXEL, &colors ) && colors != 1 ) {
        std::stringstream error;
        error << "TIFF image no. " << TIFFCurrentDirectory(tiff)
            << " has " << colors << " color channels, but only greyscale images can be processed. Aborting.";
        throw std::runtime_error( error.str() );
    }
    TIFFGetField( tiff, TIFFTAG_BITSPERSAMPLE, &bitspersample );

    if ( int(width) != src->size[0] || int(height) != src->size[1] || int(depth) != src->size[2] ) {
        std::stringstream error;
        error << "TIFF image no. " << TIFFCurrentDirectory(tiff)
            << " has dimensions (" << width << ", " << height << ", " << depth
            << ") different from first image (" << src->size[0]
            << ", " << src->size[1] << ", " << src->size[2] << "). Aborting.";
        throw std::runtime_error( error.str() );
    }
    if ( bitspersample != sizeof(Pixel)*8 ) {
        std::stringstream error;
        error << "TIFF image no. " << TIFFCurrentDirectory(tiff) << " has "
                << bitspersample << " bits per pixel, but "
                << sizeof(Pixel)*8 << " bits are necessary.";
        throw std::runtime_error( error.str() );
    }
}

Source::Image&
Source::iterator::dereference() const
{ 
    if ( ! img.is_initialized() ) {
        TIFFOperation op( "in reading TIFF file",
                          *msg, src->ignore_warnings );
        check_position(op);
        check_params();

        typename Plane::Size sz;
        ::TIFF *tiff = src->tiff;
        tsize_t strip_size = TIFFStripSize( tiff );
        tstrip_t strip_count = TIFFNumberOfStrips( tiff );
        assert( sz.rows() <= 3 );
        for (int i = 0; i < sz.rows(); ++i)
            sz[i] = src->size[i] * camera::pixel;
        Plane i( sz, directory * camera::frame);

        DEBUG("Reading image " << directory << " of size " << i.sizes().transpose() << " from TIFF with " << strip_count << " strips with " << strip_size / sizeof(Pixel) << " pixels each for an image sized " << src->size[0] << " " << src->size[1]);

        Pixel* data = i.ptr(), *end_of_data = data + i.size_in_pixels();
        for (tstrip_t strip = 0; strip < strip_count; strip++) {
            tsize_t remaining_space = sizeof(Pixel) * (end_of_data - data);
            DEBUG("Reading strip into buffer starting at " << data << " and having " << remaining_space << " bytes remaining");
            tsize_t really_read = TIFFReadEncodedStrip( tiff, strip, data,
                std::min( remaining_space, strip_size ) );
            data += std::min( really_read, strip_size ) / sizeof(Pixel);
        }
        DEBUG("Finished reading image");
        img = i;

        op.throw_exception_for_errors();
        DEBUG("Thrown no exceptions");
    }

    return *img;
}

Source::base_iterator
Source::begin() {
    return base_iterator( iterator(*this) );
}

Source::base_iterator
Source::end() {
    return base_iterator( iterator() );
}

Config::Config()
: simparm::Object("TIFF", "TIFF file"),
  ignore_warnings("IgnoreLibtiffWarnings",
    "Ignore libtiff warnings", true),
  determine_length("DetermineFileLength",
    "Determine length of file", false)
{
}

ChainLink::ChainLink() 
: simparm::Listener( simparm::Event::ValueChanged )
{
    receive_changes_from( config.ignore_warnings.value );
    receive_changes_from( config.determine_length.value );
}

BaseSource*
ChainLink::makeSource()
{
    return new Source( get_file() );
}

Source::TraitsPtr 
Source::get_traits(typename BaseSource::Wishes) {
    simparm::Entry<long> count( "EntryCount", "Number of images in TIFF file", 0 );
    count.editable = false;
    push_back(count);
    DEBUG("Creating traits from file object");
    TraitsPtr rv = TraitsPtr( file->getTraits(true, count).release() );
    DEBUG("Returning traits " << rv.get());
    return rv;
}

Source::~Source() {}

void ChainLink::operator()(const simparm::Event& e) {
    InputMutexGuard lock( global_mutex() );
    republish_traits();
}

void unit_test( TestState& s ) {
    ChainLink l;
    l.publish_meta_info();
    s.testrun( l.current_meta_info().get() 
            && l.current_meta_info()->provides_nothing(),
        "Traits are right for empty file" );
    l.current_meta_info()->get_signal< signals::InputFileNameChange >()( test_file_name );
    s.testrun( l.current_meta_info().get() 
            && ! l.current_meta_info()->provides_nothing(),
        "Traits are right for test file" );
}

void ChainLink::modify_meta_info( MetaInfo& i ) {
    i.accepted_basenames.push_back( make_pair("extension_tif", ".tif") );
    i.accepted_basenames.push_back( make_pair("extension_tiff", ".tiff") );
}
OpenFile* ChainLink::make_file( const std::string& n ) const
{
    DEBUG( "Creating file structure for " << n );
    return new OpenFile( n, config, const_cast<simparm::Node&>(static_cast<const simparm::Node&>(config)) );
}

}
}

#endif
