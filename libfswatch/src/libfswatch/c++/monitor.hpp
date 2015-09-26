/*
 * Copyright (c) 2014-2015 Enrico M. Crisostomo
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef FSW__MONITOR_H
#  define FSW__MONITOR_H

#  include "filter.hpp"
#  include <vector>
#  include <string>
#  ifdef HAVE_CXX_MUTEX
#    include <mutex>
#  endif
#  include <map>
#  include "event.hpp"
#  include "../c/cmonitor.h"

namespace fsw
{
  typedef void FSW_EVENT_CALLBACK(const std::vector<event> &, void *);

  struct compiled_monitor_filter;

  class monitor
  {
  public:
    monitor(std::vector<std::string> paths,
            FSW_EVENT_CALLBACK * callback,
            void * context = nullptr);
    virtual ~monitor();
    monitor(const monitor& orig) = delete;
    monitor& operator=(const monitor & that) = delete;

    void set_properties(std::map<std::string, std::string> options);
    std::string get_property(std::string name);
    void set_latency(double latency);
    void set_allow_overflow(bool overflow);
    void set_recursive(bool recursive);
    void add_filter(const monitor_filter &filter);
    void set_filters(const std::vector<monitor_filter> &filters);
    void set_follow_symlinks(bool follow);
    void * get_context() const;
    void set_context(void * context);
    void start();
    void add_event_type_filter(const fsw_event_type_filter &filter);
    void set_event_type_filters(const std::vector<fsw_event_type_filter> &filters);

  protected:
    bool accept_event_type(fsw_event_flag event_type) const;
    bool accept_path(const std::string &path) const;
    bool accept_path(const char *path) const;
    void notify_events(const std::vector<event> &events) const;
    void notify_overflow() const;
    std::vector<fsw_event_flag> filter_flags(const event &evt) const;

    virtual void run() = 0;

  protected:
    std::vector<std::string> paths;
    std::map<std::string, std::string> properties;
    FSW_EVENT_CALLBACK * callback;
    void * context = nullptr;
    double latency = 1.0;
    bool allow_overflow = false;
    bool recursive = false;
    bool follow_symlinks = false;

  private:
#  ifdef HAVE_CXX_MUTEX
    std::mutex run_mutex;
#  endif
    std::vector<compiled_monitor_filter> filters;
    std::vector<fsw_event_type_filter> event_type_filters;
  };

  typedef monitor * (*FSW_FN_MONITOR_CREATOR)(std::vector<std::string> paths,
    FSW_EVENT_CALLBACK * callback,
    void * context);

  /*
   * This class maintains a register of the available monitors and let users
   * create monitors by name.  Monitors classes are required to register
   * themselves invoking the register_type method and providing a name to their
   * type.  Registration can be performed using the monitor_creator utility
   * class and the REGISTER_MONITOR and REGISTER_MONITOR_IMPL macros.
   */
  class monitor_factory
  {
  public:
    static monitor * create_monitor(fsw_monitor_type type,
                                    std::vector<std::string> paths,
                                    FSW_EVENT_CALLBACK * callback,
                                    void * context = nullptr);
    static monitor * create_monitor(const std::string & name,
                                    std::vector<std::string> paths,
                                    FSW_EVENT_CALLBACK * callback,
                                    void * context = nullptr);
    static std::vector<std::string> get_types();
    static bool exists_type(const std::string& name);
    static void register_creator(const std::string & name,
                                 FSW_FN_MONITOR_CREATOR creator);
    monitor_factory() = delete;
    monitor_factory(const monitor_factory& orig) = delete;
    monitor_factory& operator=(const monitor_factory & that) = delete;
  private:
    static std::map<std::string, FSW_FN_MONITOR_CREATOR> & creators_by_string();
  };

  /*
   * The constructor of this class perform the registration of the given
   * (name, type) pair in the monitor_factory registry.  This class is used by
   * the REGISTER_MONITOR and REGISTER_MONITOR_IMPL macros.
   */
  template<class M>
  class monitor_registrant
  {
  public:

    monitor_registrant(const std::string & name, fsw_monitor_type type)
    {
      FSW_FN_MONITOR_CREATOR default_creator =
        [](std::vector<std::string> paths,
        FSW_EVENT_CALLBACK * callback,
        void * context = nullptr) -> monitor *
        {
          return new M(paths, callback, context);
        };

      monitor_factory::register_creator(name, default_creator);
    }
  };

  /*
   * This macro is used to simplify the registration process of a monitor
   * type.  Since registration of a monitor type is usually performed once, a
   * static private instance monitor_factory_registrant of the
   * monitor_registrant class is declared by this macro in the enclosing class.
   *
   * Beware that since this macro adds a private qualifier, every field
   * declared after it must be correctly qualified.
   *
   * The use of the REGISTER_MONITOR macro in a class
   * must always be matched by a corresponding use of the REGISTER_MONITOR_IMPL
   * macro in the class definition.
   *
   * To register class my_monitor with type my_type,
   * use the REGISTER_MONITOR macro as in the following example:
   *
   * [my_class.h]
   * class my_monitor
   * {
   *   REGISTER_MONITOR(my_monitor, my_monitor_type);
   *   ...
   * };
   *
   */
#  define REGISTER_MONITOR(classname, monitor_type) \
private: \
static const monitor_registrant<classname> monitor_factory_registrant;

  /*
   * This macro is used to simplify the registration process of a monitor
   * type.  Since registration of a monitor type is usually performed once, a
   * static private instance monitor_factory_registrant of the
   * monitor_registrant class is defined in the monitor class specified by
   * classname.
   *
   * A invocation of the REGISTER_MONITOR_IMPL macro must always be matched by
   * an invocation of the REGISTER_MONITOR macro in the class declaration.
   *
   * To register class my_monitor with type my_type,
   * use the REGISTER_MONITOR macro as in the following example:
   *
   * [my_class.cpp]
   *
   * REGISTER_MONITOR_IMPL(my_monitor, my_monitor_type);
   */
#  define REGISTER_MONITOR_IMPL(classname, monitor_type) \
const monitor_registrant<classname> classname::monitor_factory_registrant(#classname, monitor_type);
}

#endif  /* FSW__MONITOR_H */
