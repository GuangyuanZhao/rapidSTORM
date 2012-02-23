#define BOOST_DISABLE_ASSERTS
#define LOCPREC_NOISESOURCE_CPP

#include <dStorm/debug.h>
#include "NoiseSource.h"
#include <time.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>
#include <cassert>
#include <dStorm/engine/Image.h>
#include <fstream>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>
#include <dStorm/Image_impl.h>
#include <dStorm/input/Source.h>
#include <dStorm/input/MetaInfo.h>
#include <dStorm/engine/InputTraits.h>
#include <boost/units/Eigen/Array>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/foreach.hpp>

#include "FluorophoreDistributions.h"

using namespace boost::units;

namespace locprec {

void FluorophoreSetConfig::registerNamedEntries() {
    this->push_back( fluorophoreConfig );
    this->push_back( distribution );
    this->push_back( store );
    this->push_back( recall );
    this->push_back( fluorophore_index );
}

void NoiseConfig::registerNamedEntries() {
    this->receive_changes_from( newSet.value );
    this->receive_changes_from( layer_count.value );
    receive_changes_from_subtree( optics ); 

    this->push_back( noiseGeneratorConfig );
    this->push_back( layer_count );
    optics.registerNamedEntries();
    this->push_back( optics );
    this->push_back( newSet );
    this->push_back( imageNumber );
    this->push_back( integrationTime );
    this->push_back( saveActivity );
}

#define NAME "Generated"
#define DESC "Randomly generated values"

FluorophoreSetConfig::FluorophoreSetConfig(std::string name, std::string desc)
: simparm::Set(name, desc),
  distribution( "FluorophoreDistribution", "Pattern to "
                           "distribute simulated fluorophores with" ),
  store("StoreFluorophores", "Store fluorophore positions and PSFs" ),
  recall("RecallFluorophores", "Recall fluorophore positions and PSFs" ),
  fluorophore_index("FluorophoreIndex", "Fluorophore ident for transmission", 0)
{ 
    distribution.addChoice( new FluorophoreDistributions::Random());
    distribution.addChoice( FluorophoreDistributions::make_lattice());
    distribution.addChoice( new FluorophoreDistributions::Lines());

    registerNamedEntries();
}

FluorophoreSetConfig::FluorophoreSetConfig(const FluorophoreSetConfig& o)
: simparm::Set(o),
  fluorophoreConfig(o.fluorophoreConfig),
  distribution(o.distribution),
  store(o.store),
  recall(o.recall),
  fluorophore_index(o.fluorophore_index)
{
    registerNamedEntries();
}

NoiseConfig::NoiseConfig()
: simparm::Object(NAME, DESC),
  simparm::TreeListener(),
  next_fluo_id(1),
  newSet("NewFluorophoreSet", "Add fluorophore set"),
  imageNumber("ImageNumber", "Number of source images to generate", 10000),
  integrationTime("IntegrationTime", "Integration time for one image", 0.1),
  saveActivity( "SaveActivity", "Save fluorophore activity information to file" ),
  layer_count( "LayerCount", "Number of layers to generate", 1 )
{
    create_fluo_set();
    registerNamedEntries();
    optics.set_number_of_fluorophores(2);
}

NoiseConfig::NoiseConfig( const NoiseConfig & cp )
: simparm::Object(cp),
  dStorm::input::Terminus(cp),
  simparm::TreeListener(),
  next_fluo_id(cp.next_fluo_id),
  noiseGeneratorConfig(cp.noiseGeneratorConfig),
  newSet(cp.newSet),
  imageNumber(cp.imageNumber),
  integrationTime(cp.integrationTime),
  saveActivity(cp.saveActivity),
  layer_count(cp.layer_count),
  optics(cp.optics)
{
    for ( FluoSets::const_iterator i = cp.fluorophore_sets.begin();
                                   i != cp.fluorophore_sets.end(); ++i)
        add_fluo_set( std::auto_ptr<FluorophoreSetConfig>(
            new FluorophoreSetConfig(**i) ) );
    registerNamedEntries();
}

void NoiseConfig::operator()( const simparm::Event& e)
{
    if ( newSet.triggered() ) {
        create_fluo_set();
        newSet.untrigger();
    } else if ( e.cause == simparm::Event::ValueChanged ) {
        if ( &e.source == &layer_count.value )
            optics.set_number_of_planes( layer_count() );
        publish_meta_info();
    } else 
	TreeListener::add_new_children(e);
}

void NoiseConfig::create_fluo_set()
{
    std::stringstream id;
    id << next_fluo_id++;
    add_fluo_set( std::auto_ptr<FluorophoreSetConfig>
        ( new FluorophoreSetConfig("FluorophoreSet" + id.str(),
                                   "Fluorpohore set " + id.str()) ));
}

void NoiseConfig::add_fluo_set( std::auto_ptr<FluorophoreSetConfig> s )
{
    fluorophore_sets.push_back( s.get() );
    this->simparm::Node::push_back( std::auto_ptr<simparm::Node>(s) );
}

std::auto_ptr< boost::ptr_list<Fluorophore> >
FluorophoreSetConfig::create_fluorophores(
    const dStorm::engine::InputTraits& t,
    gsl_rng *rng,
    int imN ) const
{
    std::auto_ptr< boost::ptr_list<Fluorophore> > fluorophores
        ( new boost::ptr_list<Fluorophore>() );
    if ( recall ) {
        simparm::FileEntry opener( recall );
        std::istream& in = opener.get_input_stream();
        int number;
        in >> number;

        for (int i = 0; i < number; i++)
            fluorophores->push_back( new Fluorophore( in,
                fluorophoreConfig ) );
    } else {
        const FluorophoreDistribution& distribution = this->distribution.value();
        FluorophoreDistribution::Positions positions = 
            distribution.fluorophore_positions( t.size_in_sample_space(), rng);

        int bins = 100;
        dStorm::Image<Fluorophore*,2>::Size sz;
        sz.x() = sz.y() = (bins+1) * camera::pixel;
        dStorm::Image<Fluorophore*,2> cache(sz);
        cache.fill(0);

        while ( ! positions.empty() ) {
            fluorophores->push_back( new Fluorophore(positions.front(), imN, fluorophoreConfig, t, fluorophore_index()) );
            positions.pop();
        }
    }

    if ( store ) {
        simparm::FileEntry opener( store );
        std::ostream& out = opener.get_output_stream();
        out << fluorophores->size() << "\n";
        boost::ptr_list<Fluorophore>::const_iterator i;
        for ( i = fluorophores->begin(); i != fluorophores->end(); ++i )
            out << *i << "\n";
    }

    return fluorophores;
}

NoiseSource::NoiseSource( NoiseConfig &config )
: simparm::Set("NoiseSource", "Noise source status"),
  randomSeedEntry(config.noiseGeneratorConfig.random_seed),
  t( new dStorm::engine::InputTraits() )
{

    DEBUG("Just made traits for noise source");
    rng = gsl_rng_alloc(gsl_rng_mt19937);
    DEBUG("Using random seed " << randomSeedEntry());
    gsl_rng_set(rng, randomSeedEntry() );
    noiseGenerator = 
        NoiseGenerator<unsigned short>::factory(config.noiseGeneratorConfig, rng);

    for (int p = 0; p < config.optics.number_of_planes(); ++p) {
        dStorm::image::MetaInfo<2> size;
        size.size.x() = config.noiseGeneratorConfig.width() * camera::pixel;
        size.size.y() = config.noiseGeneratorConfig.height() * camera::pixel;
        t->push_back( size, dStorm::traits::Optics() );
    }
    config.optics.set_traits( *t );
    for (int p = 0; p < t->plane_count(); ++p) {
        t->plane(p).create_projection();
    }
    imN = config.imageNumber();
    integration_time = config.integrationTime() * si::seconds;

    if ( config.saveActivity )
        output.reset( new std::ofstream
            ( config.saveActivity().c_str() ));

    for ( NoiseConfig::FluoSets::const_iterator
            i = config.get_fluorophore_sets().begin();
            i != config.get_fluorophore_sets().end(); ++i)
    {
        std::auto_ptr<FluorophoreList> l = 
            (*i)->create_fluorophores( *t, rng, imN );
        fluorophores.transfer( fluorophores.end(), *l );
    }

}

NoiseSource::~NoiseSource()

{
    /* The last value of our random number generator is the next
     * seed. */
    randomSeedEntry = gsl_rng_get(rng);
    gsl_rng_free(rng);
}

dStorm::engine::ImageStack* NoiseSource::fetch( int imNum )
{
    boost::lock_guard<boost::mutex> lock(mutex);
    std::auto_ptr<dStorm::engine::ImageStack>
        result(new dStorm::engine::ImageStack(imNum * camera::frame));

    for ( int p = 0; p < t->plane_count(); ++p ) {
        dStorm::engine::Image2D i( t->image(p).size );
        noiseGenerator->pixelNoise(i.ptr(), i.size_in_pixels());
        result->push_back( i );
    }

    /* Then add the fluorophores. */
    DEBUG("Making glare for " << fluorophores.size() << " fluorophores");
    if ( output.get() ) *output << imNum;
    int index = 0;
    BOOST_FOREACH( Fluorophore& fl, fluorophores )  {
        int photons =
            fl.glareInImage(rng, *result, imNum, integration_time);
        if ( photons > 0 && output.get() ) *output << ", " << index << " " << photons;
        ++index;
    }
    if ( output.get() ) *output << "\n";

    return result.release();
}

class NoiseSource::iterator
: public boost::iterator_facade<iterator,Image,std::input_iterator_tag>
{
    friend class boost::iterator_core_access;
    NoiseSource* const src;
    mutable boost::shared_ptr<Image> img;
    int image_number;
    
    Image& dereference() const { 
        if ( img.get() == NULL ) {
            img.reset( src->fetch(image_number) ); 
        }
        return *img; 
    }
    void increment() { img.reset(); image_number++; }
    bool equal(const iterator& i) const { return image_number == i.image_number; }

  public:
    iterator() : src(NULL), image_number(0) {}
    iterator(NoiseSource& ns, int im) : src(&ns), image_number(im) {}
};

NoiseSource::base_iterator 
NoiseSource::begin() {
    return base_iterator( iterator(*this, 0) );
}

NoiseSource::base_iterator 
NoiseSource::end() {
    return base_iterator( iterator(*this, imN) );
}

NoiseSource::Source::TraitsPtr
NoiseSource::get_traits( typename Source::Wishes ) {
    return t;
}

void NoiseConfig::publish_meta_info() {
    typedef dStorm::input::Traits<Image> Traits;

    boost::shared_ptr< Traits > rv( new Traits() );
    rv->image_number().range().first = 0 * camera::frame;
    rv->image_number().range().second = dStorm::traits::ImageNumber::ValueType
        ::from_value( (imageNumber() - 1) );
    for (int p = 0; p < optics.number_of_planes(); ++p) {
        dStorm::image::MetaInfo<2> p;
        p.size.x() = noiseGeneratorConfig.width() * camera::pixel;
        p.size.y() = noiseGeneratorConfig.height() * camera::pixel;
        rv->push_back( p, dStorm::traits::Optics() );
    }
    this->optics.set_traits( *rv );

    dStorm::input::MetaInfo::Ptr t( new dStorm::input::MetaInfo() );
    t->set_traits( rv );
    update_current_meta_info( t );
}

}
