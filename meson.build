project('doca-flow-simple', 'C',
	version: '1.0.0',
	default_options: ['buildtype=debug']
)

# Find DOCA dependency
doca_dep = dependency('doca', required: true)

# Find DPDK dependency
dpdk_dep = dependency('libdpdk', required: true)

# Source files
sources = files('doca_flow_simple.c')

# Executable
executable('doca_flow_simple',
	sources,
	dependencies: [doca_dep, dpdk_dep],
	install: true
)
