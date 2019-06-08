ai_h=\
	$(blood_src)/aibat.h \
	$(blood_src)/aibeast.h \
	$(blood_src)/aiboneel.h \
	$(blood_src)/aiburn.h \
	$(blood_src)/aicaleb.h \
	$(blood_src)/aicerber.h \
	$(blood_src)/aicult.h \
	$(blood_src)/aigarg.h \
	$(blood_src)/aighost.h \
	$(blood_src)/aigilbst.h \
	$(blood_src)/aihand.h \
	$(blood_src)/aihound.h \
	$(blood_src)/aiinnoc.h \
	$(blood_src)/aipod.h \
	$(blood_src)/airat.h \
	$(blood_src)/aispid.h \
	$(blood_src)/aitchern.h \
	$(blood_src)/aiunicult.h \
	$(blood_src)/aizomba.h \
	$(blood_src)/aizombf.h
	
common_h=\
    $(engine_inc)/compat.h \
    $(engine_inc)/common.h \
    $(engine_inc)/pragmas.h \
    $(engine_inc)/build.h \
    $(engine_inc)/baselayer.h \
    $(engine_inc)/palette.h \
    $(engine_inc)/polymer.h \
    $(engine_inc)/polymost.h \
    $(engine_inc)/texcache.h \
    $(engine_inc)/cache1d.h \
    $(mact_inc)/file_lib.h \
    $(mact_inc)/keyboard.h \
    $(mact_inc)/mouse.h \
    $(mact_inc)/joystick.h \
    $(mact_inc)/control.h \
    $(audiolib_inc)/fx_man.h \
    $(audiolib_inc)/music.h \
    $(blood_src)/common_game.h \
    $(blood_src)/blood.h \
    $(blood_src)/actor.h \
    $(blood_src)/ai.h \
    $(blood_src)/asound.h \
    $(blood_src)/callback.h \
    $(blood_src)/choke.h \
    $(blood_src)/config.h \
    $(blood_src)/controls.h \
    $(blood_src)/credits.h \
    $(blood_src)/db.h \
    $(blood_src)/demo.h \
    $(blood_src)/dude.h \
    $(blood_src)/endgame.h \
    $(blood_src)/eventq.h \
    $(blood_src)/fire.h \
    $(blood_src)/function.h \
    $(blood_src)/fx.h \
    $(blood_src)/gamedefs.h \
    $(blood_src)/gamemenu.h \
    $(blood_src)/getopt.h \
    $(blood_src)/gib.h \
    $(blood_src)/globals.h \
    $(blood_src)/inifile.h \
    $(blood_src)/iob.h \
    $(blood_src)/levels.h \
    $(blood_src)/loadsave.h \
    $(blood_src)/map2d.h \
    $(blood_src)/menu.h \
    $(blood_src)/messages.h \
    $(blood_src)/mirrors.h \
    $(blood_src)/misc.h \
    $(blood_src)/network.h \
    $(blood_src)/osdcmds.h \
    $(blood_src)/player.h \
    $(blood_src)/pqueue.h \
    $(blood_src)/qav.h \
    $(blood_src)/qheap.h \
    $(blood_src)/replace.h \
    $(blood_src)/resource.h \
    $(blood_src)/screen.h \
    $(blood_src)/sectorfx.h \
    $(blood_src)/seq.h \
    $(blood_src)/sfx.h \
    $(blood_src)/sound.h \
    $(blood_src)/tile.h \
    $(blood_src)/trig.h \
    $(blood_src)/triggers.h \
    $(blood_src)/view.h \
    $(blood_src)/warp.h \
    $(blood_src)/tile.h

$(blood_obj)/blood.$o: $(blood_src)/blood.cpp $(common_h)
$(blood_obj)/actor.$o: $(blood_src)/actor.cpp $(common_h) $(ai_h)
$(blood_obj)/ai.$o: $(blood_src)/ai.cpp $(common_h) $(ai_h)
$(blood_obj)/aibat.$o: $(blood_src)/aibat.cpp $(common_h) $(ai_h)
$(blood_obj)/aibeast.$o: $(blood_src)/aibeast.cpp $(common_h) $(ai_h)
$(blood_obj)/aiboneel.$o: $(blood_src)/aiboneel.cpp $(common_h) $(ai_h)
$(blood_obj)/aiburn.$o: $(blood_src)/aiburn.cpp $(common_h) $(ai_h)
$(blood_obj)/aicaleb.$o: $(blood_src)/aicaleb.cpp $(common_h) $(ai_h)
$(blood_obj)/aicerber.$o: $(blood_src)/aicerber.cpp $(common_h) $(ai_h)
$(blood_obj)/aicult.$o: $(blood_src)/aicult.cpp $(common_h) $(ai_h)
$(blood_obj)/aigarg.$o: $(blood_src)/aigarg.cpp $(common_h) $(ai_h)
$(blood_obj)/aighost.$o: $(blood_src)/aighost.cpp $(common_h) $(ai_h)
$(blood_obj)/aigilbst.$o: $(blood_src)/aigilbst.cpp $(common_h) $(ai_h)
$(blood_obj)/aihand.$o: $(blood_src)/aihand.cpp $(common_h) $(ai_h)
$(blood_obj)/aihound.$o: $(blood_src)/aihound.cpp $(common_h) $(ai_h)
$(blood_obj)/aiinnoc.$o: $(blood_src)/aiinnoc.cpp $(common_h) $(ai_h)
$(blood_obj)/aipod.$o: $(blood_src)/aipod.cpp $(common_h) $(ai_h)
$(blood_obj)/airat.$o: $(blood_src)/airat.cpp $(common_h) $(ai_h)
$(blood_obj)/aispid.$o: $(blood_src)/aispid.cpp $(common_h) $(ai_h)
$(blood_obj)/aitchern.$o: $(blood_src)/aitchern.cpp $(common_h) $(ai_h)
$(blood_obj)/aiunicult.$o: $(blood_src)/aiunicult.cpp $(common_h) $(ai_h)
$(blood_obj)/aizomba.$o: $(blood_src)/aizomba.cpp $(common_h) $(ai_h)
$(blood_obj)/aizombf.$o: $(blood_src)/aizombf.cpp $(common_h) $(ai_h)
$(blood_obj)/asound.$o: $(blood_src)/asound.cpp $(common_h)
$(blood_obj)/callback.$o: $(blood_src)/callback.cpp $(common_h)
$(blood_obj)/choke.$o: $(blood_src)/choke.cpp $(common_h)
$(blood_obj)/common.$o: $(blood_src)/common.cpp $(common_h)
$(blood_obj)/config.$o: $(blood_src)/config.cpp $(common_h)
$(blood_obj)/controls.$o: $(blood_src)/controls.cpp $(common_h)
$(blood_obj)/credits.$o: $(blood_src)/credits.cpp $(common_h)
$(blood_obj)/db.$o: $(blood_src)/db.cpp $(common_h)
$(blood_obj)/demo.$o: $(blood_src)/demo.cpp $(common_h)
$(blood_obj)/dude.$o: $(blood_src)/dude.cpp $(common_h)
$(blood_obj)/endgame.$o: $(blood_src)/endgame.cpp $(common_h)
$(blood_obj)/eventq.$o: $(blood_src)/eventq.cpp $(common_h)
$(blood_obj)/fire.$o: $(blood_src)/fire.cpp $(common_h)
$(blood_obj)/fx.$o: $(blood_src)/fx.cpp $(common_h)
$(blood_obj)/gamemenu.$o: $(blood_src)/gamemenu.cpp $(common_h)
$(blood_obj)/gameutil.$o: $(blood_src)/gameutil.cpp $(common_h)
$(blood_obj)/getopt.$o: $(blood_src)/getopt.cpp $(common_h)
$(blood_obj)/gib.$o: $(blood_src)/gib.cpp $(common_h)
$(blood_obj)/globals.$o: $(blood_src)/globals.cpp $(common_h)
$(blood_obj)/inifile.$o: $(blood_src)/inifile.cpp $(common_h)
$(blood_obj)/iob.$o: $(blood_src)/iob.cpp $(common_h)
$(blood_obj)/levels.$o: $(blood_src)/levels.cpp $(common_h)
$(blood_obj)/loadsave.$o: $(blood_src)/loadsave.cpp $(common_h)
$(blood_obj)/map2d.$o: $(blood_src)/map2d.cpp $(common_h)
$(blood_obj)/menu.$o: $(blood_src)/menu.cpp $(common_h)
$(blood_obj)/messages.$o: $(blood_src)/messages.cpp $(common_h)
$(blood_obj)/mirrors.$o: $(blood_src)/mirrors.cpp $(common_h)
$(blood_obj)/misc.$o: $(blood_src)/misc.cpp $(common_h)
$(blood_obj)/network.$o: $(blood_src)/network.cpp $(common_h)
$(blood_obj)/osdcmd.$o: $(blood_src)/osdcmd.cpp $(common_h)
$(blood_obj)/player.$o: $(blood_src)/player.cpp $(common_h)
$(blood_obj)/pqueue.$o: $(blood_src)/pqueue.cpp $(common_h)
$(blood_obj)/qav.$o: $(blood_src)/qav.cpp $(common_h)
$(blood_obj)/qheap.$o: $(blood_src)/qheap.cpp $(common_h)
$(blood_obj)/replace.$o: $(blood_src)/replace.cpp $(common_h)
$(blood_obj)/resource.$o: $(blood_src)/resource.cpp $(common_h)
$(blood_obj)/screen.$o: $(blood_src)/screen.cpp $(common_h)
$(blood_obj)/sectorfx.$o: $(blood_src)/sectorfx.cpp $(common_h)
$(blood_obj)/seq.$o: $(blood_src)/seq.cpp $(common_h)
$(blood_obj)/sfx.$o: $(blood_src)/sfx.cpp $(common_h)
$(blood_obj)/sound.$o: $(blood_src)/sound.cpp $(common_h)
$(blood_obj)/tile.$o: $(blood_src)/tile.cpp $(common_h)
$(blood_obj)/trig.$o: $(blood_src)/trig.cpp $(common_h)
$(blood_obj)/triggers.$o: $(blood_src)/triggers.cpp $(common_h)
$(blood_obj)/view.$o: $(blood_src)/view.cpp $(common_h) $(ai_h)
$(blood_obj)/warp.$o: $(blood_src)/warp.cpp $(common_h)
$(blood_obj)/weapon.$o: $(blood_src)/weapon.cpp $(common_h)
$(blood_obj)/winbits.$o: $(blood_src)/winbits.cpp

# misc objects
$(blood_obj)/game_icon.$o: $(blood_rsrc)/game_icon.c $(blood_rsrc)/game_icon.ico

$(blood_obj)/gameres.$o: $(blood_rsrc)/gameres.rc $(blood_src)/startwin.game.h $(blood_rsrc)/game.bmp
$(blood_obj)/buildres.$o: $(blood_rsrc)/buildres.rc $(engine_inc)/startwin.editor.h $(blood_rsrc)/build.bmp
$(blood_obj)/startwin.game.$o: $(blood_src)/startwin.game.cpp $(blood_h) $(engine_inc)/build.h $(engine_inc)/winlayer.h $(engine_inc)/compat.h
$(blood_obj)/startgtk.game.$o: $(blood_src)/startgtk.game.cpp $(blood_h) $(engine_inc)/dynamicgtk.h $(engine_inc)/build.h $(engine_inc)/baselayer.h $(engine_inc)/compat.h

# mact objects
$(mact_obj)/animlib.$o: $(mact_src)/animlib.cpp $(mact_inc)/animlib.h $(engine_inc)/compat.h
$(mact_obj)/file_lib.$o: $(mact_src)/file_lib.cpp $(mact_inc)/file_lib.h
$(mact_obj)/control.$o: $(mact_src)/control.cpp $(mact_inc)/control.h $(mact_inc)/keyboard.h $(mact_inc)/mouse.h $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/keyboard.$o: $(mact_src)/keyboard.cpp $(mact_inc)/keyboard.h $(engine_inc)/compat.h $(engine_inc)/baselayer.h
$(mact_obj)/joystick.$o: $(mact_src)/joystick.cpp $(mact_inc)/joystick.h $(engine_inc)/baselayer.h
$(mact_obj)/scriplib.$o: $(mact_src)/scriplib.cpp $(mact_inc)/scriplib.h $(mact_src)/_scrplib.h $(engine_inc)/compat.h
