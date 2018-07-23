duke3d_h=\
    $(engine_inc)/compat.h \
    $(engine_inc)/pragmas.h \
    $(engine_inc)/build.h \
    $(engine_inc)/baselayer.h \
    $(engine_inc)/polymer.h \
    $(engine_inc)/polymost.h \
    $(engine_inc)/texcache.h \
    $(engine_inc)/cache1d.h \
    $(mact_inc)/file_lib.h \
    $(mact_inc)/keyboard.h \
    $(mact_inc)/mouse.h \
    $(mact_inc)/joystick.h \
    $(mact_inc)/control.h \
    $(duke3d_src)/macros.h \
    $(duke3d_src)/gamedefs.h \
    $(duke3d_src)/function.h \
    $(duke3d_src)/config.h \
    $(duke3d_src)/sounds.h \
    $(duke3d_src)/sounds_common.h \
    $(duke3d_src)/soundsdyn.h \
    $(duke3d_src)/rts.h \
    $(duke3d_src)/_rts.h \
    $(duke3d_src)/soundefs.h \
    $(audiolib_inc)/fx_man.h \
    $(audiolib_inc)/music.h \
    $(duke3d_src)/namesdyn.h \
    $(duke3d_src)/duke3d.h \
    $(duke3d_src)/player.h \
    $(duke3d_src)/sector.h \
    $(duke3d_src)/game.h \
    $(duke3d_src)/actors.h \
    $(duke3d_src)/menus.h

gamedef_h=$(duke3d_src)/gamedef.h $(duke3d_src)/gameexec.h $(duke3d_src)/gamevars.h

game_defs_dep=$(duke3d_src)/lunatic/_defs_game.lua

$(duke3d_obj)/game.$o: $(duke3d_src)/game.cpp $(game_defs_dep) $(mact_inc)/scriplib.h $(duke3d_h) $(duke3d_src)/input.h $(duke3d_src)/osdfuncs.h $(duke3d_src)/osdcmds.h $(duke3d_src)/grpscan.h $(duke3d_src)/demo.h $(engine_inc)/hightile.h
$(duke3d_obj)/actors.$o: $(duke3d_src)/actors.cpp $(duke3d_h)
$(duke3d_obj)/anim.$o: $(duke3d_src)/anim.cpp $(duke3d_h) $(duke3d_src)/input.h $(mact_inc)/animlib.h $(engine_inc)/animvpx.h
$(duke3d_obj)/cheats.$o: $(duke3d_src)/cheats.cpp $(duke3d_src)/cheats.h
$(duke3d_obj)/cmdline.$o: $(duke3d_src)/cmdline.cpp $(duke3d_src)/cmdline.h
$(duke3d_obj)/demo.$o: $(duke3d_src)/demo.cpp $(duke3d_h) $(duke3d_src)/input.h
$(duke3d_obj)/gamedef.$o: $(duke3d_src)/gamedef.cpp $(duke3d_h) $(gamedef_h) $(duke3d_src)/savegame.h
$(duke3d_obj)/gameexec.$o: $(duke3d_src)/gameexec.cpp $(duke3d_src)/gamestructures.cpp $(duke3d_h) $(gamedef_h)
$(duke3d_obj)/gamestructures.$o: $(duke3d_src)/gamestructures.cpp $(duke3d_h) $(gamedef_h)
$(duke3d_obj)/gamevars.$o: $(duke3d_src)/gamevars.cpp $(duke3d_src)/gamestructures.cpp $(duke3d_h) $(gamedef_h) $(duke3d_src)/savegame.h
$(duke3d_obj)/global.$o: $(duke3d_src)/global.cpp $(duke3d_h)
$(duke3d_obj)/input.$o: $(duke3d_src)/input.cpp $(duke3d_h) $(duke3d_src)/input.h
$(duke3d_obj)/mdump.$o: $(duke3d_src)/mdump.cpp $(duke3d_src)/mdump.h
$(duke3d_obj)/menus.$o: $(duke3d_src)/menus.cpp $(duke3d_h) $(duke3d_src)/input.h $(mact_inc)/mouse.h $(duke3d_src)/menus.h
$(duke3d_obj)/namesdyn.$o: $(duke3d_src)/namesdyn.cpp $(duke3d_h)
$(duke3d_obj)/net.$o: $(duke3d_src)/net.cpp $(duke3d_h)
$(duke3d_obj)/player.$o: $(duke3d_src)/player.cpp $(duke3d_h)
$(duke3d_obj)/premap.$o: $(duke3d_src)/premap.cpp $(duke3d_h) $(engine_inc)/osd.h
$(duke3d_obj)/savegame.$o: $(duke3d_src)/savegame.cpp $(duke3d_h) $(duke3d_src)/savegame.h
$(duke3d_obj)/sbar.$o: $(duke3d_src)/sbar.cpp $(duke3d_src)/sbar.h
$(duke3d_obj)/screens.$o: $(duke3d_src)/screens.cpp $(duke3d_src)/screens.h
$(duke3d_obj)/screentext.$o: $(duke3d_src)/screentext.cpp $(duke3d_src)/screentext.h
$(duke3d_obj)/sector.$o: $(duke3d_src)/sector.cpp $(duke3d_h) $(duke3d_src)/input.h
$(duke3d_obj)/sounds.$o: $(duke3d_src)/sounds.cpp $(duke3d_h)
$(duke3d_obj)/soundsdyn.$o: $(duke3d_src)/soundsdyn.cpp $(duke3d_h)
$(duke3d_obj)/rts.$o: $(duke3d_src)/rts.cpp $(duke3d_h)
$(duke3d_obj)/config.$o: $(duke3d_src)/config.cpp $(duke3d_h) $(mact_inc)/scriplib.h $(duke3d_src)/_functio.h
$(duke3d_obj)/winbits.$o: $(duke3d_src)/winbits.cpp
$(duke3d_obj)/osdfuncs.$o: $(duke3d_src)/names.h $(engine_inc)/build.h $(engine_inc)/osd.h
$(duke3d_obj)/osdcmds.$o: $(duke3d_src)/osdcmds.cpp $(duke3d_src)/osdcmds.h $(engine_inc)/osd.h $(duke3d_h)

$(duke3d_obj)/lunatic_game.$o: $(engine_inc)/lunatic.h $(duke3d_src)/lunatic/lunatic_game.cpp $(duke3d_src)/lunatic/lunatic_game.h $(duke3d_src)/gamedef.h $(duke3d_src)/gameexec.h $(engine_inc)/cache1d.h $(engine_inc)/osd.h
$(duke3d_obj)/lunatic_editor.$o: $(engine_inc)/lunatic.h $(duke3d_src)/lunatic/lunatic_editor.cpp $(duke3d_src)/lunatic/lunatic_editor.h $(engine_inc)/cache1d.h $(engine_inc)/osd.h

# editor objects
m32_script_hs=$(engine_inc)/m32script.h $(duke3d_src)/m32def.h
$(duke3d_obj)/astub.$o: $(duke3d_src)/astub.cpp \
    $(engine_inc)/compat.h \
    $(engine_inc)/build.h \
    $(engine_inc)/editor.h \
    $(engine_inc)/pragmas.h \
    $(engine_inc)/baselayer.h \
    $(engine_inc)/osd.h \
    $(engine_inc)/cache1d.h \
    $(engine_inc)/crc32.h \
    $(engine_inc)/scriptfile.h \
    $(engine_inc)/lz4.h \
    $(duke3d_src)/macros.h \
    $(duke3d_src)/osdfuncs.h \
    $(duke3d_src)/names.h \
    $(duke3d_src)/mapster32.h \
    $(duke3d_src)/keys.h \
    $(m32_script_hs)
$(duke3d_obj)/sounds_mapster32.$o: $(duke3d_src)/sounds_mapster32.cpp \
    $(duke3d_src)/sounds_mapster32.h \
    $(duke3d_src)/sounds_common.h \
    $(engine_inc)/compat.h \
    $(engine_inc)/baselayer.h \
    $(engine_inc)/cache1d.h \
    $(engine_inc)/build.h \
    $(engine_inc)/editor.h \
    $(engine_inc)/osd.h \
    $(duke3d_src)/macros.h \
    $(audiolib_inc)/fx_man.h \

$(duke3d_obj)/m32def.$o: $(duke3d_src)/m32def.cpp $(m32_script_hs) $(engine_inc)/cache1d.h $(duke3d_src)/sounds_mapster32.h $(duke3d_src)/sounds_common.h $(duke3d_src)/keys.h
$(duke3d_obj)/m32exec.$o: $(duke3d_src)/m32exec.cpp $(m32_script_hs) $(duke3d_src)/sounds_mapster32.h $(duke3d_src)/sounds_common.h $(engine_inc)/osd.h $(duke3d_src)/keys.h $(audiolib_inc)/fx_man.h
$(duke3d_obj)/m32structures.$o: $(duke3d_src)/m32structures.cpp $(m32_script_hs) $(engine_inc)/compat.h $(engine_inc)/prlights.h
$(duke3d_obj)/m32vars.$o: $(duke3d_src)/m32vars.cpp $(duke3d_src)/m32structures.cpp $(m32_script_hs) $(engine_inc)/osd.h $(duke3d_src)/keys.h $(engine_inc)/polymer.h
# TODO: m32common.cpp

# misc objects
$(duke3d_obj)/game_icon.$o: $(duke3d_rsrc)/game_icon.c $(duke3d_rsrc)/game_icon.ico
$(duke3d_obj)/build_icon.$o: $(duke3d_rsrc)/build_icon.c $(duke3d_rsrc)/build_icon.ico

$(duke3d_obj)/grpscan.$o: $(duke3d_src)/grpscan.cpp $(engine_inc)/compat.h $(engine_inc)/baselayer.h $(engine_inc)/scriptfile.h $(engine_inc)/cache1d.h $(engine_inc)/crc32.h $(duke3d_src)/grpscan.h
$(duke3d_obj)/gameres.$o: $(duke3d_rsrc)/gameres.rc $(duke3d_src)/startwin.game.h $(duke3d_rsrc)/game.bmp
$(duke3d_obj)/buildres.$o: $(duke3d_rsrc)/buildres.rc $(engine_inc)/startwin.editor.h $(duke3d_rsrc)/build.bmp
$(duke3d_obj)/startwin.game.$o: $(duke3d_src)/startwin.game.cpp $(duke3d_h) $(engine_inc)/build.h $(engine_inc)/winlayer.h $(engine_inc)/compat.h $(duke3d_src)/grpscan.h
$(duke3d_obj)/startgtk.game.$o: $(duke3d_src)/startgtk.game.cpp $(duke3d_h) $(engine_inc)/dynamicgtk.h $(engine_inc)/build.h $(engine_inc)/baselayer.h $(engine_inc)/compat.h


# mact objects
$(mact_obj)/animlib.$o: $(mact_src)/animlib.cpp $(mact_inc)/animlib.h $(engine_inc)/compat.h
$(mact_obj)/file_lib.$o: $(mact_src)/file_lib.cpp $(mact_inc)/file_lib.h
$(mact_obj)/control.$o: $(mact_src)/control.cpp $(mact_inc)/control.h $(mact_inc)/keyboard.h $(mact_inc)/mouse.h $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/keyboard.$o: $(mact_src)/keyboard.cpp $(mact_inc)/keyboard.h $(engine_inc)/compat.h $(engine_inc)/baselayer.h
$(mact_obj)/joystick.$o: $(mact_src)/joystick.cpp $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/scriplib.$o: $(mact_src)/scriplib.cpp $(mact_inc)/scriplib.h $(mact_src)/_scrplib.h $(engine_inc)/compat.h

$(duke3d_obj)/midi.$o: $(duke3d_src)/midi.cpp $(duke3d_src)/_midi.h $(duke3d_src)/midi.h $(audiolib_inc)/music.h
$(duke3d_obj)/mpu401.$o: $(duke3d_src)/mpu401.cpp $(duke3d_src)/mpu401.h $(audiolib_inc)/music.h
$(duke3d_obj)/music.$o: $(duke3d_src)/music.cpp $(duke3d_src)/midi.h $(duke3d_src)/mpu401.h $(audiolib_inc)/music.h
