#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <fstream>
#include <cassert>
#include <sstream>

#include <boost/smart_ptr/make_shared.hpp>
#include <dStorm/Engine.h>
#include <dStorm/input/Forwarder.h>
#include <dStorm/input/InputMutex.h>
#include <dStorm/input/MetaInfo.h>
#include <dStorm/output/Basename.h>
#include <dStorm/output/FilterSource.h>
#include <dStorm/output/SourceFactory.h>
#include <dStorm/signals/UseSpotFinder.h>
#include <dStorm/signals/UseSpotFitter.h>
#include <simparm/Menu.h>
#include <simparm/text_stream/RootNode.h>
#include <simparm/TreeRoot.h>
#include <simparm/TreeEntry.h>
#include <ui/serialization/Node.h>

#include "Car.h"
#include "Config.h"
#include "debug.h"
#include "ModuleLoader.h"

using namespace std;

namespace dStorm {
namespace job {

class Config::TreeRoot : public output::FilterSource
{
    simparm::TreeRoot tree_root;
    simparm::TreeObject name_object;
    output::Config* my_config;
    output::Capabilities cap;

    std::string getName() const { return name_object.getName(); }
    std::string getDesc() const { return name_object.getDesc(); }
    void attach_ui( simparm::NodeHandle ) { throw std::logic_error("Not implemented on tree base"); }

  public:
    TreeRoot();
    TreeRoot( const TreeRoot& other )
    : output::FilterSource( other),
      tree_root(other.tree_root),
      name_object(other.name_object)
    {
        DEBUG("Copying output tree root");
        this->set_output_factory( *other.my_config );
        my_config = dynamic_cast<output::Config*>(getFactory());
    }
    ~TreeRoot() {
        DEBUG("Destroying output tree root");
    }

    TreeRoot* clone() const { return new TreeRoot(*this); }
    output::Config &root_factory() { return *my_config; }

    void set_trace_capability( const input::Traits<output::LocalizedImage>& t ) {
        cap.set_source_image( t.source_image_is_set );
        cap.set_smoothed_image( t.smoothed_image_is_set );
        cap.set_candidate_tree( t.candidate_tree_is_set );
        cap.set_cluster_sources( ! t.source_traits.empty() );
        this->set_source_capabilities( cap );
    }

    void attach_full_ui( simparm::NodeHandle at ) { 
        simparm::NodeHandle n = tree_root.attach_ui(at);
        simparm::NodeHandle r = name_object.attach_ui(n);
        attach_children_ui( r ); 
    }
    void hide_in_tree() { name_object.show_in_tree = false; }
};

Config::TreeRoot::TreeRoot()
: output::FilterSource(),
  name_object("EngineOutput", "dSTORM engine output"),
  cap( output::Capabilities()
            .set_source_image()
            .set_smoothed_image()
            .set_candidate_tree()
            .set_input_buffer() )
{
    DEBUG("Building output tree root node at " << 
            &static_cast<Node&>(*this) );
    {
        output::Config exemplar;
        DEBUG("Setting output factory from exemplar t  " << static_cast<output::SourceFactory*>(&exemplar) << " in config at " << &exemplar);
        this->set_output_factory( exemplar );
        DEBUG("Destructing exemplar config");
    }
    DEBUG("Setting source capabilities");
    this->set_source_capabilities( cap );

    DEBUG("Downcasting own config handle");
    assert( getFactory() != NULL );
    my_config = dynamic_cast<output::Config*>(getFactory());
    assert( my_config != NULL );
    DEBUG("Finished building output tree node");
}

Config::Config() 
: car_config("Car", "Job options"),
  outputRoot( new TreeRoot() ),
  outputSource(*outputRoot),
  outputConfig(outputRoot->root_factory()),
  helpMenu( "HelpMenu", "Help" ),
  outputBox("Output", "Output options"),
  configTarget("SaveConfigFile", "Store config used in computation in", "-settings.txt"),
  auto_terminate("AutoTerminate", "Automatically terminate finished jobs",
                 true),
  pistonCount("CPUNumber", "Number of CPUs to use")
{
    configTarget.set_user_level(simparm::Beginner);
    auto_terminate.set_user_level(simparm::Expert);

    pistonCount.set_user_level(simparm::Expert);
    pistonCount.setHelpID( "#CPUNumber" );
    pistonCount.setHelp("Use this many parallel threads to compute the "
                        "STORM result. If you notice a low CPU usage, "
                        "raise this value to the number of cores you "
                        "have.");
#if defined(_SC_NPROCESSORS_ONLN)
    int pn = sysconf(_SC_NPROCESSORS_ONLN);
    pistonCount = (pn == 0) ? 8 : pn;
#elif defined(HAVE_WINDOWS_H)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    pistonCount = info.dwNumberOfProcessors;
#else
    pistonCount.set_user_level(simparm::Beginner);
    pistonCount = 8;
#endif

    add_modules( *this );
    input->publish_meta_info();
}

Config::Config(const Config &c) 
: car_config(c.car_config),
  outputRoot(c.outputRoot->clone()),
  outputSource(*outputRoot),
  outputConfig(outputRoot->root_factory()),
  helpMenu( c.helpMenu ),
  outputBox(c.outputBox),
  configTarget(c.configTarget),
  auto_terminate(c.auto_terminate),
  pistonCount(c.pistonCount)
{
    if ( c.input.get() )
        create_input( std::auto_ptr<input::Link>(c.input->clone()) );
    input->publish_meta_info();
    DEBUG("Copied Car config");
}

Config::~Config() {
    dStorm::input::InputMutexGuard lock( input::global_mutex() );
    outputRoot.reset( NULL );
    input_listener.reset();
    input.reset();
}

void Config::create_input( std::auto_ptr<input::Link> p ) {
    input_listener = p->notify( boost::bind(&Config::traits_changed, this, _1) );
    input = p;
}

simparm::NodeHandle Config::attach_ui( simparm::NodeHandle at ) {
   DEBUG("Registering named entries of CarConfig with " << size() << " elements before registering");
   current_ui = car_config.attach_ui ( at );
   attach_children_ui( current_ui );
   return current_ui;
}

void Config::attach_children_ui( simparm::NodeHandle at ) {
   input->registerNamedEntries( at );
   pistonCount.attach_ui(  at  );
   simparm::NodeHandle b = outputBox.attach_ui( at );
   outputRoot->attach_full_ui( b );
   configTarget.attach_ui( b );
   auto_terminate.attach_ui(  at  );
   DEBUG("Registered named entries of CarConfig with " << size() << " elements after registering");
}

void Config::add_spot_finder( std::auto_ptr<engine::spot_finder::Factory> finder) {
    input->publish_meta_info();
    input->current_meta_info()->get_signal< signals::UseSpotFinder >()( *finder );
}

void Config::add_spot_fitter( std::auto_ptr<engine::spot_fitter::Factory> fitter) {
    input->publish_meta_info();
    input->current_meta_info()->get_signal< signals::UseSpotFitter >()( *fitter );
}

void Config::add_input( std::auto_ptr<input::Link> l, InsertionPlace p) {
    if ( input.get() )
        input->insert_new_node( l, p );
    else
        create_input( l );
}

void Config::add_output( std::auto_ptr<output::OutputSource> o ) {
    outputConfig.addChoice( o.release() );
}

std::auto_ptr<input::BaseSource> Config::makeSource() const {
    return input->make_source();
}

const input::MetaInfo&
Config::get_meta_info() const {
    return *input->current_meta_info();
}

void Config::traits_changed( boost::shared_ptr<const input::MetaInfo> traits ) {
    DEBUG("Basename declared in traits is " << traits->suggested_output_basename );
    configTarget.set_output_file_basename( traits->suggested_output_basename );
    outputRoot->set_output_file_basename( traits->suggested_output_basename );
    if ( traits->provides<output::LocalizedImage>() ) 
        outputRoot->set_trace_capability( *traits->traits<output::LocalizedImage>() );
}

std::auto_ptr< Job > Config::make_job() {
    return std::auto_ptr< Job >( new Car(*this) );
}

}
}
