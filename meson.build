project('crashmanager', 'c',
        version: '0.1.0',
  meson_version: '>= 0.40.0',
)

add_project_arguments([
  '-I' + meson.build_root() + '/common',
], language: 'c')

cmgr_defaults_data = configuration_data()
cmgr_defaults_data.set('version', meson.project_version())
cmgr_defaults_data.set('config_dir', get_option('CONFDIR'))
configure_file(input : 'common/cmgr_defaults.h.in',
             output : 'cmgr_defaults.h',
             configuration : cmgr_defaults_data)

cmgr_msgtype_data = configuration_data()
cmgr_msgtype_data.set('version', meson.project_version())
configure_file(input : 'common/cmgr_msgtyp.h.in',
             output : 'cmgr_msgtyp.h',
             configuration : cmgr_msgtype_data)

dep_lxc = declare_dependency()
if get_option('LXC')
  add_project_arguments('-DWITH_LXC', language : 'c')
  dep_lxc = dependency('lxc', version : '>=3.0')
endif

dep_systemd = declare_dependency()
if get_option('SYSTEMD')
  add_project_arguments('-DWITH_SYSTEMD', language : 'c')
  dep_systemd = dependency('libsystemd')
endif

dep_genivi_dlt = declare_dependency()
if get_option('GENIVI_DLT')
  add_project_arguments('-DWITH_GENIVI_DLT', language : 'c')
  dep_genivi_dlt = dependency('automotive-dlt')
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
    'genivi/libcapiwrap/nsm_consumer.c',
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

if get_option('CRASHTEST')
  crashtest_sources = [
    'testing/crashtest/crashtest.c',
    ]

  crashtest_deps = []

  executable('crashtest', crashtest_sources,
    dependencies: crashtest_deps,
    install: true,
    )
endif

if get_option('SANITIZER')
  add_global_arguments('-DSANITIZER', language : 'c')
endif

dep_glib = dependency('glib-2.0')

if get_option('CRASHMANAGER')
  crashmanager_sources = [
    'crashmanager/cmgr_main.c',
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
    install: true,
  )
endif

if get_option('CRASHHANDLER')
  crashhandler_sources = [
    'crashhandler/chdl_main.c',
  ]

  crashhandler_deps = [dep_glib]

  executable('crashhandler', crashhandler_sources,
    dependencies: crashhandler_deps,
    install: true,
  )
endif

if get_option('CRASHDECODER')
  crashdecoder_sources = [
    'crashdecoder/cdec_main.c',
  ]

  crashdecoder_deps = [dep_glib]

  executable('crashdecoder', crashdecoder_sources,
    dependencies: crashdecoder_deps,
    install: true,
  )
endif
