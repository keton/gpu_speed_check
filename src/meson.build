src_files = [
    'main.c',
	'rc_manager.c',
	'pcie_speed.c',
	'console.c',
    program_rc,
]

inc_dirs = include_directories('.')

pciutils_dep = dependency('pciutils', required: true)

deps = [
	pciutils_dep,
]

# run executable as administrator
link_args = [
	'/MANIFEST:EMBED',
	'/MANIFESTUAC:level=\'requireAdministrator\'',
]

executable(meson.project_name(), src_files, include_directories: inc_dirs, dependencies: deps, win_subsystem:'windows', link_args: link_args)
