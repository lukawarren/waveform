project(
  'sdl2 mixer',
  'c',
  version : '2.8.0',
  default_options: [
    'warning_level=0'
  ]
)

feature_args = ['-DMUSIC_WAV', '-DMUSIC_OGG']
if get_option('default_library') != 'static' and host_machine.system() == 'windows'
  feature_args += '-DDLL_EXPORT'
endif

sdl2_dep = dependency('sdl2')
ogg_dep = dependency('ogg')
vorbis_dep = dependency('vorbis')
vorbisfile_dep = dependency('vorbisfile')

flac_dep = dependency('libflac', required: get_option('flac'))
if flac_dep.found()
  feature_args += '-DMUSIC_FLAC_LIBFLAC'
endif

opus_dep = dependency('opusfile', required: get_option('opus'))
if opus_dep.found()
  feature_args += '-DMUSIC_OPUS'
endif

feature_args += '-DMUSIC_MP3_MINIMP3'

incdirs = include_directories('include', 'src', 'src/codecs')

sources = files(
  'src/codecs/load_aiff.c',
  'src/codecs/load_voc.c',
  'src/codecs/mp3utils.c',
  'src/codecs/music_cmd.c',
  'src/codecs/music_drflac.c',
  'src/codecs/music_minimp3.c',
  'src/codecs/music_flac.c',
  'src/codecs/music_fluidsynth.c',
  'src/codecs/music_modplug.c',
  'src/codecs/music_nativemidi.c',
  'src/codecs/music_ogg.c',
  'src/codecs/music_ogg_stb.c',
  'src/codecs/music_opus.c',
  'src/codecs/music_timidity.c',
  'src/codecs/music_wav.c',
  'src/codecs/music_xmp.c',
  'src/effect_position.c',
  'src/effect_stereoreverse.c',
  'src/effects_internal.c',
  'src/mixer.c',
  'src/music.c',
  'src/utils.c',
)

sdl2_mixer_lib = library(
  'sdl2mixer_custom',
  sources,
  c_args: feature_args,
  include_directories: incdirs,
  dependencies: [vorbisfile_dep, vorbis_dep, ogg_dep, opus_dep, flac_dep, sdl2_dep]
)

sdl2_mixer_dep = declare_dependency(
  include_directories: incdirs,
  link_with: sdl2_mixer_lib,
)

meson.override_dependency('sdl2_mixer_custom', sdl2_mixer_dep)
