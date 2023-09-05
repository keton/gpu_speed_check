
# download PCI IDs database
pciids = custom_target('update_pciids', command: ['update-pciids.cmd', '@OUTPUT@'], output: 'pci.ids.gz')

windows = import('windows')
program_rc = windows.compile_resources('program_rc.rc', include_directories:'../src', depends: pciids)
