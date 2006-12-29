audio.o: stdinc.h audio.c audio.h stdinc.h audio_format.h
audio_format_mp3.o: stdinc.h audio_format_mp3.c audio_format.h stdinc.h audio.h stdinc.h audio_output.h
audio_format_sndfile.o: stdinc.h audio_format_sndfile.c audio_format.h stdinc.h audio.h stdinc.h audio_output.h
audio_format_vorbis.o: stdinc.h audio_format_vorbis.c audio_format.h stdinc.h audio.h stdinc.h audio_output.h
audio_output_ao.o: stdinc.h audio_output_ao.c audio_output.h stdinc.h audio.h stdinc.h config.h
audio_output_oss.o: stdinc.h audio_output_oss.c audio_output.h stdinc.h audio.h stdinc.h config.h
config.o: stdinc.h config.c config.h stdinc.h gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h
gui_browser.o: stdinc.h gui_browser.c config.h stdinc.h gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h stdinc.h gui_vfslist.h stdinc.h playq.h
gui_draw.o: stdinc.h gui_draw.c config.h stdinc.h gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h
gui_input.o: stdinc.h gui_input.c gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h stdinc.h playq.h
gui_msgbar.o: stdinc.h gui_msgbar.c gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h
gui_playq.o: stdinc.h gui_playq.c gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h stdinc.h gui_vfslist.h stdinc.h playq.h
gui_vfslist.o: stdinc.h gui_vfslist.c gui_vfslist.h stdinc.h gui_internal.h stdinc.h gui.h stdinc.h audio.h stdinc.h vfs.h
herrie.o: stdinc.h herrie.c audio_output.h stdinc.h audio.h stdinc.h config.h stdinc.h gui.h stdinc.h playq.h stdinc.h vfs.h stdinc.h scrobbler.h
playq.o: stdinc.h playq.c audio.h stdinc.h audio_output.h stdinc.h gui.h stdinc.h playq.h stdinc.h vfs.h
scrobbler.o: stdinc.h scrobbler.c config.h stdinc.h gui.h stdinc.h audio.h stdinc.h scrobbler.h stdinc.h scrobbler_internal.h
scrobbler_hash.o: stdinc.h scrobbler_hash.c scrobbler_internal.h
scrobbler_send.o: stdinc.h scrobbler_send.c config.h stdinc.h scrobbler_internal.h
vfs.o: stdinc.h vfs.c config.h stdinc.h vfs.h stdinc.h vfs_modules.h
vfs_dir.o: stdinc.h vfs_dir.c config.h stdinc.h vfs_modules.h stdinc.h vfs.h
vfs_file.o: stdinc.h vfs_file.c vfs_modules.h stdinc.h vfs.h
vfs_playlist.o: stdinc.h vfs_playlist.c vfs_modules.h stdinc.h vfs.h
