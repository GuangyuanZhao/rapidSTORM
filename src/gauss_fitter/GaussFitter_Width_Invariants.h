#ifndef DSTORM_GAUSSFITTER_WIDTH_INVARIANTS_H
#define DSTORM_GAUSSFITTER_WIDTH_INVARIANTS_H

#include <fit++/Exponential2D.hh>
#include <dStorm/engine/JobInfo_decl.h>
#include <dStorm/engine/Spot_decl.h>
#include <dStorm/engine/Image.h>
#include <dStorm/Localization_decl.h>
#include "GaussFitterConfig_decl.h"

namespace dStorm {
namespace engine {

using namespace fitpp;
using namespace fitpp::Exponential2D;

template <int FitFlags, bool Residue_Analysis>
struct Width_Invariants;

template <int FitFlags>
struct Width_Invariants<FitFlags, true>
: public Width_Invariants<FitFlags, false>
{
    typedef typename fitpp::Exponential2D::For<2, FitFlags> FitGroup;
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
