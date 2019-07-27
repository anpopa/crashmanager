project('crashmanager', 'c',
        version: '0.1.0',
  meson_version: '>= 0.40.0',
)

add_project_arguments([
  '-I' + meson.build_root() + '/common',
], language: 'c')

cmgr_defaults_data = configuration_data()
cmgr_defaults_data.set('version', meson.project_version())
cmgr_defaults_data.set('config_dir', '/etc')
configure_file(input : 'common/cmgr_defaults.h.in',
             output : 'cmgr_defaults.h',
             configuration : cmgr_defaults_data)

cmgr_msgtype_data = configuration_data()
cmgr_msgtype_data.set('version', meson.project_version())
configure_file(input : 'common/cmgr_msgtyp.h.in',
             output : 'cmgr_msgtyp.h',
             configuration : cmgr_msgtype_data)

opt_lxc = get_option('WITH_LXC')
if opt_lxc == true
  opt_lxc_dep = dependency('lxc', version : '>=3.0', requiered : true)
  add_global_arguments('-DWITH_LXC', language : 'c')
endif

opt_systemd = get_option('WITH_SYSTEMD')
opt_genivi = get_option('WITH_GENIVI')
opt_debug_attach = get_option('WITH_DEBUG_ATTACH')
opt_tests = get_option('WITH_TESTS')
opt_crashtest = get_option('WITH_CRASHTEST')
opt_sanitizer = get_option('WITH_SANITIZER')

global_deps = dependency('glib-2.0')

# Build crashmanager executable
crashmanager_sources = [
  'crashmanager/cmgr_main.c',
]

executable('crashmanager', crashmanager_sources,
  dependencies: [global_deps],
  install: true,
)

# Build crashhandler executable
crashhandler_sources = [
  'crashhandler/chdl_main.c',
]

executable('crashhandler', crashhandler_sources,
  dependencies: [global_deps],
  install: true,
)

# Build crashdecoder executable
crashdecoder_sources = [
  'crashdecoder/cdec_main.c',
]

executable('crashdecoder', crashdecoder_sources,
  dependencies: [global_deps],
  install: true,
)
