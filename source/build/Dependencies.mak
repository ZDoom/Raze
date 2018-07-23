# Build Engine dependencies
#
$(engine_obj)/a-c.$o: $(engine_src)/a-c.cpp $(engine_inc)/a.h
$(engine_obj)/a.$o: $(engine_src)/a.$(asm)
$(engine_obj)/animvpx.$o: $(engine_src)/animvpx.cpp $(engine_inc)/animvpx.h $(glad_inc)/glad/glad.h
$(engine_obj)/baselayer.$o: $(engine_src)/baselayer.cpp $(engine_inc)/compat.h $(engine_inc)/baselayer.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/osd.h
$(engine_obj)/build.$o: $(engine_src)/build.cpp $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/pragmas.h $(engine_inc)/compat.h $(engine_inc)/baselayer.h $(engine_inc)/editor.h
$(engine_obj)/cache1d.$o: $(engine_src)/cache1d.cpp $(engine_inc)/compat.h $(engine_inc)/cache1d.h $(engine_inc)/pragmas.h $(engine_inc)/baselayer.h $(engine_inc)/kplib.h
$(engine_obj)/compat.$o: $(engine_src)/compat.cpp $(engine_inc)/compat.h $(engine_inc)/libdivide.h
$(engine_obj)/config.$o: $(engine_src)/config.cpp $(engine_inc)/compat.h $(engine_inc)/osd.h $(engine_inc)/editor.h
$(engine_obj)/crc32.$o: $(engine_src)/crc32.cpp $(engine_inc)/crc32.h
$(engine_obj)/defs.$o: $(engine_src)/defs.cpp $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/baselayer.h $(engine_inc)/scriptfile.h $(engine_inc)/compat.h
$(engine_obj)/engine.$o: $(engine_src)/engine.cpp $(engine_inc)/compat.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/pragmas.h $(engine_inc)/cache1d.h $(engine_inc)/a.h $(engine_inc)/osd.h $(engine_inc)/baselayer.h $(engine_src)/engine_priv.h $(engine_src)/engine_oldmap.h $(engine_inc)/polymost.h $(engine_inc)/hightile.h $(engine_inc)/mdsprite.h $(engine_inc)/polymer.h
$(engine_obj)/2d.$o: $(engine_src)/2d.cpp $(engine_inc)/build.h
$(engine_obj)/tiles.$o: $(engine_src)/tiles.cpp $(engine_inc)/build.h
$(engine_obj)/clip.$o: $(engine_src)/clip.cpp $(engine_inc)/build.h $(engine_inc)/clip.h
$(engine_obj)/screenshot.$o: $(engine_src)/screenshot.cpp
$(engine_obj)/hash.$o: $(engine_src)/hash.cpp $(engine_inc)/hash.h
$(engine_obj)/colmatch.$o: $(engine_src)/colmatch.cpp
$(engine_obj)/mhk.$o: $(engine_src)/mhk.cpp
$(engine_obj)/palette.$o: $(engine_src)/palette.cpp $(engine_inc)/palette.h
$(engine_obj)/polymost.$o: $(engine_src)/polymost.cpp $(engine_inc)/lz4.h $(engine_inc)/compat.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_src)/engine_priv.h $(engine_inc)/polymost.h $(engine_inc)/hightile.h $(engine_inc)/mdsprite.h $(engine_inc)/texcache.h
$(engine_obj)/texcache.$o: $(engine_src)/texcache.cpp $(engine_inc)/texcache.h $(engine_inc)/polymost.h $(engine_inc)/dxtfilter.h $(engine_inc)/kplib.h
$(engine_obj)/tilepacker.$o: $(engine_src)/tilepacker.cpp  $(engine_inc)/compat.h $(engine_inc)/build.h $(engine_inc)/tilepacker.h
$(engine_obj)/dxtfilter.$o: $(engine_src)/dxtfilter.cpp $(engine_inc)/dxtfilter.h $(engine_inc)/texcache.h
$(engine_obj)/hightile.$o: $(engine_src)/hightile.cpp $(engine_inc)/kplib.h $(engine_inc)/hightile.h
$(engine_obj)/voxmodel.$o: $(engine_src)/voxmodel.cpp $(engine_src)/engine_priv.h $(engine_inc)/polymost.h $(engine_inc)/hightile.h $(engine_inc)/mdsprite.h $(engine_inc)/texcache.h
$(engine_obj)/mdsprite.$o: $(engine_src)/mdsprite.cpp $(engine_src)/engine_priv.h $(engine_inc)/polymost.h $(engine_inc)/hightile.h $(engine_inc)/mdsprite.h $(engine_inc)/texcache.h
$(engine_obj)/textfont.$o: $(engine_src)/textfont.cpp
$(engine_obj)/smalltextfont.$o: $(engine_src)/smalltextfont.cpp
$(engine_obj)/glbuild.$o: $(engine_src)/glbuild.cpp $(engine_inc)/glbuild.h $(engine_inc)/baselayer.h $(glad_inc)/glad/glad.h
$(engine_obj)/glsurface.$o: $(engine_src)/glsurface.cpp $(engine_inc)/compat.h $(engine_inc)/palette.h $(engine_inc)/glsurface.h $(glad_inc)/glad/glad.h $(engine_inc)/baselayer.h $(engine_inc)/build.h
$(engine_obj)/kplib.$o: $(engine_src)/kplib.cpp $(engine_inc)/compat.h $(engine_inc)/kplib.h
$(engine_obj)/md4.$o: $(engine_src)/md4.cpp $(engine_inc)/md4.h
$(engine_obj)/lz4.$o: $(engine_src)/lz4.c $(engine_inc)/lz4.h
$(engine_obj)/osd.$o: $(engine_src)/osd.cpp $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/osd.h $(engine_inc)/compat.h $(engine_inc)/baselayer.h
$(engine_obj)/pragmas.$o: $(engine_src)/pragmas.cpp $(engine_inc)/compat.h
$(engine_obj)/scriptfile.$o: $(engine_src)/scriptfile.cpp $(engine_inc)/scriptfile.h $(engine_inc)/cache1d.h $(engine_inc)/compat.h
$(engine_obj)/sdlayer.$o: $(engine_src)/sdlayer.cpp $(engine_src)/sdlayer12.cpp $(engine_inc)/compat.h $(engine_inc)/sdlayer.h $(engine_inc)/baselayer.h $(engine_inc)/cache1d.h $(engine_inc)/pragmas.h $(engine_inc)/a.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/osd.h $(glad_inc)/glad/glad.h  $(engine_inc)/glbuild.h
$(engine_obj)/winlayer.$o: $(engine_src)/winlayer.cpp $(engine_inc)/compat.h $(engine_inc)/winlayer.h $(engine_inc)/baselayer.h $(engine_inc)/pragmas.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/a.h $(engine_inc)/osd.h $(engine_inc)/dxdidf.h $(glad_inc)/glad/glad.h $(glad_inc)/glad/glad_wgl.h  $(engine_inc)/glbuild.h $(engine_inc)/rawinput.h $(engine_inc)/winbits.h
$(engine_obj)/gtkbits.$o: $(engine_src)/gtkbits.cpp $(engine_inc)/baselayer.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/dynamicgtk.h
$(engine_obj)/dynamicgtk.$o: $(engine_src)/dynamicgtk.cpp $(engine_inc)/dynamicgtk.h
$(engine_obj)/polymer.$o: $(engine_src)/polymer.cpp $(engine_inc)/polymer.h $(engine_inc)/compat.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(glad_inc)/glad/glad.h $(engine_inc)/glbuild.h $(engine_inc)/osd.h $(engine_inc)/pragmas.h $(engine_inc)/mdsprite.h $(engine_inc)/polymost.h
$(engine_obj)/mutex.$o: $(engine_src)/mutex.cpp $(engine_inc)/mutex.h
$(engine_obj)/rawinput.$o: $(engine_src)/rawinput.cpp $(engine_inc)/rawinput.h
$(engine_obj)/wiibits.$o: $(engine_src)/wiibits.cpp $(engine_inc)/wiibits.h
$(engine_obj)/winbits.$o: $(engine_src)/winbits.cpp $(engine_inc)/winbits.h
$(engine_obj)/xxhash.$o: $(engine_src)/xxhash.c $(engine_inc)/xxhash.h
$(engine_obj)/pngwrite.$o: $(engine_src)/pngwrite.cpp $(engine_inc)/pngwrite.h
$(engine_obj)/fix16.$o: $(engine_src)/fix16.c $(engine_inc)/fix16.h $(engine_inc)/fix16_int64.h
$(engine_obj)/miniz.$o: $(engine_src)/miniz.c $(engine_inc)/miniz.h $(engine_inc)/miniz_common.h $(engine_inc)/miniz_tinfl.h $(engine_inc)/miniz_tdef.h
$(engine_obj)/miniz_tinfl.$o: $(engine_src)/miniz_tinfl.c $(engine_inc)/miniz.h $(engine_inc)/miniz_common.h $(engine_inc)/miniz_tinfl.h $(engine_inc)/miniz_tdef.h
$(engine_obj)/miniz_tdef.$o: $(engine_src)/miniz_tdef.c $(engine_inc)/miniz.h $(engine_inc)/miniz_common.h $(engine_inc)/miniz_tinfl.h $(engine_inc)/miniz_tdef.h
$(engine_obj)/fix16_str.$o: $(engine_src)/fix16_str.c $(engine_inc)/fix16.h 

$(engine_obj)/lunatic.$o: $(engine_src)/lunatic.cpp $(engine_inc)/lunatic.h $(engine_inc)/cache1d.h $(engine_inc)/osd.h

$(engine_obj)/startwin.editor.$o: $(engine_src)/startwin.editor.cpp $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/editor.h $(engine_inc)/winlayer.h $(engine_inc)/compat.h $(engine_inc)/startwin.editor.h
$(engine_obj)/startgtk.editor.$o: $(engine_src)/startgtk.editor.cpp $(engine_inc)/baselayer.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(engine_inc)/editor.h $(engine_inc)/compat.h

$(tools_obj)/compat_tools.$o: $(tools_src)/compat_tools.cpp
$(tools_obj)/kextract.$o: $(tools_src)/kextract.cpp $(engine_inc)/compat.h
$(tools_obj)/kgroup.$o: $(tools_src)/kgroup.cpp $(engine_inc)/compat.h
$(tools_obj)/transpal.$o: $(tools_src)/transpal.cpp $(engine_inc)/compat.h $(engine_inc)/pragmas.h
$(tools_obj)/wad2art.$o: $(tools_src)/wad2art.cpp $(engine_inc)/compat.h $(engine_inc)/pragmas.h
$(tools_obj)/wad2map.$o: $(tools_src)/wad2map.cpp $(engine_inc)/compat.h $(engine_inc)/pragmas.h
$(tools_obj)/kmd2tool.$o: $(tools_src)/kmd2tool.cpp $(engine_inc)/compat.h
$(tools_obj)/md2tool.$o: $(tools_src)/md2tool.cpp $(engine_inc)/compat.h $(engine_inc)/build.h $(engine_inc)/buildtypes.h $(glad_inc)/glad/glad.h $(engine_inc)/mdsprite.h
$(tools_obj)/generateicon.$o: $(tools_src)/generateicon.cpp $(engine_inc)/kplib.h $(engine_inc)/compat.h
$(tools_obj)/cacheinfo.$o: $(tools_src)/cacheinfo.cpp $(engine_inc)/compat.h
$(tools_obj)/enumdisplay.$o: $(tools_src)/enumdisplay.cpp $(engine_inc)/compat.h
$(tools_obj)/getdxdidf.$o: $(tools_src)/getdxdidf.cpp $(engine_inc)/compat.h
$(tools_obj)/makesdlkeytrans.$o: $(tools_src)/makesdlkeytrans.cpp
$(tools_obj)/arttool.$o: $(tools_src)/arttool.cpp
$(tools_obj)/givedepth.$o: $(tools_src)/givedepth.cpp
$(tools_obj)/mkpalette.$o: $(tools_src)/mkpalette.cpp
$(tools_obj)/unpackssi.$o: $(tools_src)/unpackssi.cpp
$(tools_obj)/bsuite.$o: $(tools_src)/bsuite.cpp
$(tools_obj)/ivfrate.$o: $(tools_src)/ivfrate.cpp $(engine_inc)/animvpx.h
$(tools_obj)/map2stl.$o: $(tools_src)/map2stl.cpp
