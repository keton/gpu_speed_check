
# download PCI IDs database
# update-pciids.cmd places output in cwd so we need to copy it into build folder first
fs = import('fs')
update_pciids = fs.copyfile('update-pciids.cmd')[0]
pciids = custom_target('update_pciids', command: update_pciids, output: 'pci.ids.gz')

windows = import('windows')
program_rc = windows.compile_resources('program_rc.rc', include_directories:'../src', depends: pciids)
