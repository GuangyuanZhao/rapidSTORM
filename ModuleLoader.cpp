#include <dStorm/engine/SpotFinder.h>
#include <dStorm/engine/SpotFitterFactory.h>
#include "ModuleLoader.h"
#include <boost/ptr_container/ptr_list.hpp>

#include "inputs/inputs.h"
#include "spotFinders/spotFinders.h"
#include "gauss_fitter/Factory.h"
#include <simparm/ChoiceEntry_Impl.hh>
#include "outputs/BasicTransmissions.h"
#include <dStorm/Config.h>
#include "local_cleanup.h"
#include <dStorm/error_handler.h>
#include "engine/ChainLink_decl.h"
#include "engine_stm/ChainLink_decl.h"
#include "noop_engine/ChainLink_decl.h"

#include <dStorm/helpers/DisplayManager.h>
#include "wxDisplay/wxManager.h"
#include "LibraryHandle.h"

#include <dStorm/plugindir.h>

#include "debug.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace dStorm {

struct LibLT_Handler {
    LibLT_Handler() { lt_dlinit(); }
    ~LibLT_Handler() { lt_dlexit(); }
};

struct ModuleLoader::Pimpl
: public JobMaster 
{
    LibLT_Handler libLT_Handler;
    typedef boost::ptr_list<LibraryHandle> List;
    List lib_handles;
    std::set<std::string> loaded;

    std::auto_ptr<Display::Manager> display;
    std::list<Job*> jobs;

    Pimpl();
    ~Pimpl();

    void load_plugins();
    enum LoadResult { Loaded, Failure };
    LoadResult try_loading_module( const char *filename );
    static int lt_dlforeachfile_callback( 
        const char *filename, void* data );
    std::string makeProgramDescription();

    void register_node( Job& j ) { jobs.push_back(&j); }
    void erase_node( Job& j ) { jobs.remove(&j); }
};

ModuleLoader::ModuleLoader()
: pimpl( new Pimpl() ) {}

ModuleLoader::Pimpl::Pimpl()
: display( new Display::wxManager() )
{
    load_plugins();
}

ModuleLoader::Pimpl::~Pimpl() {}

ModuleLoader::Pimpl::LoadResult
ModuleLoader::Pimpl::try_loading_module( const char *filename ) {
    try {
        std::auto_ptr<LibraryHandle> handle
            ( new LibraryHandle( filename ) );
        for (List::const_iterator i = lib_handles.begin(); 
             i !=lib_handles.end(); i++)
            if ( *i == *handle ) {
                DEBUG(filename  << " is a duplicate hit");
                return Failure;
            }
        DEBUG("Requesting plugin display driver from " 
              << filename);
        DEBUG("Display driver is before " << display.get() );
        handle->replace_display(display);
        DEBUG("Display dirver is now " << display.get() );
        lib_handles.push_back( handle.release() );
    } catch( const std::exception& e ) {
        std::cerr << "Unable to load plugin '" << filename << "': " <<
            e.what() << "\n";
        return Failure;
    }
    loaded.insert( std::string(filename) );
    return Loaded;
}

int ModuleLoader::Pimpl::lt_dlforeachfile_callback 
    ( const char *filename, void* data )
{
    DEBUG("Considering name " << filename);
    ModuleLoader::Pimpl &m = *(ModuleLoader::Pimpl*)data;
    if ( m.loaded.find( filename ) != m.loaded.end() )
        return 0;
    else {
        DEBUG("Trying to load plugin " << filename);
        m.try_loading_module( filename );
        return 0;
    }
}

void ModuleLoader::Pimpl::load_plugins()
{
    std::string compiled_plugin_dir = plugin_directory();
    const char *plugin_dir = compiled_plugin_dir.c_str();
    char *env_plugin_dir = getenv("RAPIDSTORM_PLUGINDIR");
    if ( env_plugin_dir != NULL )
        plugin_dir = env_plugin_dir;

    DEBUG("Finding plugins in " << plugin_dir);
    lt_dlforeachfile( plugin_dir, lt_dlforeachfile_callback, this );
    DEBUG("Found plugins");

    Display::Manager::setSingleton( *display );
}

void ModuleLoader::add_modules
    ( dStorm::Config& car_config )
{
    DEBUG("Adding basic input modules");
    dStorm::basic_inputs( &car_config.inputConfig );
    DEBUG("Adding rapidSTORM engine");
    car_config.add_engine( engine::make_rapidSTORM_engine_link() );
    car_config.add_engine( engine_stm::make_STM_engine_link() );
    car_config.add_engine( noop_engine::makeLink() );
    DEBUG("Adding basic spot finders");
    car_config.add_spot_finder( spotFinders::make_Spalttiefpass() );
    car_config.add_spot_finder( spotFinders::make_Median() );
    car_config.add_spot_finder( spotFinders::make_Erosion() );
    car_config.add_spot_finder( spotFinders::make_Gaussian() );
    DEBUG("Adding basic spot fitter");
    car_config.add_spot_fitter( new gauss_2d_fitter::Factory() );
    DEBUG("Adding basic output modules");
    dStorm::output::basic_outputs( &car_config.outputConfig );

    DEBUG("Iterating plugins");
    for ( Pimpl::List::iterator i = pimpl->lib_handles.begin(); i != pimpl->lib_handles.end();
          i++)
    {
        DEBUG("Adding plugin's modules");
        (*i) ( &car_config );
    }
}

std::string ModuleLoader::makeProgramDescription() {
    return pimpl->makeProgramDescription();
}

std::string ModuleLoader::Pimpl::makeProgramDescription() {
    std::stringstream ss;
    ss << PACKAGE_STRING;
    for ( List::iterator i = lib_handles.begin();
                            i != lib_handles.end(); i++)
    {
        List::iterator end_test = i; ++end_test;
        if ( i == lib_handles.begin() )
            ss << " with ";
        else if ( end_test == lib_handles.end() )
            ss << " and ";
        else 
            ss << ", ";
        ss << i->getDesc();
    }
    return ss.str();
}

struct EmptyJobMaster : public JobMaster {
    ost::Mutex mutex;
    ost::Condition zero;
    int count;
    EmptyJobMaster() : zero(mutex), count(0) {}

    void register_node( Job& ) { ost::MutexLock lock(mutex); count++;}
    void erase_node( Job&  ) { 
        ost::MutexLock lock(mutex);
        count--;
        if (count == 0) zero.signal();
    }

    ~EmptyJobMaster() { 
        ost::MutexLock lock(mutex);
        while (count) 
            zero.wait();
    }
};

void ModuleLoader::do_panic_processing( int argc, char *argv[] ) 
{
    std::auto_ptr<JobMaster> master( new EmptyJobMaster() );

    dStorm::ErrorHandler::CleanupArgs args;
    for (int i = 0; i < argc; i++) args.push_back(argv[i]);

    while ( ! args.empty() ) {
        size_t size_before = args.size();
        local_cleanup( args, master );
        for ( Pimpl::List::iterator i = pimpl->lib_handles.begin(); i != pimpl->lib_handles.end();
            i++)
        {
            if ( args.empty() ) break;
            i->getCleanup() ( &args, master.get() );
        }
        if ( args.size() == size_before ) {
            args.pop_front();
        }
    }
}

static ModuleLoader *ml = NULL;

void ModuleLoader::makeSingleton() 
    { ml = new ModuleLoader(); }
ModuleLoader& ModuleLoader::getSingleton() {
    if ( ml != NULL )
        return *ml; 
    else 
        throw std::logic_error("ModuleLoader used before "
                               "initialization");
}
void ModuleLoader::destroySingleton()
    { if ( ml != NULL ) { delete ml; ml = NULL; } }

void ModuleLoader::add_jobs( JobMaster& master ) {
    for ( std::list<Job*>::iterator i = pimpl->jobs.begin(); i != pimpl->jobs.end(); i++ ) {
        master.register_node( **i );
    }
}

}
