#include "wxManager.h"
#include <wx/wx.h>
#include "App.h"
#include "Window.h"
#include <dStorm/helpers/Runnables.h>
#include <boost/thread.hpp>

#include "debug.h"

namespace dStorm {
namespace Display {

struct wxManager::WindowHandle
: public Manager::WindowHandle
{
    wxManager& m;
    Window *associated_window;
    WindowHandle(wxManager& m);
    ~WindowHandle();
};

struct wxManager::IdleCall
: public ost::Runnable 
{
    wxManager& m;
    IdleCall(wxManager& m) : m(m) {}
    void run() { m.exec_waiting_runnables(); }
};

wxManager::wxManager() 
: ost::Thread("wxWidgets"),
  open_handles(0),
  closed_all_handles(mutex),
  was_started( false ),
  may_close( false ),
  toolkit_available( true ),
  idle_call( new IdleCall(*this) )
{
}

struct wxManager::Closer : public ost::Runnable {
    void run() throw() { wxGetApp().close(); }
};

wxManager::~wxManager() {
    DEBUG("Stopping display thread");
    may_close = true;
    if ( was_started ) {
        Closer closer;
        run_in_GUI_thread( &closer );
        closed_all_handles.signal();
        join();
    }
    DEBUG("Stopped display thread");
}

void wxManager::run() throw()
{
    DEBUG("Running display thread");
    int argc = 0;
    App::idle_call = idle_call.get();
    if ( !may_close )
        wxEntry(argc, (wxChar**)NULL);
    toolkit_available = false;

    ost::MutexLock lock( mutex );
    exec_waiting_runnables();
    while ( !may_close || open_handles > 0 ) {
        closed_all_handles.wait();
        exec_waiting_runnables();
    }
}

void wxManager::increase_handle_count() {
    ost::MutexLock lock(mutex);
    open_handles++;
}

void wxManager::decrease_handle_count() {
    ost::MutexLock lock(mutex);
    open_handles--;
    if ( open_handles == 0 && may_close ) {
        closed_all_handles.signal();
        if ( toolkit_available )
            wxGetApp().close();
    }
}

struct wxManager::Creator : public ost::Runnable {
    wxManager& m;

    Creator(wxManager& m) : m(m) {}

    WindowProperties properties;
    DataSource* handler;
    WindowHandle *handle;

    void run() throw();
};

void wxManager::Creator::run() throw() {
    if ( m.toolkit_available ) {
        Window * w = new Window( properties, handler, handle );
        handle->associated_window = w;
    }
}

std::auto_ptr<Manager::WindowHandle>
wxManager::register_data_source(
    const WindowProperties& properties,
    DataSource& handler
)
{
    if ( ! was_started ) {
        ost::MutexLock lock( mutex );
        if ( ! was_started ) {
            was_started = true;
            start();
        }
    }
    std::auto_ptr<Creator> creator( new Creator(*this) );
    creator->properties = properties;
    creator->handler = &handler;

    std::auto_ptr<WindowHandle> 
        handle(new WindowHandle(*this));
    increase_handle_count();
    creator->handle = handle.get();

    run_in_GUI_thread( creator.release() );

    return std::auto_ptr<Manager::WindowHandle>(
        handle.release() );
}

class wxManager::Disassociator
: public ost::WaitableRunnable
{
    WindowHandle& h;
    wxManager& m;
  public:
    Disassociator(wxManager& m, WindowHandle& handle) : h(handle), m(m) {}
    void run() throw() {
        if ( h.associated_window != NULL )
            h.associated_window->remove_data_source();
        m.decrease_handle_count();
    }
};

wxManager::WindowHandle::WindowHandle(wxManager& m)
: m(m), associated_window(NULL)
{
}

wxManager::WindowHandle::~WindowHandle()
{
    /* This code must be run even if associated_window is
     * NULL to avoid race condition. */
    Disassociator d(m, *this);
    m.run_in_GUI_thread( &d );
    d.wait();
}

void wxManager::run_in_GUI_thread( ost::Runnable* code ) 
{
    if ( ost::Thread::current_thread() == 
         static_cast<ost::Thread*>(this) )
        exec( code );
    else {
        {
            ost::MutexLock lock(mutex);
            run_queue.push( code );
        }
        wxWakeUpIdle();
        closed_all_handles.signal();
    }
}

void wxManager::exec_waiting_runnables() {
    ost::MutexLock lock(mutex);
    while ( ! run_queue.empty() ) {
        exec( run_queue.front() );
        run_queue.pop();
    }
}

void wxManager::exec(ost::Runnable* runnable) {
    runnable->run();
}

void wxManager::disassociate_window
    ( Window *window, WindowHandle* handle )
{
    handle->associated_window = NULL;
}

}
}
