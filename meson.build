project('sys-microbenchmark', 'cpp', 'c')

null_dep = dependency('', required : false)


omp_dep = dependency('openmp', required : false)

if omp_dep.found()
else
  omp_dep = null_dep
endif


libsmbm_cpp_args = ['-std=c++17']
libsmbm_link_args = []
exe_suffix = []

if host_machine.system() == 'linux'
  have_hwloc = true
  have_cpuinfo = true
elif host_machine.system() == 'android'
  have_hwloc = true
  have_cpuinfo = true
elif host_machine.system() == 'darwin'
  have_hwloc = true
  have_cpuinfo = true
elif host_machine.system() == 'windows'
  have_hwloc = true
  have_cpuinfo = true
elif host_machine.system() == 'emscripten'
  have_hwloc = false
  have_cpuinfo = false

  libsmbm_cpp_args += ['-DNO_MAIN', '-DNO_SETJMP', '-DJS_INTERFACE']
  libsmbm_link_args += ['--bind']
elif host_machine.system() == 'wasi'
  have_hwloc = false
  have_cpuinfo = false

  libsmbm_cpp_args += ['-DNO_SETJMP', '-DWASI']
  exe_suffix = 'wasm'
else
  have_hwloc = false
  have_cpuinfo = false
endif

if have_cpuinfo
  cmake = import('cmake')

  var = cmake.subproject_options()

  if meson.is_cross_build()
    var.add_cmake_defines({'CPUINFO_BUILD_UNIT_TESTS':false,
                           'CPUINFO_BUILD_MOCK_TESTS':false,
                           'CPUINFO_BUILD_BENCHMARKS':false
                          })
  endif

  cpuinfo_proj = cmake.subproject('cpuinfo', options:var)
  cpuinfo_lib = cpuinfo_proj.dependency('cpuinfo')
endif

if have_hwloc
  hwloc_proj = subproject('hwloc')

  hwloc_libs = custom_target('libhwloc',
                             input : 'build-hwloc.sh',
                             output : ['libhwloc_embedded.a'],
                             command: ['bash', '@INPUT@', meson.project_source_root() + '/hwloc']
                            )

  hwloc_dep = hwloc_proj.get_variable('hwloc_dep')
else
  hwloc_dep = null_dep
endif

if host_machine.cpu_family() == 'x86_64'
  avx512_flags = '-mavx512f'
  avx_flags = '-mavx'
else
  avx512_flags = []
  avx_flags = []
endif

simds = [
  ['x86-avx512', avx512_flags],
  ['x86-avx', avx_flags],
]

simd_deps = []

foreach simd : simds
  name = 'libsmbm-' + simd[0]
  cur_lib = static_library(
    name,
    [simd[0] + '.cpp'],
    cpp_args : [simd[1]] + libsmbm_cpp_args
  )

  simd_deps += declare_dependency(link_with : cur_lib)
endforeach

libsmbm = static_library(
  'libbsys-microbenchmark',

  'sys-microbenchmark.cpp',
  'cpuset.cpp',
  'cpu-feature.cpp',
  'sysinfo.cpp',
  'io.cpp',

  'idiv.cpp',
  'syscall.cpp',
  'memory-latency.cpp',
  'memory-bandwidth.cpp',
  'omp.cpp',
  'actual-freq.cpp',
  'ipc.cpp',
  'branch.cpp',
  'instruction.cpp',
  'pipe.cpp',
  'libc.cpp',
  'libcxx.cpp',
  'fpu.cpp',

  'x86.cpp',

  'aarch64.cpp',

  'unknown-arch.cpp',

  'lwabi.cpp',

  'emscripten.cpp',

  dependencies : [omp_dep, hwloc_dep] + simd_deps,

  cpp_args : libsmbm_cpp_args,
)

libsmbm_dep = declare_dependency(
  link_whole : libsmbm,
  link_args : libsmbm_link_args,
  compile_args : libsmbm_cpp_args,
).as_link_whole()

executable('sys-microbenchmark',
           'main.cpp',

           dependencies : libsmbm_dep,
           name_suffix : exe_suffix
)


if not meson.is_cross_build()
  executable('merge-result',
             'merge-result.cpp',
             dependencies : libsmbm_dep)
endif

if host_machine.system() == 'emscripten'
  configure_file(input: 'html/view.html',
                 output: '@PLAINNAME@',
                 copy: true)
  configure_file(input: 'html/view.js',
                 output: '@PLAINNAME@',
                 copy: true)
endif
