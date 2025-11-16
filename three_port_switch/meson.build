project('three_port_switch', 'C',
	version: '1.0.0',
	license: 'BSD-3',
	default_options: ['buildtype=debug'],
	meson_version: '>= 0.61.2'
)

APP_NAME = 'three_port_switch'

# Experimental API flag
add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c'])

# Dependencies
app_dependencies = []
app_dependencies += dependency('doca-common')
app_dependencies += dependency('doca-flow')
app_dependencies += dependency('doca-dpdk-bridge')
app_dependencies += dependency('doca-argp')
app_dependencies += dependency('libdpdk')

# Source files
app_srcs = [
	APP_NAME + '.c',
]

# Include directories
app_inc_dirs  = []
app_inc_dirs += include_directories('.')

# Executable
executable('doca_' + APP_NAME, app_srcs,
	c_args : '-Wno-missing-braces',
	dependencies : app_dependencies,
	include_directories: app_inc_dirs,
	install: false)
