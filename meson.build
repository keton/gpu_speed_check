project('pciutils_template', ['c','cpp'],
  version : run_command('git', 'describe', '--tags', check: true).stdout().strip().replace('v',''),
  default_options : ['warning_level=3', 'c_std=c17', 'cpp_std=c++20'])

subdir('assets')
subdir('src')
