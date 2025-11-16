#
# Standalone Meson build for Simple Forward VNF
#

project('simple_fwd_vnf', 'C', 'CPP',
	version: '3.1.0',
	license: 'BSD-3',
	default_options: ['buildtype=debug'],
	meson_version: '>= 0.61.2'
)

APP_NAME = 'simple_fwd_vnf'

# Comment this line to restore warnings of experimental DOCA features
add_project_arguments('-D DOCA_ALLOW_EXPERIMENTAL_API', language: ['c', 'cpp'])

app_dependencies = []
# Required for all DOCA programs
app_dependencies += dependency('doca-common')
# DOCA Flow
app_dependencies += dependency('doca-flow')
# DOCA DPDK bridge
app_dependencies += dependency('doca-dpdk-bridge')
# Utility DOCA library for executables
app_dependencies += dependency('doca-argp')
# Required DOCA Driver
app_dependencies += dependency('libdpdk')

app_srcs = [
	# Main application
	APP_NAME + '.c',
	'simple_fwd.c',
	'simple_fwd_ft.c',
	'simple_fwd_pkt.c',
	'simple_fwd_port.c',
	'simple_fwd_vnf_core.c',
	# Common files
	'common.c',
	'flow_common.c',
	'dpdk_utils.c',
	'utils.c',
]

app_inc_dirs  = []
# Local directory
app_inc_dirs += include_directories('.')

executable('doca_' + APP_NAME, app_srcs,
	c_args : '-Wno-missing-braces',
	dependencies : app_dependencies,
	include_directories: app_inc_dirs,
	install: false)
