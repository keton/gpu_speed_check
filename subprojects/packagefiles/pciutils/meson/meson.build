project('pciutils', 'c',
  version : run_command('git', 'describe', '--tags', check: true).stdout().strip().replace('v',''),
  default_options : ['warning_level=2', 'c_std=c17'])

compile_args = meson.get_compiler('c').get_supported_arguments(
  '/wd4244', # 'conversion' conversion from 'type1' to 'type2', possible loss of data
  '/wd4267', # 'conversion' conversion from 'type1' to 'type2', possible loss of data
  '/wd4305', # 'conversion' conversion from 'type1' to 'type2', possible loss of data
  '/wd4098', # 'void' function returning a value
)

compile_args += [
	'-DWIN32=1',
	'-DWIN64=1',
	'-DPCI_CONFIG_H=1',
	'-DPCI_COMPRESSED_IDS=1',
	'-DPCI_HAVE_64BIT_ADDRESS=1',
	'-D_CRT_SECURE_NO_WARNINGS',
	'-DPCI_ARCH_I386',
	'-DPCI_OS_WINDOWS',
	'-DPCI_HAVE_PM_INTEL_CONF',
	'-DPCI_IDS="pci.ids.gz"',
	'-DPCI_PATH_IDS_DIR=""',
	'-DPCILIB_VERSION="@0@"'.format(meson.project_version()),
]

inc_dirs = include_directories('lib')

src_files = [
	'lib/init.c',
	'lib/access.c',
	'lib/generic.c',
	'lib/names.c',
	'lib/filter.c',
	'lib/names-hash.c',
	'lib/names-parse.c',
	'lib/names-net.c',
	'lib/names-cache.c',
	'lib/names-hwdb.c',
	'lib/params.c',
	'lib/caps.c',
	'lib/i386-ports.c',
]

zlib_dep = dependency('zlib', fallback : ['zlib', 'zlib_dep'], default_options: ['default_library=static'])

deps = [
	zlib_dep,
]

# do not expose all headers to base project

fs = import('fs')

dist_includes = [
	fs.copyfile('config.h')[0],
	fs.copyfile('lib/pci.h')[0],
	fs.copyfile('lib/header.h')[0],
	fs.copyfile('lib/types.h')[0],
]

pciutils_lib = static_library('pciutils', src_files, include_directories: inc_dirs, dependencies: deps, c_args : compile_args)
pciutils_dep = declare_dependency(link_with : pciutils_lib, sources : dist_includes)

meson.override_dependency('pciutils', pciutils_dep)

lspci_src = [
	'lspci.c',
	'ls-vpd.c',
	'ls-caps.c',
	'ls-caps-vendor.c',
	'ls-ecaps.c',
	'ls-kernel.c',
	'ls-tree.c',
	'ls-map.c',
	'common.c',
	'compat/getopt.c',
]

setpci_src = [
	'setpci.c',
	'common.c',
	'compat/getopt.c',
]

if not meson.is_subproject()
	executable('lspci', lspci_src, include_directories: inc_dirs, dependencies: deps, link_with: pciutils_lib, c_args : compile_args)
	executable('setpci', setpci_src, include_directories: inc_dirs, dependencies: deps, link_with: pciutils_lib, c_args : compile_args)
endif
