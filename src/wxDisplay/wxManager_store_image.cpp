#include "debug.h"
#include "wxManager.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <Magick++.h>
#include <cmath>

static const char *SI_prefixes[]
= { "f", "p", "n", "�", "m", "", "k", "M", "G", "T",
    "E" };
std::string SIize( float value ) {
    if ( value < 1E-21 ) return "0";
    int prefix = int(floor(log10(value))) / 3;
    prefix = std::max(-5, std::min(prefix, 5));
    float rv = value / pow(1000, prefix);
    char buffer[128];
    snprintf( buffer, 128, "%.3g %s", rv, 
              SI_prefixes[prefix + 5] );
    return buffer;
}

namespace dStorm {
namespace Display {

#ifdef HAVE_LIBGRAPHICSMAGICK__
template <int MagickDepth>
inline void 
    make_magick_pixel( Magick::PixelPacket& mp, const dStorm::Pixel& p );

template <>
inline void 
    make_magick_pixel<8>( Magick::PixelPacket& mp,
                          const dStorm::Pixel& p )
{
    mp.red = p.red();
    mp.blue = p.blue();
    mp.green = p.green();
    mp.opacity = p.alpha();
}

template <>
inline void 
    make_magick_pixel<16>( Magick::PixelPacket& mp,
                           const dStorm::Pixel& p )
{
    mp.red = (p.red() | (p.red() << 8));
    mp.blue = (p.blue() | (p.blue() << 8));
    mp.green = (p.green() | (p.green() << 8));
    mp.opacity = (p.alpha() | (p.alpha() << 8));
}

static
std::auto_ptr<Magick::Image> 
make_palette( const data_cpp::Vector<KeyChange>& key ) 
{
    DEBUG("Making palette");
    std::auto_ptr< Magick::Image > rv
        ( new Magick::Image(
            Magick::Geometry( key.size(), 1 ),
            Magick::ColorRGB( 0, 0, 0 ) ) );
    rv->type(Magick::TrueColorType);

    Magick::PixelPacket *pixels = 
        rv->getPixels(0, 0, key.size(), 1);
    for (int i = 0; i < key.size(); i++)
        make_magick_pixel<QuantumDepth>
            ( pixels[i], key[i].color );

    rv->syncPixels();
    DEBUG("Made palette");
    return rv;
}

static std::auto_ptr<Magick::Image>
make_unannotated_key_image( 
    int width, int color_bar_height, int annotation_area_height,
    Magick::Color background,
    std::auto_ptr<Magick::Image> palette
) {
    DEBUG("Making unannotated key");
    int border = 5;
    int w = std::max(1, width-2*border);
    Magick::Geometry key_img_size 
        = Magick::Geometry(w, color_bar_height);
    key_img_size.aspect( true );
    palette->sample( key_img_size );

    std::auto_ptr<Magick::Image> rv ( 
        new Magick::Image(
            Magick::Geometry(width, color_bar_height+annotation_area_height),
            background ) );
    rv->composite( *palette, (width-w)/2, 0, Magick::OverCompositeOp );
    DEBUG("Made unannotated key");
    return rv;
}

static
std::auto_ptr<Magick::Image> 
make_key_image( 
    int width,
    Magick::Color foreground,
    Magick::Color background,
    const data_cpp::Vector<KeyChange>& key )
{
    DEBUG("Making annotated key");
    std::auto_ptr<Magick::Image> rv, palette = make_palette(key);
    int lh = palette->fontPointsize();
    int midline = 20;
    int text_area_height = 5*lh;
    int key_annotation_height = lh;
    rv = make_unannotated_key_image( 
        width, midline, text_area_height + key_annotation_height,
        background, palette);

    DEBUG("Annotating");
    Magick::TypeMetric metrics;
    rv->strokeColor( foreground );
    rv->fillColor( foreground );
    for (int i = lh/6; i < width-(lh-lh/6); i += lh )
    {
        int index = round(i * key.size() * 1.0 / width );
        index = std::max(0, std::min(index, key.size()));
        DEBUG("Annotating at index " << index << " for key size " << key.size() << " and position " << i << " of " << width);
        float value = key[ index ].value;
        std::string s = SIize(value);
        rv->annotate(s, 
            Magick::Geometry( lh, text_area_height-5,
                                i-lh/6, midline+5),
            Magick::NorthWestGravity, 90 );
        rv->draw( Magick::DrawableLine( i, midline,
                                        i, midline+3 ) );
    }

    DEBUG("Writing key annotation");
    std::string message =
        "Key: total A/D counts per pixel";
    if ( width >= 20*lh )
      rv->annotate( message,
        Magick::Geometry( width, 
            key_annotation_height, 0, midline+text_area_height ),
            Magick::NorthGravity, 0);

    DEBUG("Made annotated key");
    return rv;
}

static void write_main_image(
    Magick::Image& image,
    int width,
    const ImageChange& whole_image,
    const Change::PixelQueue& small_changes
) {
    DEBUG("Writing main image");
    int cols = width, rows = whole_image.pixels.size() / cols;
    DEBUG("Using stride " << width);

    const Pixel *p = whole_image.pixels.ptr();
    for (int y = 0; y < rows; y++) {
        Magick::PixelPacket *pixels = image.setPixels
            ( 0, y, cols, 1 );
        for (int x = 0; x < cols; x++) {
            make_magick_pixel<QuantumDepth>( pixels[x], *p );
            ++p;
        }
        image.syncPixels();
    }
    DEBUG("Wrote main image");
}

static void write_scale_bar(
    Magick::Image& image,
    float meters_per_pixel,
    int width,
    int x_offset )
{
    int lh = image.fontPointsize();
    if ( int(image.rows()) < 18+lh ) return;
    int y_offset = image.rows()-12-lh;

    DEBUG("Writing scale bar at " << x_offset << " " << image.rows()-12-lh << " down to " 
        << x_offset+width << " and " << y_offset+5);
    image.draw( Magick::DrawableRectangle( 
            x_offset, y_offset, x_offset+width, y_offset+5 ) );

    DEBUG("Writing scale bar annotation");
    image.annotate(
         SIize(meters_per_pixel * width) + "m", 
            Magick::Geometry(width, lh, x_offset, y_offset+10),
            Magick::CenterGravity );
    DEBUG("Wrote scale bar annotation");
}
#endif

void wxManager::store_image(
    std::string filename,
    const Change& image )
{
    DEBUG("Storing image");
    if ( !image.do_resize || !image.do_clear )
        throw std::logic_error("No complete image given for store_image");
#ifndef HAVE_LIBGRAPHICSMAGICK__
    throw std::runtime_error("Cannot save images: Magick library not used in compilation");
#else
    DEBUG("Image to store has width " << image.resize_image.width << " and height " << image.resize_image.height
        <<" and key size " << image.change_key.size());
    int width = image.resize_image.width; 
    int main_height = image.resize_image.height;
    int total_height = main_height;
    int border_after_image = 10;

    Color bg = image.clear_image.background;
    Magick::ColorRGB background 
        ( bg.red()/255.0, bg.green()/255.0, bg.blue()/255.0 ),
                     foreground 
        ( 1.0 - background.red(), 1.0 - background.green(), 
          1.0 - background.blue() );

    std::auto_ptr< Magick::Image > key_img;
    if ( image.change_key.size() ) {
        key_img = make_key_image( 
            width, foreground, background, image.change_key );
        total_height += border_after_image + key_img->rows();
    }

    DEBUG("Creating image sized " << width << " by " << total_height);
    Magick::Image img( Magick::Geometry(width, total_height), background );
    img.type(Magick::TrueColorType);
    img.strokeColor( foreground );
    img.fillColor( foreground );

    write_main_image( img, width, image.image_change, image.change_pixels );
    if ( key_img.get() != NULL )
        img.composite( *key_img, 0, main_height, Magick::OverCompositeOp );
    int scale_bar_width = std::min( width/3, 100 );
    write_scale_bar( img, image.resize_image.pixel_size,
                     scale_bar_width, std::max(0, width-scale_bar_width-5 ) );
    DEBUG("Wrote scale bar");
    
    img.resolutionUnits( Magick::PixelsPerCentimeterResolution );
    unsigned int pix_per_cm = int( 0.01 / image.resize_image.pixel_size );
    img.density(Magick::Geometry(pix_per_cm, pix_per_cm));
    img.write( filename );

#endif
}

}
}