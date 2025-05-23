taskmanager_generated_sources = []

taskmanager_generated_sources += gnome.compile_resources(
  'task-manager-resources',
  'task-manager.gresource.xml',
  export: false,
)

taskmanager_sources = [
  taskmanager_generated_sources,
  'main.c',
  'process-monitor.c',
  'process-monitor.h',
  'process-statusbar.c',
  'process-statusbar.h',
  'process-tree-model.c',
  'process-tree-model.h',
  'process-tree-view.c',
  'process-tree-view.h',
  'process-window.c',
  'process-window.h',
  'settings-dialog.c',
  'settings-dialog.h',
  'settings.c',
  'settings.h',
  'task-manager.c',
  'task-manager.h',
  'task-manager-@0@.c'.format(os_implementation),
]

if libwnck.found()
taskmanager_sources += [
  'app-manager.c',
  'app-manager.h',
]
endif

taskmanager = executable(
  'xfce4-taskmanager',
  taskmanager_sources,
  sources: xfce_revision_h,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    cairo,
    libxmu,
    libxfce4util,
    libxfce4ui,
    xfconf,
    libx11,
    libwnck,
    libkvm,
    libkstat,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
