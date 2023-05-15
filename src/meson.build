src_files = [
    'main.c',
	'rc_manager.c',
	'pcie_speed.c',
	'console.c',
	'toast.cpp',
    program_rc,
]

inc_dirs = include_directories('.')

pciutils_dep = dependency('pciutils', required: true)

# add WinToast library
cmake   = import('cmake')
wintoast_var = cmake.subproject_options()
wintoast_var.add_cmake_defines({'WINTOASTLIB_BUILD_EXAMPLES': false})
wintoast_var.add_cmake_defines({'WINTOASTLIB_QT_ENABLED': false})
wintoast_var.set_override_option('warning_level', '2')

if get_option('buildtype') == 'release'
	wintoast_var.add_cmake_defines({'CMAKE_BUILD_TYPE': 'release'})
endif

wintoast_pro = cmake.subproject('wintoast', options: wintoast_var, required: true)
wintoast_dep = wintoast_pro.dependency('WinToast')

deps = [
	pciutils_dep,
	wintoast_dep,
]

# run executable as administrator
link_args = [
	'/MANIFEST:EMBED',
	'/MANIFESTUAC:level=\'requireAdministrator\'',
]

executable(meson.project_name(), src_files, include_directories: inc_dirs, dependencies: deps, win_subsystem:'windows', link_args: link_args)
