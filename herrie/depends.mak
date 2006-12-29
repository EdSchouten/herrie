audio.o: audio.c audio.h audio_format.h
audio_format_mp3.o: audio_format_mp3.c audio_format.h audio.h audio_output.h
audio_format_sndfile.o: audio_format_sndfile.c audio_format.h audio.h audio_output.h
audio_format_vorbis.o: audio_format_vorbis.c audio_format.h audio.h audio_output.h
audio_output_ao.o: audio_output_ao.c audio_output.h audio.h config.h
audio_output_oss.o: audio_output_oss.c audio_output.h audio.h config.h
config.o: config.c config.h gui_internal.h gui.h audio.h vfs.h
gui_browser.o: gui_browser.c config.h gui_internal.h gui.h audio.h vfs.h gui_vfslist.h playq.h
gui_draw.o: gui_draw.c config.h gui_internal.h gui.h audio.h vfs.h
gui_input.o: gui_input.c gui_internal.h gui.h audio.h vfs.h playq.h
gui_msgbar.o: gui_msgbar.c gui_internal.h gui.h audio.h vfs.h
gui_playq.o: gui_playq.c gui_internal.h gui.h audio.h vfs.h gui_vfslist.h playq.h
gui_vfslist.o: gui_vfslist.c gui_vfslist.h gui_internal.h gui.h audio.h vfs.h
herrie.o: herrie.c audio_output.h audio.h config.h gui.h playq.h vfs.h scrobbler.h
playq.o: playq.c audio.h audio_output.h gui.h playq.h vfs.h
scrobbler.o: scrobbler.c config.h gui.h audio.h scrobbler.h scrobbler_internal.h
scrobbler_hash.o: scrobbler_hash.c scrobbler_internal.h
scrobbler_send.o: scrobbler_send.c config.h scrobbler_internal.h
vfs.o: vfs.c config.h vfs.h vfs_modules.h
vfs_dir.o: vfs_dir.c config.h vfs_modules.h vfs.h
vfs_file.o: vfs_file.c vfs_modules.h vfs.h
vfs_playlist.o: vfs_playlist.c vfs_modules.h vfs.h
