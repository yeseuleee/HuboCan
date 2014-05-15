#ifndef HUBOQTMANAGERWIDGET_H
#define HUBOQTMANAGERWIDGET_H

#include <QWidget>

#include <vector>
#include <QVector>
#include <QThread>

#include "ui_ManagerWidget.h"
#include "AchdHandle.h"

#include "HuboRT/ManagerReq.hpp"
#include "HuboCmd/AuxSender.hpp"

namespace HuboQt {

typedef QVector<AchdHandle*> AchdPtrArray;

class LocalManager : public QThread
{
    Q_OBJECT

public:

    void run();

};

class ManagerWidget : public QWidget
{
    Q_OBJECT
    
public:
    ManagerWidget();
    ~ManagerWidget();
    
    void load_hostname();

    void set_displaying_locked_procs(bool displaying_locked_procs);
    QString selected_button_style;
    
protected:

    LocalManager _lmgr;
    
    bool _displaying_locked_procs;
    bool _connected;
    
    Ui::ManagerWidget* _ui;
    
    HuboRT::ManagerReq* _req;
    HuboCmd::AuxSender _init;
    
    AchdPtrArray _perm_achd_handles;    // This is for the Manager's channels which will
                                        // always be exactly the same
    
    AchdPtrArray _main_achd_handles;    // This is for the rest of the channels which we
                                        // will depend on the Manager to inform us about
    
    bool _double_check_init();
    
    void _clear_achd_handles(AchdPtrArray& achds);
    void _parse_channel_descriptions(const StringArray& descs);
    void _display_channels();
    
    void _display_registered_processes(const StringArray& procs);
    void _display_locked_processes(const StringArray& locks);
    
    void _start_achds(AchdPtrArray& achds);
    void _disconnect_achds(AchdPtrArray& achds);
    
    void _check_if_achds_running(AchdPtrArray& achds, int exit_code);
    
    void _set_status(manager_err_t incoming_status, const QString& status_context);
    
protected Q_SLOTS:

    void local_mgr_launch();
    void local_mgr_stop();
    
    void save_hostname(const QString& new_name);
    
    void start_all_achds();
    void disconnect_all_achds();
    
    void startup_everything();
    void shutdown_everything();
    void homeall();
    
    void create_all();
    void destroy_all();
    void launch_all();
    void stop_all();
    void kill_all();
    
    void refresh_chans();
    void refresh_registered_procs();
    void refresh_registered_procs_raw();
    void refresh_locked_procs();
    void refresh_locked_procs_raw();
    void timer_refresh();

    void create_chan();
    void destroy_chan();
    void launch_proc();
    void stop_proc();
    void kill_proc();
    
    void inform_disconnect(int exit_code);

Q_SIGNALS:

    void channels_opened();
    
private:
    
    ManagerWidget(const ManagerWidget& doNotCopy);
    ManagerWidget& operator=(const ManagerWidget& doNotCopy);
    
};

} // namespace HuboQt

#endif // HUBOQTMANAGERWIDGET_H
