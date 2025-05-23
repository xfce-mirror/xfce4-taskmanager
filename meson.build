project(
  'xfce4-taskmanager',
  'c',
version : '1.6.0-dev',
  license : 'GPL-2.0-or-later',
  meson_version : '>= 0.54.0',
  default_options : ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'apps'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'

cc = meson.get_compiler('c')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'glib': '>= 2.66.0',
  'gtk': '>= 3.24.0',
  'cairo': '>= 1.5.0',
  'libx11': '>= 1.6.7',
  'libxmu': '>= 1.1.2',
  'wnck': '>= 3.2',
  'xfce4': '>= 4.18.0',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
cairo = dependency('cairo', version: dependency_versions['cairo'])
libxmu = dependency('xmu', version: dependency_versions['libxmu'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
xfconf = dependency('libxfconf-0', version: dependency_versions['xfce4'])

feature_cflags = []
headers = []

# Optional dependencies
libx11 = dependency('x11', version: dependency_versions['libx11'], required: get_option('x11'))
if libx11.found()
  feature_cflags += '-DHAVE_LIBX11=1'
endif

libwnck = dependency('libwnck-3.0', version: dependency_versions['wnck'], required: get_option('wnck'))
if libwnck.found()
  feature_cflags += '-DHAVE_WNCK=1'
endif

# Check the OS Family
target_os = host_machine.system()
libkvm = dependency('', required: false)
libkstat = dependency('', required: false)
if get_option('skel')
  os_implementation = 'skel'
else
  if target_os == 'freebsd'
    os_implementation = 'freebsd'
    libkvm = cc.find_library('kvm')
    headers += [
      'sys/param.h',
      'sys/proc.h',
      'sys/sysctl.h',
      'sys/types.h',
      'sys/user.h',
      'fcntl.h',
      'kvm.h',
      'paths.h',
      'pwd.h',
      'unistd.h',
    ]
  elif ['dragonfly', 'netbsd', 'openbsd'].contains(target_os)
    os_implementation = 'bsd'
    headers += [
      'sys/param.h',
      'sys/sched.h',
      'sys/swap.h',
      'sys/sysctl.h',
      'sys/types.h',
      'err.h',
      'pwd.h',
      'stdlib.h',
      'string.h',
      'unistd.h',
    ]
  elif target_os == 'sunos'
    os_implementation = 'solaris'
    libkstat = cc.find_library('kstat')
    headers += [
      'sys/procfs.h',
      'sys/stat.h',
      'sys/swap.h',
      'sys/types.h',
      'fcntl.h',
      'kstat.h',
      'procfs.h',
      'pwd.h',
      'stdlib.h',
      'string.h',
    ]
  elif target_os == 'linux'
    os_implementation = 'linux'
    headers += [
      'sys/resource.h',
      'sys/stat.h',
      'sys/types.h',
      'pwd.h',
      'signal.h',
      'stdio.h',
      'string.h',
      'unistd.h',
    ]
  elif target_os == 'darwin'
    os_implementation = 'darwin'
    headers += [
      'mach/mach.h',
      'sys/param.h',
      'sys/proc.h',
      'sys/sysctl.h',
      'sys/types.h',
      'Availability.h',
      'err.h',
      'stdlib.h',
      'string.h',
      'TargetConditionals.h',
      'unistd.h',
   ]
  else
    error('no OS implementation is available for target:', target_os)
  endif
endif

message('OS implementation:', os_implementation)

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

foreach header : headers
  if cc.check_header(header)
    extra_cflags += '-DHAVE_@0@=1'.format(header.underscorify().to_upper())
  endif
endforeach

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DTVM_VERSION_HELP="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

i18n.merge_file(
    input: 'xfce4-taskmanager.desktop.in',
    output: 'xfce4-taskmanager.desktop',
    po_dir: 'po',
    type: 'desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'applications',
)

subdir('po')
subdir('data' / 'icons')
subdir('src')
