#ifndef DSTORM_BASETRANSMISSIONCONFIGS
#define DSTORM_BASETRANSMISSIONCONFIGS

#include <simparm/TriggerEntry.hh>
#include "OutputSource.h"
#include <map>
#include <boost/utility.hpp>

namespace simparm {
    template <typename Type> class NodeChoiceEntry;
};

namespace dStorm {
namespace output {

class SourceFactory;

/** The FilterSource class is the base class for all
 *  OutputSource objects which have other outputs as
 *  output. For example, see the LocalizationFilter. 
 *
 *  The FilterSource is built around an SourceFactory, which is added
 *  to its simparm::Node *  element and delivers the OutputSource child
 *  object. To avoid initialization loops, this factory is not constructed
 *  in the normal constructor, but rather via initialize() or in the
 *  copy constructor.
 *
 *  This class will transparently add a Crankshaft transmission to the
 *  output when multiple outputs are given. If this is the
 *  case, it has to distinguish between several config items
 *  with the same name, for example multiple TableOutput configs.
 *  It does so by packing each node into a transparent node called
 *  CrankshaftNodeX, where X is a unique integer.
 *
 *  The important methods and members are output, which is the output
 *  transmission that should be used by deriving classes, and add(),
 *  which can be used to explicitly set the output member. 
 **/
class FilterSource
: public OutputSource,
  public simparm::Node::Callback,
  boost::noncopyable
{
  private:
    /** The unique integer X for the next disambiguation node. */
    int next_identity;

    /** The basename is saved for freshly constructed entries. */
    Basename basename;

    void registerNamedEntries();

    /** The factory is the source object for a new transmission source.
     *  This new transmission source is fetched via 
     *  factory->make_transmission().
     *
     *  If the initialization is delayed via OnCopy, the factory member
     *  will not be set until initialize() is called. This avoids init
     *  loops. */
    std::auto_ptr<SourceFactory> factory;

    /** The target transmissions for this forwarding source. */
    typedef std::list<OutputSource*> Outputs;
    Outputs outputs;

    /** Control elements for adding/removing transmissions. */
    struct RemovalObject;
    std::auto_ptr< simparm::NodeChoiceEntry<RemovalObject> >
        removeSelector;
    std::map< OutputSource*, RemovalObject* > removalObjects;

    /** \return true after initialize() was called. */
    bool is_initialized() const { return factory.get() != NULL; }
    /** \see simparm::Node::Callback::operator() */
    void operator()(const simparm::Event&);
    /** Construct the disambiguation node for the given transmission,
     *  add the remover entry and insert it into our config node. */
    void link_transmission( OutputSource* src );

  protected:
    FilterSource(simparm::Node&);
    FilterSource(simparm::Node&, const FilterSource& o);

    SourceFactory* getFactory() { return factory.get(); }
    const SourceFactory* getFactory() const { return factory.get(); }

  public:
    ~FilterSource();
    FilterSource* clone() const = 0;

    /** This method triggers the delayed initialization of the factory
     *  element. */
    virtual void set_output_factory(const SourceFactory& f);

    virtual void set_source_capabilities( Capabilities );

    /** Explicitely set the output element. Circumvents the 
     *  \c factory. */
    void add(std::auto_ptr<OutputSource> src);
    /** Convenience wrapper for the auto_ptr-add */
    void add(OutputSource *src)
        { add( std::auto_ptr<OutputSource>(src) ); }
    /** Will remove the given OutputSource from this config and
     *  deletes it. Removes the source from the selector as well and
     *  deletes the disambiguation node. */
    void remove( OutputSource& src );

    /** \see dStorm::OutputSource */
    void set_output_file_basename(const Basename& basename);

    typedef Outputs::iterator iterator;
    typedef Outputs::const_iterator const_iterator;

    iterator begin() { return outputs.begin(); }
    iterator end() { return outputs.end(); }
    const_iterator begin() const { return outputs.begin(); }
    const_iterator end() const { return outputs.end(); }

    std::auto_ptr<Output> make_output(); 
};

typedef FilterSource TransmissionSource;

}
}

#endif
