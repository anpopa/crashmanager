project('crashmanager', 'c',
  version: '0.1.0',
  meson_version: '>= 0.40.0',
  )

add_project_arguments([
  '-I' + meson.build_root(),
  ], language: 'c')

cdm_c_include_dir = [
   'crashmanager'
   ]

cdm_c_compiler_args = [
  '-Wall',
  '-Wextra',
  '-Wno-unused_function',
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
configure_file(input : 'crashmanager/cdm-defaults.h.in',
  output : 'cdm-defaults.h',
  configuration : cdm_defaults_data)

cdm_msgtype_data = configuration_data()
cdm_msgtype_data.set('version', meson.project_version())
configure_file(input : 'crashmanager/cdm-message-type.h.in',
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
	'ctests/crashtest/crashtest.c',
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
	'crashmanager/cdm-main.c',
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
	'crashhandler/cdh-main.c',
	'crashhandler/cdh-archive.c',
	'crashhandler/cdh-context.c',
	'crashhandler/cdh-coredump.c',
	'crashhandler/cdh-crashid.c',
	'crashhandler/cdh-data.c',
	'crashhandler/cdh-manager.c',
	'crashmanager/cdm-message.c',
	'crashmanager/cdm-options.c',
	'crashmanager/cdm-logging.c',
	]

  crashhandler_deps = [dep_glib]

  executable('crashhandler', crashhandler_sources,
	dependencies: crashhandler_deps,
	include_directories : cdm_c_include_dir,
	c_args: cdm_c_compiler_args,
	install: true,
	)
endif

if get_option('CRASHDECODER')
  crashdecoder_sources = [
	'crashdecoder/cdd-main.c',
	]

  crashdecoder_deps = [dep_glib]

  executable('crashdecoder', crashdecoder_sources,
	dependencies: crashdecoder_deps,
	c_args: cdm_c_compiler_args,
	install: true,
	)
endif
