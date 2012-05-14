#ifndef DSTORM_VIEWER_COLOURSCHEME_H
#define DSTORM_VIEWER_COLOURSCHEME_H

#include <simparm/Node.hh>
#include <memory>
#include "Backend.h"
#include <dStorm/make_clone_allocator.hpp>

namespace dStorm {
namespace viewer {

struct ColourScheme {
    virtual ~ColourScheme() {}
    virtual ColourScheme* clone() const = 0;
    virtual simparm::Node& getNode() = 0;
    const simparm::Node& getNode() const { return const_cast<ColourScheme&>(*this).getNode(); }

    template <typename Scheme>
    static std::auto_ptr<ColourScheme> config_for();

    virtual std::auto_ptr<Backend> make_backend( Config&, Status& ) const = 0; 
    virtual void add_listener( simparm::Listener& ) {}
};

}
}

DSTORM_MAKE_BOOST_CLONE_ALLOCATOR(dStorm::viewer::ColourScheme)

#endif
