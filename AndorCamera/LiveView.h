#ifndef ANDORCAMERA_LIVEVIEW_H
#define ANDORCAMERA_LIVEVIEW_H

#include "AndorDirect_decl.h"
#include <boost/utility.hpp>
#include <boost/units/quantity.hpp>
#include <simparm/Object.hh>
#include <boost/optional/optional.hpp>
#include <dStorm/helpers/DisplayManager.h>
#include <boost/units/systems/camera/frame_rate.hpp>
#include <dStorm/helpers/thread.h>
#include <dStorm/ImageTraits.h>
#include <simparm/NumericEntry.hh>

namespace dStorm {
namespace AndorCamera {

class LiveView :
    boost::noncopyable, public simparm::Object,
    public dStorm::Display::DataSource
{
  public:
    typedef traits::Optics<2>::Resolutions Resolution;

  private:
    Resolution resolution;
    simparm::BoolEntry show_live;
    boost::optional< boost::units::quantity<boost::units::camera::intensity> >
        lower_user_limit, upper_user_limit;

    ost::Mutex window_mutex, change_mutex;
    CamImage current_image_content;

    std::auto_ptr<dStorm::Display::Change> change;
    std::auto_ptr<dStorm::Display::Manager::WindowHandle> window;

    void registerNamedEntries();

    void show_window(CamImage::Size size);
    void hide_window();

    void compute_image_change( const CamImage& image );
    void compute_key_change( CameraPixel darkest,
                             CameraPixel brightest );

    std::auto_ptr<dStorm::Display::Change> get_changes();
    void notice_closed_data_window();
    void notice_user_key_limits(int, bool, std::string);

  public:
    LiveView(
        bool on_by_default,
        Resolution resolution );
    ~LiveView();
    void show( const CamImage& image );
};

}
}

#endif