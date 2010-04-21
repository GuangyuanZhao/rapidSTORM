#ifndef DSTORM_GAUSSFITTER_WIDTH_INVARIANTS_H
#define DSTORM_GAUSSFITTER_WIDTH_INVARIANTS_H

#include <fit++/Exponential2D.hh>
#include <dStorm/engine/JobInfo.h>
#include <dStorm/engine/Spot_decl.h>
#include <dStorm/Localization_decl.h>
#include "GaussFitterConfig_decl.h"

namespace dStorm {
namespace engine {

using namespace fitpp;
using namespace fitpp::Exponential2D;

template <bool Free_Sigmas, bool Residue_Analysis>
struct Width_Invariants;

struct StartInformation {
    Eigen::Vector2i maxs;
    Eigen::Vector2d start;
};

template <bool Free_Sigmas>
struct Width_Invariants<Free_Sigmas, false>
{
    typedef typename fitpp::Exponential2D::For<1,
            (Free_Sigmas) ? fitpp::Exponential2D::FreeForm
                        : fitpp::Exponential2D::FixedForm>
        FitGroup;

    typename FitGroup::Constants constants;
    FitFunction<FitGroup::VarC> fit_function;
    typename FitGroup::NamedParameters params;
    const double amplitude_threshold;
    const double start_sx, start_sy, start_sxy;

    Width_Invariants( const GaussFitterConfig&, const JobInfo& );
    StartInformation set_start( const Spot& spot, const Image& image,
                    double shift_estimate,
                    typename FitGroup::Variables* variables );
    bool check_result( typename FitGroup::Variables *variables, 
                       Localization *target,
                       const StartInformation& start );
};

template <bool Free_Sigmas>
struct Width_Invariants<Free_Sigmas, true>
: public Width_Invariants<Free_Sigmas, false>
{
    typedef typename fitpp::Exponential2D::For<2,
            (Free_Sigmas) ? fitpp::Exponential2D::FreeForm
                        : fitpp::Exponential2D::FixedForm>
        FitGroup;

    typename FitGroup::Constants constants;
    FitFunction<FitGroup::VarC> fit_function;
    typename FitGroup::NamedParameters params;
    const double asymmetry_threshold, required_peak_distance_sq;

    Width_Invariants( const GaussFitterConfig&, const JobInfo& );
    void start_from_splitted_single_fit
        ( typename FitGroup::Variables* v, const Eigen::Vector2i& dir);
    bool peak_distance_small(typename FitGroup::Variables *variables);
;
};

}
}

#endif