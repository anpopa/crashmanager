project('crashmanager', 'c',
  version: '0.1.0',
  meson_version: '>= 0.49.0',
  )

add_project_arguments([
  '-I' + meson.build_root(),
  ], language: 'c')

cdm_c_include_dirs = [
  'common', 
  'libcdhepilog'
  ]

cdm_c_compiler_args = [
  '-Wall',
  '-Wextra',
  '-Wno-unused-function',
  '-Wformat',
  '-Wno-variadic-macros',
  '-Wno-strict-aliasing',
  '-fstack-protector-strong',
  '-fwrapv',
  '-Wformat-signedness',
  '-Wmissing-include-dirs',
  '-Wimplicit-fallthrough=5',
  '-Wunused-parameter',
  '-Wuninitialized',
  '-Walloca',
  '-Wduplicated-branches',
  '-Wduplicated-cond',
  '-Wfloat-equal',
  '-Wshadow',
  '-Wcast-qual',
  '-Wconversion',
  '-Wlogical-op',
  '-Werror',
  '-Wformat-security',
  '-Walloc-zero',
  '-Wcast-align',
  '-Wredundant-decls'
  ]

cdm_defaults_data = configuration_data()
cdm_defaults_data.set('version', meson.project_version())
cdm_defaults_data.set('config_dir', get_option('CONFDIR'))
cdm_defaults_data.set('install_prefix', get_option('prefix'))
configure_file(input : 'common/cdm-defaults.h.in',
  output : 'cdm-defaults.h',
  configuration : cdm_defaults_data)

cdm_config_data = configuration_data()
cdm_config_data.set('version', meson.project_version())
configure_file(input : 'config/crashmanager.conf.in',
  output : 'crashmanager.conf',
  configuration : cdm_config_data,
  install_dir : get_option('CONFDIR'))

dep_lxc = declare_dependency()
if get_option('LXC')
  add_project_arguments('-DWITH_LXC', language : 'c')
  dep_lxc = dependency('lxc', version : '>=3.0')
endif

dep_scp = declare_dependency()
if get_option('SCP_TRANSFER')
  add_project_arguments('-DWITH_SCP_TRANSFER', language : 'c')
  dep_scp = dependency('libssh2', version : '>=1.8')
endif

dep_systemd = declare_dependency()
if get_option('SYSTEMD')
  add_project_arguments('-DWITH_SYSTEMD', language : 'c')
  dep_systemd = dependency('libsystemd')

  sd_service_data = configuration_data()
  sd_service_data.set('install_prefix', get_option('prefix'))
  configure_file(input : 'config/crashmanager.service.in',
    output : 'crashmanager.service',
    configuration : sd_service_data,
    install_dir : '/etc/systemd/system')
endif

dep_genivi_dlt = declare_dependency()
if get_option('GENIVI_DLT')
  add_project_arguments('-DWITH_GENIVI_DLT', language : 'c')
  dep_genivi_dlt = dependency('automotive-dlt', method : 'pkg-config')
endif

if get_option('DEBUG_ATTACH')
  add_project_arguments('-DWITH_DEBUG_ATTACH', language : 'c')
endif

if get_option('TESTS')
  add_project_arguments('-DWITH_TESTS', language : 'c')
endif

if get_option('CRASHMANAGER')
  add_project_arguments('-DWITH_CRASHMANAGER', language : 'c')
endif

dep_dbus_services = declare_dependency()
if get_option('DBUS_SERVICES')
  add_project_arguments('-DWITH_DBUS_SERVICES', language : 'c')
  dep_dbus_services = dependency('gio-2.0', version : '>=2.58')

  dbus_conf = configuration_data()
  dbus_conf.set('install_prefix', get_option('prefix'))
  configure_file(input : 'config/crashmanager-dbus.conf',
    output : 'crashmanager-dbus.conf',
    configuration : dbus_conf,
    install_dir : '/etc/dbus-1/system.d')
endif

dep_threads = dependency('threads')
dep_libcdhepilog = declare_dependency()
if get_option('LIBCDHEPILOG')
  libcdhepilog_sources = [
    'libcdhepilog/cdh-epilog.c',
    'libcdhepilog/cdh-elogmsg.c',
    ]

  libCdhEpilog = static_library('CdhEpilog', 
    libcdhepilog_sources, 
    include_directories : include_directories(cdm_c_include_dirs), 
    install : true
    )

  install_headers('libcdhepilog/cdh-epilog.h', subdir : 'crashmanager')
  dep_libcdhepilog = declare_dependency(link_with : libCdhEpilog)

  pkg = import('pkgconfig')
  pkg.generate(libraries : libCdhEpilog,
             subdirs : 'crashmanager',
             version : '1.0',
             filebase : 'libcdhepilog',
             name : 'libcdhepilog',
             description : 'Crashmanager epilog user library')
endif

if get_option('CRASHTEST')
  crashtest_sources = [
    'testing/crashtest/crashtest.c',
    ]

  crashtest_deps = [
    dep_libcdhepilog,
    dep_threads
    ]

  executable('crashtest', crashtest_sources,
    dependencies: crashtest_deps,
    include_directories : include_directories(cdm_c_include_dirs), 
    install: true,
    )
endif

dep_glib = dependency('glib-2.0', version : '>=2.58')
dep_libarchive = dependency('libarchive')
dep_sqlite = dependency('sqlite3')

if get_option('CRASHMANAGER')
  crashmanager_sources = [
    'common/cdm-message.c',
    'common/cdm-logging.c',
    'common/cdm-options.c',
    'common/cdm-utils.c',
    'libcdhepilog/cdh-elogmsg.c',
    'crashmanager/cdm-main.c',
    'crashmanager/cdm-client.c',
    'crashmanager/cdm-server.c',
    'crashmanager/cdm-elogclt.c',
    'crashmanager/cdm-elogsrv.c',
    'crashmanager/cdm-janitor.c',
    'crashmanager/cdm-transfer.c',
    'crashmanager/cdm-journal.c',
    'crashmanager/cdm-application.c',
    ]

  if get_option('SYSTEMD')
    crashmanager_sources += 'crashmanager/cdm-sdnotify.c'
  endif
  
  if get_option('DBUS_SERVICES')
    crashmanager_sources += 'crashmanager/cdm-dbusown.c'
  endif

  crashmanager_deps = [
    dep_glib,
    dep_lxc,
    dep_libarchive, 
    dep_systemd,
    dep_sqlite,
    dep_genivi_dlt,
    dep_dbus_services, 
    dep_scp,
    ]

  executable('crashmanager', crashmanager_sources,
    dependencies: crashmanager_deps,
    include_directories : include_directories(cdm_c_include_dirs), 
    c_args: cdm_c_compiler_args,
    install: true,
    )
endif

if get_option('CRASHHANDLER')
  sysctl_data = configuration_data()
  sysctl_data.set('install_prefix', get_option('prefix'))
  configure_file(input : 'config/51-crashhandler.conf.in',
    output : '51-crashhandler.conf',
    configuration : sysctl_data,
    install_dir : '/etc/sysctl.d')

  crashhandler_sources = [
    'common/cdm-message.c',
    'common/cdm-options.c',
    'common/cdm-logging.c',
    'common/cdm-utils.c',
    'crashhandler/cdh-main.c',
    'crashhandler/cdh-archive.c',
    'crashhandler/cdh-context.c',
    'crashhandler/cdh-coredump.c',
    'crashhandler/cdh-application.c',
    'crashhandler/cdh-manager.c',
    ]

  crashhandler_deps = [
    dep_glib, 
    dep_libarchive, 
    dep_genivi_dlt
    ]

  executable('crashhandler', crashhandler_sources,
    dependencies: crashhandler_deps,
    include_directories : include_directories(cdm_c_include_dirs), 
    c_args: cdm_c_compiler_args,
    install: true,
    )
endif

if get_option('CRASHINFO')
  crashinfo_sources = [
    'common/cdm-options.c',
    'common/cdm-logging.c',
    'common/cdm-utils.c',
    'crashinfo/cdi-main.c',
    'crashinfo/cdi-application.c',
    'crashinfo/cdi-archive.c',
    'crashinfo/cdi-journal.c',
    ]

  crashinfo_deps = [
    dep_glib, 
    dep_lxc,
    dep_libarchive, 
    dep_sqlite,
    dep_genivi_dlt
    ]

  executable('crashinfo', crashinfo_sources,
    dependencies: crashinfo_deps,
    include_directories : include_directories(cdm_c_include_dirs), 
    c_args: cdm_c_compiler_args,
    install: true,
    )
endif

install_data(sources: 'LICENSE', install_dir: '/usr/share/licenses/crashmanager')

