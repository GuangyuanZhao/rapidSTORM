#include <boost/mpl/for_each.hpp>
#include <Eigen/StdVector>
#include "unit_test.h"
#include "parameters.h"
#include "Zhuang.h"
#include "No3D.h"

namespace dStorm {
namespace guf {
namespace PSF {

MockDataTag::Data
mock_data() {
    MockDataTag::Data disjoint_data;
    for (int i = 0; i < 10; ++i) {
        disjoint_data.data.push_back( MockDataTag::Data::DataRow() );
        MockDataTag::Data::DataRow& d = disjoint_data.data.back();
        d.inputs(0,0) = i * M_PI / 30.0;
        for (int j = 0; j < 12; ++j) {
            d.output[j] = rand() * 1E-5;
            d.logoutput[j] = (d.output[j] < 1E-10)
                ? -23*d.output[j] : d.output[j] * log(d.output[j]);
        }
    }
    disjoint_data.min.fill( boost::units::quantity<dStorm::guf::PSF::LengthUnit>::from_value( -1E50 ) );
    disjoint_data.max.fill( boost::units::quantity<dStorm::guf::PSF::LengthUnit>::from_value( 1E50 ) );
    disjoint_data.pixel_size = boost::units::quantity<dStorm::guf::PSF::AreaUnit>(1E-15 * boost::units::si::meter * boost::units::si::meter);
    for (int j = 0; j < 12; ++j)
        disjoint_data.xs[j] = j * exp(1) / 30.0;
    return disjoint_data;
}

template <typename Expression>
struct RandomParameterSetter {
    Expression m;
    typedef void result_type;

    RandomParameterSetter() {}

    template <int Dim>
    void operator()( dStorm::guf::PSF::BestSigma<Dim> p ) { m(p).set_value( 0.400+ 0.1*Dim ); }
    template <int Dim>
    void operator()( nonlinfit::Xs<Dim,dStorm::guf::PSF::LengthUnit> ) {}
    template <int Dim>
    void operator()( dStorm::guf::PSF::Mean<Dim> p ) { m(p).set_value( 2.500+ 0.050*Dim ); }
    void operator()( dStorm::guf::PSF::MeanZ p ) { m(p).set_value( 0.600 ); }
    template <int Dim>
    void operator()( dStorm::guf::PSF::ZOffset<Dim> p ) { m(p).set_value( 0.200- 0.050*Dim ); }
    void operator()( dStorm::guf::PSF::Amplitude p ) { m(p).set_value( 1E7 ); }
    template <int Dim>
    void operator()( dStorm::guf::PSF::DeltaSigma<Dim> p ) { m(p).set_value( 0.6 + 0.4 * Dim ); }
    void operator()( dStorm::guf::PSF::Prefactor p ) { m(p).set_value( 0.3 ); }
    void operator()( dStorm::guf::PSF::Wavelength p ) { m(p).set_value( 0.5 ); }
    void operator()( dStorm::guf::PSF::ZPosition p ) { m(p).set_value( 1E-2 ); }

    const Expression& operator()() { 
        boost::mpl::for_each< typename Expression::Variables >( boost::ref(*this) );
        return m;
    }
};

template <typename Expression> 
Expression mock_model() { return RandomParameterSetter<Expression>()(); }

template Zhuang mock_model<Zhuang>();
template No3D mock_model<No3D>();

void run_unit_tests( TestState &state ) {
    check_zhuang_evaluator( state );
    check_no3d_evaluator( state );
}
    
}
}
}