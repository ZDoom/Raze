
set( PCH_SOURCES
	src/2d_d.cpp
	src/2d_r.cpp
	src/actors.cpp
	src/actors_r.cpp
	src/actors_d.cpp
	src/actors_lava.cpp
	src/animatesprites_r.cpp
	src/animatesprites_d.cpp
	src/bowling.cpp
	src/ccmds.cpp
	src/cheats.cpp
	src/d_menu.cpp
	src/dispatch.cpp
	src/flags_d.cpp
	src/flags_r.cpp
	src/game.cpp
	src/game_misc.cpp
	src/gamedef.cpp
	src/gameexec.cpp
	src/gameloop.cpp
	src/gamevar.cpp
	src/hudweapon_d.cpp
	src/hudweapon_r.cpp
	src/input.cpp
	src/interpolate.cpp
	src/noise.cpp
	src/player.cpp
	src/player_d.cpp
	src/player_r.cpp
	src/player_w.cpp
	src/premap.cpp
	src/premap_d.cpp
	src/premap_r.cpp
	src/prediction.cpp
	src/render.cpp
	src/sbar.cpp
	src/sbar_d.cpp
	src/sbar_r.cpp
	src/sectors.cpp
	src/sectors_d.cpp
	src/sectors_r.cpp
	src/sounds.cpp
	src/spawn.cpp
	src/spawn_d.cpp
	src/spawn_r.cpp
	src/zz_global.cpp
	src/zz_savegame.cpp
	)

add_game_library2( duke )

include_directories( src )
