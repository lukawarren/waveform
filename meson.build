project(
    'waveform',
    'c',
    version: '0.1.0',
    default_options: [
        'c_std=c11',
        'warning_level=everything'
    ]
)

# Dependencies
gtk4_dep = dependency('gtk4', version: '>= 4.0')
libadwaita_dep = dependency('libadwaita-1', version: '>= 1.0')
sdl2_dep = dependency('sdl2', version: '>= 2.0.0')
sdl2mixer_dep = dependency('sdl2_mixer', version: '>= 2.0.0')
gnome = import('gnome')

# Sources
src = [
    'src/main.c',
    'src/playlist.c',
    'src/playback.c',
    'src/audio_stream.c',
    'src/visualiser.c'
]
include_dir = include_directories('include')

# UI / resources
blueprints = custom_target('blueprints',
    input: files(
        'src/ui/window.blp'
    ),
    output: '.',
    command: [find_program('blueprint-compiler'), 'batch-compile', '@OUTPUT@', '@CURRENT_SOURCE_DIR@', '@INPUT@'],
)
resources = gnome.compile_resources(
    'resources',
    'waveform.gresource.xml',
    dependencies: [ blueprints ]
)

# Build the executable
executable(
    'waveform',
    src,
    resources,
    dependencies: [gtk4_dep, libadwaita_dep, sdl2_dep, sdl2mixer_dep],
    include_directories: include_dir,
    install: false
)
