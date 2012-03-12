#ifndef DSTORM_EXPRESSION_SIMPLEFILTERS_H
#define DSTORM_EXPRESSION_SIMPLEFILTERS_H

#include <simparm/Eigen_decl.hh>
#include <simparm/BoostUnits.hh>
#include <boost/units/Eigen/Core>
#include <simparm/Eigen.hh>
#include <simparm/BoostOptional.hh>
#include <simparm/Entry.hh>
#include "CommandLine.h"
#include <dStorm/units/nanolength.h>
#include <dStorm/localization/Traits.h>

namespace dStorm {
namespace expression {

struct SimpleFilters 
: public simparm::Listener
{
    SimpleFilters();
    SimpleFilters(const SimpleFilters&);
    void set_manager( config::ExpressionManager * manager );
    void set_visibility( const input::Traits<Localization>& );

    typedef boost::units::divide_typeof_helper< 
        boost::units::power10< boost::units::si::length, -12 >::type,
        boost::units::camera::time 
    >::type ShiftSpeed;
  private:
    config::ExpressionManager * manager;
    simparm::Entry< boost::optional< boost::units::quantity< boost::units::camera::intensity > > >
        lower_amplitude;
    simparm::Entry< boost::optional< Eigen::Matrix< boost::units::quantity<ShiftSpeed,float>, 3, 1, Eigen::DontAlign> > >
        drift_correction;
    simparm::Entry< float > two_kernel_improvement;
    void operator()(const simparm::Event&);
    void publish_amp();
    void publish_drift_correction();
    void publish_tki();
};

}
}

#endif
