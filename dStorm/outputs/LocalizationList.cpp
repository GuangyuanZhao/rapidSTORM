#define DSTORM_LOCALIZATIONLIST_CPP
#include "LocalizationList.h"
#include <cassert>

using namespace std;

namespace dStorm {
namespace outputs {

    LocalizationList::LocalizationList( output::Localizations* target )
        : OutputObject("LocalizationList", "Localization target file"),
            target(target) {}

    LocalizationList::LocalizationList( const LocalizationList& o )
        : OutputObject(o), 
          allocTarget( (o.allocTarget.get()) ? 
            new output::Localizations(*o.allocTarget) : NULL ),
          target( (o.allocTarget.get()) ? allocTarget.get() : o.target )
        {}

    output::Output::AdditionalData
    LocalizationList::announceStormSize(const Announcement &a)
    { 
        input::Traits<Localization> traits = a;

        if (target == NULL) {
            allocTarget.reset( new output::Localizations( traits ) );
            target = allocTarget.get();
        } else
            target->setDim( traits );

        return AdditionalData();
    }

    output::Output::Result 
        LocalizationList::receiveLocalizations(const EngineResult& er)
    { 
        assert(target != NULL);
        target->insert( er ); 
        return KeepRunning; 
    }

}
}
