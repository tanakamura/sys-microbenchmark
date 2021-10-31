project('hwloc', 'c',
  meson_version : '>=0.57.0',
)

mod = import('unstable_external_project')

options = ['--prefix=@PREFIX@',
           '--libdir=@PREFIX@/@LIBDIR@',
           '--includedir=@PREFIX@/@INCLUDEDIR@',
          ]

cross_options = [ ]

if meson.is_cross_build()
  if host_machine.system() == 'windows'
    cross_options = ['--host=' + host_machine.cpu_family() + '-mingw32']
  else
    cross_options = ['--host=' + host_machine.cpu_family() + '-' + host_machine.system()]
  endif
endif

p = mod.add_project('configure',
                    configure_options : options,
                    cross_configure_options : cross_options,
)

hwloc_dep = p.dependency('hwloc')

