#ifndef DSTORM_ENGINE_SPOTFITTERFACTORY_H
#define DSTORM_ENGINE_SPOTFITTERFACTORY_H

#include "JobInfo_decl.h"
#include "SpotFitter_decl.h"
#include "Image.h"
#include <simparm/Node_decl.hh>
#include <simparm/Callback.hh>
#include <memory>
#include "../output/Traits_decl.h"
#include "../output/Basename_decl.h"

namespace dStorm {
namespace engine {
namespace spot_fitter {

struct Factory {
    simparm::Node& node;
  public:
    Factory(simparm::Node& node) : node(node) {}
    simparm::Node& getNode() { return node; }
    operator simparm::Node&() { return node; }
    const simparm::Node& getNode() const { return node; }
    operator const simparm::Node&() const { return node; }

    virtual Factory* clone() const = 0;
    virtual ~Factory();
    virtual std::auto_ptr<Implementation> make( const JobInfo& ) = 0;
    virtual void set_requirements( input::Traits<engine::Image>& ) = 0;
    virtual void set_traits( output::Traits&, const JobInfo& ) = 0;
    virtual void set_variables( output::Basename& ) const {}

    virtual void register_trait_changing_nodes( simparm::Listener& ) = 0;
    /** Check whether spot fitter objects could actually be built with the current configuration. */
    virtual void check_configuration( const input::Traits<engine::Image>&, const output::Traits& ) {}
};

}
}
}

#endif