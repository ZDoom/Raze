
set( PCH_SOURCES
		src/2d.cpp
		src/ai.cpp
		src/aidemon.cpp
		src/aidevil.cpp
		src/aidragon.cpp
		src/aifatwitch.cpp
		src/aifish.cpp
		src/aifred.cpp
		src/aigoblin.cpp
		src/aigonzo.cpp
		src/aigron.cpp
		src/aiguardian.cpp
		src/aiimp.cpp
		src/aijudy.cpp
		src/aikatie.cpp
		src/aikobold.cpp
		src/aikurt.cpp
		src/aiminotaur.cpp
		src/ainewguy.cpp
		src/airat.cpp
		src/aiskeleton.cpp
		src/aiskully.cpp
		src/aispider.cpp
		src/aiwillow.cpp
		src/animate.cpp
		src/items.cpp
		src/names.cpp
		src/potions.cpp
		src/sndnames.cpp
		src/spellbooks.cpp
		src/weapons.cpp
		src/wepdata.cpp
		src/whani.cpp
		src/whfx.cpp
		src/whmap.cpp
		src/whldsv.cpp
		src/whobj.cpp
		src/whplr.cpp
		src/whtag.cpp
		src/wh.cpp
		src/main.cpp
		src/music.cpp
		src/sound.cpp
		src/fonts.cpp
	)

add_game_library2( whaven )

include_directories( src )
