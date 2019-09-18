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
    $(rr_src)/macros.h \
    $(rr_src)/gamedefs.h \
    $(rr_src)/function.h \
    $(rr_src)/config.h \
    $(rr_src)/sounds.h \
    $(rr_src)/sounds_common.h \
    $(rr_src)/soundsdyn.h \
    $(rr_src)/rts.h \
    $(rr_src)/_rts.h \
    $(rr_src)/soundefs.h \
    $(audiolib_inc)/fx_man.h \
    $(audiolib_inc)/music.h \
    $(rr_src)/namesdyn.h \
    $(rr_src)/duke3d.h \
    $(rr_src)/player.h \
    $(rr_src)/sector.h \
    $(rr_src)/game.h \
    $(rr_src)/actors.h \
    $(rr_src)/menus.h

gamedef_h=$(rr_src)/gamedef.h $(rr_src)/gameexec.h $(rr_src)/gamevars.h

game_defs_dep=$(rr_src)/lunatic/_defs_game.lua

$(rr_obj)/game.$o: $(rr_src)/game.cpp $(game_defs_dep) $(mact_inc)/scriplib.h $(rr_h) $(rr_src)/input.h $(rr_src)/osdfuncs.h $(rr_src)/osdcmds.h $(rr_src)/grpscan.h $(rr_src)/demo.h $(engine_inc)/hightile.h
$(rr_obj)/actors.$o: $(rr_src)/actors.cpp $(rr_h)
$(rr_obj)/anim.$o: $(rr_src)/anim.cpp $(rr_h) $(rr_src)/input.h $(mact_inc)/animlib.h $(engine_inc)/animvpx.h
$(rr_obj)/cheats.$o: $(rr_src)/cheats.cpp $(rr_src)/cheats.h
$(rr_obj)/cmdline.$o: $(rr_src)/cmdline.cpp $(rr_src)/cmdline.h
$(rr_obj)/demo.$o: $(rr_src)/demo.cpp $(rr_h) $(rr_src)/input.h
$(rr_obj)/gamedef.$o: $(rr_src)/gamedef.cpp $(rr_h) $(gamedef_h) $(rr_src)/savegame.h
$(rr_obj)/gameexec.$o: $(rr_src)/gameexec.cpp $(rr_src)/gamestructures.cpp $(rr_h) $(gamedef_h)
$(rr_obj)/gamestructures.$o: $(rr_src)/gamestructures.cpp $(rr_h) $(gamedef_h)
$(rr_obj)/gamevars.$o: $(rr_src)/gamevars.cpp $(rr_src)/gamestructures.cpp $(rr_h) $(gamedef_h) $(rr_src)/savegame.h
$(rr_obj)/global.$o: $(rr_src)/global.cpp $(rr_h)
$(rr_obj)/input.$o: $(rr_src)/input.cpp $(rr_h) $(rr_src)/input.h
$(rr_obj)/mdump.$o: $(rr_src)/mdump.cpp $(rr_src)/mdump.h
$(rr_obj)/menus.$o: $(rr_src)/menus.cpp $(rr_h) $(rr_src)/input.h $(mact_inc)/mouse.h $(rr_src)/menus.h
$(rr_obj)/namesdyn.$o: $(rr_src)/namesdyn.cpp $(rr_h)
$(rr_obj)/net.$o: $(rr_src)/net.cpp $(rr_h)
$(rr_obj)/player.$o: $(rr_src)/player.cpp $(rr_h)
$(rr_obj)/premap.$o: $(rr_src)/premap.cpp $(rr_h) $(engine_inc)/osd.h
$(rr_obj)/savegame.$o: $(rr_src)/savegame.cpp $(rr_h) $(rr_src)/savegame.h
$(rr_obj)/sbar.$o: $(rr_src)/sbar.cpp $(rr_src)/sbar.h
$(rr_obj)/screens.$o: $(rr_src)/screens.cpp $(rr_src)/screens.h
$(rr_obj)/screentext.$o: $(rr_src)/screentext.cpp $(rr_src)/screentext.h
$(rr_obj)/sector.$o: $(rr_src)/sector.cpp $(rr_h) $(rr_src)/input.h
$(rr_obj)/sounds.$o: $(rr_src)/sounds.cpp $(rr_h)
$(rr_obj)/soundsdyn.$o: $(rr_src)/soundsdyn.cpp $(rr_h)
$(rr_obj)/rts.$o: $(rr_src)/rts.cpp $(rr_h)
$(rr_obj)/config.$o: $(rr_src)/config.cpp $(rr_h) $(mact_inc)/scriplib.h $(rr_src)/_functio.h
$(rr_obj)/winbits.$o: $(rr_src)/winbits.cpp
$(rr_obj)/osdfuncs.$o: $(rr_src)/names.h $(engine_inc)/build.h $(engine_inc)/osd.h
$(rr_obj)/osdcmds.$o: $(rr_src)/osdcmds.cpp $(rr_src)/osdcmds.h $(engine_inc)/osd.h $(rr_h)

$(rr_obj)/lunatic_game.$o: $(engine_inc)/lunatic.h $(rr_src)/lunatic/lunatic_game.cpp $(rr_src)/lunatic/lunatic_game.h $(rr_src)/gamedef.h $(rr_src)/gameexec.h $(engine_inc)/cache1d.h $(engine_inc)/osd.h
$(rr_obj)/lunatic_editor.$o: $(engine_inc)/lunatic.h $(rr_src)/lunatic/lunatic_editor.cpp $(rr_src)/lunatic/lunatic_editor.h $(engine_inc)/cache1d.h $(engine_inc)/osd.h

# misc objects
$(rr_obj)/game_icon.$o: $(rr_rsrc)/game_icon.c $(rr_rsrc)/game_icon.ico
$(rr_obj)/build_icon.$o: $(rr_rsrc)/build_icon.c $(rr_rsrc)/build_icon.ico

$(rr_obj)/grpscan.$o: $(rr_src)/grpscan.cpp $(engine_inc)/compat.h $(engine_inc)/baselayer.h $(engine_inc)/scriptfile.h $(engine_inc)/cache1d.h $(engine_inc)/crc32.h $(rr_src)/grpscan.h
$(rr_obj)/gameres.$o: $(rr_rsrc)/gameres.rc $(rr_src)/startwin.game.h $(rr_rsrc)/game.bmp
$(rr_obj)/buildres.$o: $(rr_rsrc)/buildres.rc $(engine_inc)/startwin.editor.h $(rr_rsrc)/build.bmp
$(rr_obj)/startwin.game.$o: $(rr_src)/startwin.game.cpp $(rr_h) $(engine_inc)/build.h $(engine_inc)/winlayer.h $(engine_inc)/compat.h $(rr_src)/grpscan.h
$(rr_obj)/startgtk.game.$o: $(rr_src)/startgtk.game.cpp $(rr_h) $(engine_inc)/dynamicgtk.h $(engine_inc)/build.h $(engine_inc)/baselayer.h $(engine_inc)/compat.h


# mact objects
$(mact_obj)/animlib.$o: $(mact_src)/animlib.cpp $(mact_inc)/animlib.h $(engine_inc)/compat.h
$(mact_obj)/file_lib.$o: $(mact_src)/file_lib.cpp $(mact_inc)/file_lib.h
$(mact_obj)/control.$o: $(mact_src)/control.cpp $(mact_inc)/control.h $(mact_inc)/keyboard.h $(mact_inc)/mouse.h $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/keyboard.$o: $(mact_src)/keyboard.cpp $(mact_inc)/keyboard.h $(engine_inc)/compat.h $(engine_inc)/baselayer.h
$(mact_obj)/joystick.$o: $(mact_src)/joystick.cpp $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/scriplib.$o: $(mact_src)/scriplib.cpp $(mact_inc)/scriplib.h $(mact_src)/_scrplib.h $(engine_inc)/compat.h

$(rr_obj)/midi.$o: $(rr_src)/midi.cpp $(rr_src)/_midi.h $(rr_src)/midi.h $(audiolib_inc)/music.h
$(rr_obj)/mpu401.$o: $(rr_src)/mpu401.cpp $(rr_src)/mpu401.h $(audiolib_inc)/music.h
$(rr_obj)/music.$o: $(rr_src)/music.cpp $(rr_src)/midi.h $(rr_src)/mpu401.h $(audiolib_inc)/music.h
