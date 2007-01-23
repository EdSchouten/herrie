audio_file.o: stdinc.h.gch audio_file.c audio_file.h audio_format.h vfs.h
audio_format_mp3.o: stdinc.h.gch audio_format_mp3.c audio_format.h audio_file.h audio_output.h
audio_format_sndfile.o: stdinc.h.gch audio_format_sndfile.c audio_format.h audio_file.h audio_output.h
audio_format_vorbis.o: stdinc.h.gch audio_format_vorbis.c audio_format.h audio_file.h audio_output.h
audio_output_ao.o: stdinc.h.gch audio_output_ao.c audio_file.h audio_output.h config.h gui.h
audio_output_oss.o: stdinc.h.gch audio_output_oss.c audio_file.h audio_output.h config.h
audio_output_sdl.o: stdinc.h.gch audio_output_sdl.c audio_file.h audio_output.h gui.h
config.o: stdinc.h.gch config.c config.h gui.h
gui_browser.o: stdinc.h.gch gui_browser.c config.h gui_internal.h gui.h gui_vfslist.h vfs.h playq.h
gui_draw.o: stdinc.h.gch gui_draw.c config.h gui_internal.h gui.h
gui_input.o: stdinc.h.gch gui_input.c gui_internal.h gui.h playq.h
gui_msgbar.o: stdinc.h.gch gui_msgbar.c gui_internal.h gui.h
gui_playq.o: stdinc.h.gch gui_playq.c audio_file.h gui_internal.h gui.h gui_vfslist.h vfs.h playq.h
gui_vfslist.o: stdinc.h.gch gui_vfslist.c gui_vfslist.h gui_internal.h gui.h vfs.h
main.o: stdinc.h.gch main.c audio_output.h config.h gui.h playq.h scrobbler.h audio_file.h vfs.h
playq.o: stdinc.h.gch playq.c audio_file.h audio_output.h gui.h playq.h vfs.h
scrobbler.o: stdinc.h.gch scrobbler.c config.h gui.h scrobbler.h audio_file.h scrobbler_internal.h
scrobbler_hash.o: stdinc.h.gch scrobbler_hash.c scrobbler_internal.h
scrobbler_send.o: stdinc.h.gch scrobbler_send.c config.h scrobbler_internal.h
vfs.o: stdinc.h.gch vfs.c config.h vfs.h vfs_modules.h
vfs_http.o: stdinc.h.gch vfs_http.c gui.h vfs_modules.h vfs.h
vfs_playlist.o: stdinc.h.gch vfs_playlist.c vfs_modules.h vfs.h
vfs_regular.o: stdinc.h.gch vfs_regular.c config.h vfs_modules.h vfs.h
