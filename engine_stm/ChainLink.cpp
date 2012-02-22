#include "debug.h"

#include "ChainLink.h"
#include "LocalizationBuncher.h"
#include <dStorm/input/MetaInfo.h>
#include <dStorm/input/Method.hpp>
#include <dStorm/output/LocalizedImage_traits.h>

#include <boost/mpl/vector.hpp>
#include <dStorm/output/LocalizedImage_decl.h>
#include <dStorm/localization/record.h>
#include <dStorm/Localization_decl.h>
#include <dStorm/input/Source.h>
#include <dStorm/output/LocalizedImage.h>
#include <simparm/ChoiceEntry_Impl.hh>

namespace dStorm {
namespace engine_stm {

using namespace input;

class ChainLink : public input::Method< ChainLink >
{
    friend class input::Method< ChainLink >;
    simparm::Node& getNode() { throw std::logic_error("Not implemented"); }

    typedef boost::mpl::vector<localization::Record,dStorm::Localization>
        SupportedTypes;
    template <typename Type>
    bool changes_traits( const MetaInfo&, const Traits<Type>& )
        { return true; }
    template <typename Type>
    BaseTraits* create_traits( MetaInfo&, const Traits<Type>& p ) 
        { return new Traits<output::LocalizedImage>(p, "STM", "Localizations file"); }

    template <typename Type>
    input::BaseSource* make_source( std::auto_ptr< input::Source<Type> > p ) 
        { return new Source<Type>( p ); }

  public:
    std::string name() const { return Forwarder::name(); }
    std::string description() const { return Forwarder::description(); }
    void registerNamedEntries( simparm::Node& node ) 
        { Forwarder::registerNamedEntries(node); }
};

class STMEngine : public input::Forwarder
{
    void traits_changed( TraitsRef orig, Link* ) {
        if ( orig.get() )
            if ( orig->provides< output::LocalizedImage >() )
                this->update_current_meta_info( orig );
            else {
                boost::shared_ptr<input::MetaInfo> my_traits( new input::MetaInfo(*orig) );
                my_traits->set_traits( NULL );
                this->update_current_meta_info( my_traits );
            }
        else
            this->update_current_meta_info( orig );
    }
    STMEngine* clone() const { return new STMEngine(*this); }
    void registerNamedEntries( simparm::Node& at ) {
        input::Forwarder::registerNamedEntries( at );
        at.push_back( std::auto_ptr<simparm::Node>( new simparm::Object(name(), description()) ) );
    }

  public:
    std::string name() const { return "LocalizationBuncher"; }
    std::string description() const { return "Replay stored localizations"; }
};

std::auto_ptr<input::Link>
make_localization_buncher()
{
    return std::auto_ptr<input::Link>( new ChainLink( ) );
}

std::auto_ptr<input::Link>
make_STM_engine_link()
{
    return std::auto_ptr<input::Link>( new STMEngine( ) );
}

}
}
