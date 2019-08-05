project('crashmanager', 'c',
  version: '0.1.0',
  meson_version: '>= 0.40.0',
  )

add_project_arguments([
  '-I' + meson.build_root(),
  ], language: 'c')

cdm_c_include_dirs = [
  'src/common'
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
configure_file(input : 'src/common/cdm-defaults.h.in',
  output : 'cdm-defaults.h',
  configuration : cdm_defaults_data)

cdm_msgtype_data = configuration_data()
cdm_msgtype_data.set('version', meson.project_version())
configure_file(input : 'src/common/cdm-message-type.h.in',
  output : 'cdm-message-type.h',
  configuration : cdm_msgtype_data)

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

dep_genivi_nsm = declare_dependency()
if get_option('GENIVI_NSM')
  add_project_arguments('-DWITH_GENIVI_NSM', language : 'c')
  dep_genivi_nsm += dependency('CommonAPI', method : 'cmake', modules : ['CommonAPI'])
  dep_genivi_nsm += dependency('CommonAPI-SomeIP', method : 'cmake', modules : [
    'org.genivi.nodestatemanager.proxy.core',
    'org.genivi.nodestatemanager.proxy.someip'
    ])

  libcapiwrap_sources = [
    'src/extern/libcapiwrap/nsm_consumer.c',
    ]

  libcapiwrap_deps = [dep_genivi_nsm]

  dep_libcapiwrap = static_library('libcapiwrap', libcapiwrap_sources,
    dependencies: libcapiwrap_deps,
    install: true,
    )

  dep_genivi_nsm += dep_libcapiwrap
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

if get_option('CRASHTEST')
  crashtest_sources = [
    'tests/crashtest/crashtest.c',
    ]

  crashtest_deps = []

  executable('crashtest', crashtest_sources,
    dependencies: crashtest_deps,
    install: true,
    )
endif

dep_glib = dependency('glib-2.0', version : '>=2.58')
dep_libarchive = dependency('libarchive')

if get_option('CRASHMANAGER')
  crashmanager_sources = [
    'src/common/cdm-message.c',
    'src/common/cdm-logging.c',
    'src/common/cdm-options.c',
    'src/common/cdm-utils.c',
    'src/manager/cdm-main.c',
    'src/manager/cdm-client.c',
    'src/manager/cdm-server.c',
    'src/manager/cdm-janitor.c',
    'src/manager/cdm-lifecycle.c',
    'src/manager/cdm-sdnotify.c',
    'src/manager/cdm-transfer.c',
    'src/manager/cdm-bitstore.c',
    'src/manager/cdm-application.c',
    ]

  crashmanager_deps = [
    dep_glib,
    dep_lxc,
    dep_systemd,
    dep_genivi_dlt,
    dep_genivi_nsm
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
  configure_file(input : 'config/20-crashhandler.conf.in',
    output : '20-crashhandler.conf',
    configuration : sysctl_data,
    install_dir : '/etc/sysctl.d')

  crashhandler_sources = [
    'src/common/cdm-message.c',
    'src/common/cdm-options.c',
    'src/common/cdm-logging.c',
    'src/common/cdm-utils.c',
    'src/handler/cdh-main.c',
    'src/handler/cdh-archive.c',
    'src/handler/cdh-context.c',
    'src/handler/cdh-coredump.c',
    'src/handler/cdh-application.c',
    'src/handler/cdh-manager.c',
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

if get_option('CRASHDECODER')
  crashdecoder_sources = [
    'src/decoder/cdd-main.c',
    ]

  crashdecoder_deps = [dep_glib]

  executable('crashdecoder', crashdecoder_sources,
    dependencies: crashdecoder_deps,
    include_directories : include_directories(cdm_c_include_dirs), 
    c_args: cdm_c_compiler_args,
    install: true,
    )
endif

