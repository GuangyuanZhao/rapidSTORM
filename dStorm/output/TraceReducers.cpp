#include "TraceReducer.h"

namespace dStorm {
namespace output {

class AveragingReducer : public TraceReducer {
  public:
    void reduce_trace_to_localization 
        (const Trace& from, Localization *to,
         const Eigen::Vector2d& shift) 
    {
        double x_position = 0, y_position = 0;
        int n = 0;
        int last_image_number = 0;
        double total_amplitude = 0;
        double max_parab = 0;

        for ( data_cpp::Vector<Localization>::const_iterator
                i = from.begin(); i != from.end(); i++)
        {
            x_position += i->x();
            y_position += i->y();
            n += 1;
            last_image_number = 
                std::max( last_image_number, i->getImageNumber() );
            total_amplitude += i->getStrength();
            max_parab = std::max(i->parabolicity(), max_parab);
        }

        if ( n == 0 ) n = 1;
        new(to) Localization( 
            x_position / n - shift.x(),
            y_position / n - shift.y(),
            last_image_number, total_amplitude, &from, max_parab );
    }
};

class TraceReducer::Config::Implementation {
  public:
    Implementation() 
    {}
    Implementation* clone() const { return new Implementation(*this); }

    virtual std::auto_ptr<TraceReducer> make_trace_reducer()
        { return std::auto_ptr<TraceReducer>( new AveragingReducer() ); }
};

TraceReducer::Config::Config() 
: simparm::Object("TraceReducerMaker", ""),
  implementation( new Implementation() )
{}

TraceReducer::Config::Config(const Config& c) 
: simparm::Node(c), simparm::Object(c),
  implementation( c.implementation->clone() )
{}

TraceReducer::Config::~Config()
{}

std::auto_ptr<TraceReducer> 
TraceReducer::Config::make_trace_reducer() const 
{
    return std::auto_ptr<TraceReducer>(new AveragingReducer());
}

}
}