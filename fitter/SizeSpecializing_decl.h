#include <dStorm/engine/SpotFitter_decl.h>
#include <dStorm/engine/Config_decl.h>
#include <dStorm/engine/JobInfo_decl.h>
#include <memory>

namespace dStorm {
namespace fitter {
    template <typename BaseFitter>
    class SizeSpecializing;

    template <typename BaseFitter>
    inline std::auto_ptr<engine::spot_fitter::Implementation>
    create_SizeSpecializing(
        const typename BaseFitter::SizeInvariants::Config&,
        const engine::JobInfo&);
}
}
