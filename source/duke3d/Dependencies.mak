duke3d_h=\
    $(ENGINE_INC)/compat.h \
    $(ENGINE_INC)/pragmas.h \
    $(ENGINE_INC)/build.h \
    $(ENGINE_INC)/baselayer.h \
    $(ENGINE_INC)/polymer.h \
    $(ENGINE_INC)/polymost.h \
    $(ENGINE_INC)/texcache.h \
    $(ENGINE_INC)/cache1d.h \
    $(MACT_INC)/file_lib.h \
    $(MACT_INC)/keyboard.h \
    $(MACT_INC)/mouse.h \
    $(MACT_INC)/joystick.h \
    $(MACT_INC)/control.h \
    $(DUKE3D_SRC)/macros.h \
    $(DUKE3D_SRC)/gamedefs.h \
    $(DUKE3D_SRC)/function.h \
    $(DUKE3D_SRC)/config.h \
    $(DUKE3D_SRC)/sounds.h \
    $(DUKE3D_SRC)/sounds_common.h \
    $(DUKE3D_SRC)/soundsdyn.h \
    $(DUKE3D_SRC)/rts.h \
    $(DUKE3D_SRC)/_rts.h \
    $(DUKE3D_SRC)/soundefs.h \
    $(AUDIOLIB_INC)/fx_man.h \
    $(AUDIOLIB_INC)/music.h \
    $(DUKE3D_SRC)/namesdyn.h \
    $(DUKE3D_SRC)/duke3d.h \
    $(DUKE3D_SRC)/player.h \
    $(DUKE3D_SRC)/sector.h \
    $(DUKE3D_SRC)/game.h \
    $(DUKE3D_SRC)/actors.h \
    $(DUKE3D_SRC)/menus.h

gamedef_h=$(DUKE3D_SRC)/gamedef.h $(DUKE3D_SRC)/gameexec.h $(DUKE3D_SRC)/gamevars.h

game_defs_dep=$(DUKE3D_SRC)/lunatic/_defs_game.lua

$(DUKE3D_OBJ)/game.$o: $(DUKE3D_SRC)/game.cpp $(game_defs_dep) $(MACT_INC)/scriplib.h $(duke3d_h) $(DUKE3D_SRC)/input.h $(DUKE3D_SRC)/osdfuncs.h $(DUKE3D_SRC)/osdcmds.h $(DUKE3D_SRC)/grpscan.h $(DUKE3D_SRC)/demo.h $(ENGINE_INC)/hightile.h
$(DUKE3D_OBJ)/actors.$o: $(DUKE3D_SRC)/actors.cpp $(duke3d_h)
$(DUKE3D_OBJ)/anim.$o: $(DUKE3D_SRC)/anim.cpp $(duke3d_h) $(DUKE3D_SRC)/input.h $(MACT_INC)/animlib.h $(ENGINE_INC)/animvpx.h
$(DUKE3D_OBJ)/cheats.$o: $(DUKE3D_SRC)/cheats.cpp $(DUKE3D_SRC)/cheats.h
$(DUKE3D_OBJ)/cmdline.$o: $(DUKE3D_SRC)/cmdline.cpp $(DUKE3D_SRC)/cmdline.h
$(DUKE3D_OBJ)/demo.$o: $(DUKE3D_SRC)/demo.cpp $(duke3d_h) $(DUKE3D_SRC)/input.h
$(DUKE3D_OBJ)/gamedef.$o: $(DUKE3D_SRC)/gamedef.cpp $(duke3d_h) $(gamedef_h) $(DUKE3D_SRC)/savegame.h
$(DUKE3D_OBJ)/gameexec.$o: $(DUKE3D_SRC)/gameexec.cpp $(DUKE3D_SRC)/gamestructures.cpp $(duke3d_h) $(gamedef_h)
$(DUKE3D_OBJ)/gamestructures.$o: $(DUKE3D_SRC)/gamestructures.cpp $(duke3d_h) $(gamedef_h)
$(DUKE3D_OBJ)/gamevars.$o: $(DUKE3D_SRC)/gamevars.cpp $(DUKE3D_SRC)/gamestructures.cpp $(duke3d_h) $(gamedef_h) $(DUKE3D_SRC)/savegame.h
$(DUKE3D_OBJ)/global.$o: $(DUKE3D_SRC)/global.cpp $(duke3d_h)
$(DUKE3D_OBJ)/input.$o: $(DUKE3D_SRC)/input.cpp $(duke3d_h) $(DUKE3D_SRC)/input.h
$(DUKE3D_OBJ)/mdump.$o: $(DUKE3D_SRC)/mdump.cpp $(DUKE3D_SRC)/mdump.h
$(DUKE3D_OBJ)/menus.$o: $(DUKE3D_SRC)/menus.cpp $(duke3d_h) $(DUKE3D_SRC)/input.h $(MACT_INC)/mouse.h $(DUKE3D_SRC)/menus.h
$(DUKE3D_OBJ)/namesdyn.$o: $(DUKE3D_SRC)/namesdyn.cpp $(duke3d_h)
$(DUKE3D_OBJ)/net.$o: $(DUKE3D_SRC)/net.cpp $(duke3d_h)
$(DUKE3D_OBJ)/player.$o: $(DUKE3D_SRC)/player.cpp $(duke3d_h)
$(DUKE3D_OBJ)/premap.$o: $(DUKE3D_SRC)/premap.cpp $(duke3d_h) $(ENGINE_INC)/osd.h
$(DUKE3D_OBJ)/savegame.$o: $(DUKE3D_SRC)/savegame.cpp $(duke3d_h) $(DUKE3D_SRC)/savegame.h
$(DUKE3D_OBJ)/sbar.$o: $(DUKE3D_SRC)/sbar.cpp $(DUKE3D_SRC)/sbar.h
$(DUKE3D_OBJ)/screens.$o: $(DUKE3D_SRC)/screens.cpp $(DUKE3D_SRC)/screens.h
$(DUKE3D_OBJ)/screentext.$o: $(DUKE3D_SRC)/screentext.cpp $(DUKE3D_SRC)/screentext.h
$(DUKE3D_OBJ)/sector.$o: $(DUKE3D_SRC)/sector.cpp $(duke3d_h) $(DUKE3D_SRC)/input.h
$(DUKE3D_OBJ)/sounds.$o: $(DUKE3D_SRC)/sounds.cpp $(duke3d_h)
$(DUKE3D_OBJ)/soundsdyn.$o: $(DUKE3D_SRC)/soundsdyn.cpp $(duke3d_h)
$(DUKE3D_OBJ)/rts.$o: $(DUKE3D_SRC)/rts.cpp $(duke3d_h)
$(DUKE3D_OBJ)/config.$o: $(DUKE3D_SRC)/config.cpp $(duke3d_h) $(MACT_INC)/scriplib.h $(DUKE3D_SRC)/_functio.h
$(DUKE3D_OBJ)/winbits.$o: $(DUKE3D_SRC)/winbits.cpp
$(DUKE3D_OBJ)/osdfuncs.$o: $(DUKE3D_SRC)/names.h $(ENGINE_INC)/build.h $(ENGINE_INC)/osd.h
$(DUKE3D_OBJ)/osdcmds.$o: $(DUKE3D_SRC)/osdcmds.cpp $(DUKE3D_SRC)/osdcmds.h $(ENGINE_INC)/osd.h $(duke3d_h)

$(DUKE3D_OBJ)/lunatic_game.$o: $(ENGINE_INC)/lunatic.h $(DUKE3D_SRC)/lunatic/lunatic_game.cpp $(DUKE3D_SRC)/lunatic/lunatic_game.h $(DUKE3D_SRC)/gamedef.h $(DUKE3D_SRC)/gameexec.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/osd.h
$(DUKE3D_OBJ)/lunatic_editor.$o: $(ENGINE_INC)/lunatic.h $(DUKE3D_SRC)/lunatic/lunatic_editor.cpp $(DUKE3D_SRC)/lunatic/lunatic_editor.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/osd.h

# editor objects
m32_script_hs=$(ENGINE_INC)/m32script.h $(DUKE3D_SRC)/m32def.h
$(DUKE3D_OBJ)/astub.$o: $(DUKE3D_SRC)/astub.cpp \
    $(ENGINE_INC)/compat.h \
    $(ENGINE_INC)/build.h \
    $(ENGINE_INC)/editor.h \
    $(ENGINE_INC)/pragmas.h \
    $(ENGINE_INC)/baselayer.h \
    $(ENGINE_INC)/osd.h \
    $(ENGINE_INC)/cache1d.h \
    $(ENGINE_INC)/crc32.h \
    $(ENGINE_INC)/scriptfile.h \
    $(ENGINE_INC)/lz4.h \
    $(DUKE3D_SRC)/macros.h \
    $(DUKE3D_SRC)/osdfuncs.h \
    $(DUKE3D_SRC)/names.h \
    $(DUKE3D_SRC)/mapster32.h \
    $(DUKE3D_SRC)/keys.h \
    $(m32_script_hs)
$(DUKE3D_OBJ)/sounds_mapster32.$o: $(DUKE3D_SRC)/sounds_mapster32.cpp \
    $(DUKE3D_SRC)/sounds_mapster32.h \
    $(DUKE3D_SRC)/sounds_common.h \
    $(ENGINE_INC)/compat.h \
    $(ENGINE_INC)/baselayer.h \
    $(ENGINE_INC)/cache1d.h \
    $(ENGINE_INC)/build.h \
    $(ENGINE_INC)/editor.h \
    $(ENGINE_INC)/osd.h \
    $(DUKE3D_SRC)/macros.h \
    $(AUDIOLIB_INC)/fx_man.h \

$(DUKE3D_OBJ)/m32def.$o: $(DUKE3D_SRC)/m32def.cpp $(m32_script_hs) $(ENGINE_INC)/cache1d.h $(DUKE3D_SRC)/sounds_mapster32.h $(DUKE3D_SRC)/sounds_common.h $(DUKE3D_SRC)/keys.h
$(DUKE3D_OBJ)/m32exec.$o: $(DUKE3D_SRC)/m32exec.cpp $(m32_script_hs) $(DUKE3D_SRC)/sounds_mapster32.h $(DUKE3D_SRC)/sounds_common.h $(ENGINE_INC)/osd.h $(DUKE3D_SRC)/keys.h $(AUDIOLIB_INC)/fx_man.h
$(DUKE3D_OBJ)/m32structures.$o: $(DUKE3D_SRC)/m32structures.cpp $(m32_script_hs) $(ENGINE_INC)/compat.h $(ENGINE_INC)/prlights.h
$(DUKE3D_OBJ)/m32vars.$o: $(DUKE3D_SRC)/m32vars.cpp $(DUKE3D_SRC)/m32structures.cpp $(m32_script_hs) $(ENGINE_INC)/osd.h $(DUKE3D_SRC)/keys.h $(ENGINE_INC)/polymer.h
# TODO: m32common.cpp

# misc objects
$(DUKE3D_OBJ)/game_icon.$o: $(DUKE3D_RSRC)/game_icon.c $(DUKE3D_RSRC)/game_icon.ico
$(DUKE3D_OBJ)/build_icon.$o: $(DUKE3D_RSRC)/build_icon.c $(DUKE3D_RSRC)/build_icon.ico

$(DUKE3D_OBJ)/grpscan.$o: $(DUKE3D_SRC)/grpscan.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/scriptfile.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/crc32.h $(DUKE3D_SRC)/grpscan.h
$(DUKE3D_OBJ)/gameres.$o: $(DUKE3D_RSRC)/gameres.rc $(DUKE3D_SRC)/startwin.game.h $(DUKE3D_RSRC)/game.bmp
$(DUKE3D_OBJ)/buildres.$o: $(DUKE3D_RSRC)/buildres.rc $(ENGINE_INC)/startwin.editor.h $(DUKE3D_RSRC)/build.bmp
$(DUKE3D_OBJ)/startwin.game.$o: $(DUKE3D_SRC)/startwin.game.cpp $(duke3d_h) $(ENGINE_INC)/build.h $(ENGINE_INC)/winlayer.h $(ENGINE_INC)/compat.h $(DUKE3D_SRC)/grpscan.h
$(DUKE3D_OBJ)/startgtk.game.$o: $(DUKE3D_SRC)/startgtk.game.cpp $(duke3d_h) $(ENGINE_INC)/dynamicgtk.h $(ENGINE_INC)/build.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/compat.h


# MACT objects
$(MACT_OBJ)/animlib.$o: $(MACT_SRC)/animlib.cpp $(MACT_INC)/animlib.h $(ENGINE_INC)/compat.h
$(MACT_OBJ)/file_lib.$o: $(MACT_SRC)/file_lib.cpp $(MACT_INC)/file_lib.h
$(MACT_OBJ)/control.$o: $(MACT_SRC)/control.cpp $(MACT_INC)/control.h $(MACT_INC)/keyboard.h $(MACT_INC)/mouse.h $(MACT_INC)/joystick.h $(ENGINE_INC)/baselayer.h
$(MACT_OBJ)/keyboard.$o: $(MACT_SRC)/keyboard.cpp $(MACT_INC)/keyboard.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/baselayer.h
$(MACT_OBJ)/joystick.$o: $(MACT_SRC)/joystick.cpp $(MACT_INC)/joystick.h $(ENGINE_INC)/baselayer.h
$(MACT_OBJ)/scriplib.$o: $(MACT_SRC)/scriplib.cpp $(MACT_INC)/scriplib.h $(MACT_SRC)/_scrplib.h $(ENGINE_INC)/compat.h

$(DUKE3D_OBJ)/midi.$o: $(DUKE3D_SRC)/midi.cpp $(DUKE3D_SRC)/_midi.h $(DUKE3D_SRC)/midi.h $(AUDIOLIB_INC)/music.h
$(DUKE3D_OBJ)/mpu401.$o: $(DUKE3D_SRC)/mpu401.cpp $(DUKE3D_SRC)/mpu401.h $(AUDIOLIB_INC)/music.h
$(DUKE3D_OBJ)/music.$o: $(DUKE3D_SRC)/music.cpp $(DUKE3D_SRC)/midi.h $(DUKE3D_SRC)/mpu401.h $(AUDIOLIB_INC)/music.h
