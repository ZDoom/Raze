
set( PCH_SOURCES
	src/actors.cpp
	src/actors_e.cpp
	src/anim.cpp
	src/cheats.cpp
	src/cmdline.cpp
	src/common.cpp
	src/config.cpp
	src/d_menu.cpp
	src/demo.cpp
	src/game.cpp
	src/gamedef.cpp
	src/gameexec.cpp
	src/gamevars.cpp
	src/global.cpp
	src/interpolate.cpp
	src/namesdyn.cpp
	src/net.cpp
	src/osdcmds.cpp
	src/player.cpp
	src/premap.cpp
	src/savegame.cpp
	src/sbar.cpp
	src/screens.cpp
	src/text.cpp
	src/sector.cpp
	src/sounds.cpp
	src/soundsdyn.cpp
	src/rrdh.cpp)

add_game_library2( duke )
