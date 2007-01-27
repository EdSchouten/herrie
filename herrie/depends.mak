audio_file.o: src/audio_file.c src/audio_file.h src/audio_format.h src/vfs.h
audio_format_mp3.o: src/audio_format_mp3.c src/audio_format.h src/audio_file.h src/audio_output.h
audio_format_sndfile.o: src/audio_format_sndfile.c src/audio_format.h src/audio_file.h src/audio_output.h
audio_format_vorbis.o: src/audio_format_vorbis.c src/audio_format.h src/audio_file.h src/audio_output.h
audio_output_ao.o: src/audio_output_ao.c src/audio_file.h src/audio_output.h src/config.h src/gui.h
audio_output_oss.o: src/audio_output_oss.c src/audio_file.h src/audio_output.h src/config.h
audio_output_sdl.o: src/audio_output_sdl.c src/audio_file.h src/audio_output.h src/gui.h
config.o: src/config.c src/config.h src/gui.h
gui_browser.o: src/gui_browser.c src/config.h src/gui_internal.h src/gui.h src/gui_vfslist.h src/vfs.h src/playq.h
gui_draw.o: src/gui_draw.c src/config.h src/gui_internal.h src/gui.h
gui_input.o: src/gui_input.c src/gui_internal.h src/gui.h src/playq.h
gui_msgbar.o: src/gui_msgbar.c src/gui_internal.h src/gui.h
gui_playq.o: src/gui_playq.c src/audio_file.h src/gui_internal.h src/gui.h src/gui_vfslist.h src/vfs.h src/playq.h
gui_vfslist.o: src/gui_vfslist.c src/gui_vfslist.h src/gui_internal.h src/gui.h src/vfs.h
main.o: src/main.c src/audio_output.h src/config.h src/gui.h src/playq.h src/scrobbler.h src/audio_file.h src/vfs.h
playq.o: src/playq.c src/audio_file.h src/audio_output.h src/gui.h src/playq.h src/vfs.h
scrobbler.o: src/scrobbler.c src/config.h src/gui.h src/scrobbler.h src/audio_file.h src/scrobbler_internal.h
scrobbler_hash.o: src/scrobbler_hash.c src/scrobbler_internal.h
scrobbler_send.o: src/scrobbler_send.c src/config.h src/scrobbler_internal.h
vfs.o: src/vfs.c src/config.h src/vfs.h src/vfs_modules.h
vfs_http.o: src/vfs_http.c src/gui.h src/vfs_modules.h src/vfs.h
vfs_playlist.o: src/vfs_playlist.c src/vfs_modules.h src/vfs.h
vfs_regular.o: src/vfs_regular.c src/config.h src/vfs_modules.h src/vfs.h
