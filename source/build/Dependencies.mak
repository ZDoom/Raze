# Build Engine dependencies
#
$(ENGINE_OBJ)/a-c.$o: $(ENGINE_SRC)/a-c.cpp $(ENGINE_INC)/a.h
$(ENGINE_OBJ)/a.$o: $(ENGINE_SRC)/a.$(asm)
$(ENGINE_OBJ)/animvpx.$o: $(ENGINE_SRC)/animvpx.cpp $(ENGINE_INC)/animvpx.h $(ENGINE_INC)/glbuild.h
$(ENGINE_OBJ)/baselayer.$o: $(ENGINE_SRC)/baselayer.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/osd.h
$(ENGINE_OBJ)/build.$o: $(ENGINE_SRC)/build.cpp $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/editor.h
$(ENGINE_OBJ)/cache1d.$o: $(ENGINE_SRC)/cache1d.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/kplib.h
$(ENGINE_OBJ)/compat.$o: $(ENGINE_SRC)/compat.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/libdivide.h
$(ENGINE_OBJ)/config.$o: $(ENGINE_SRC)/config.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/editor.h
$(ENGINE_OBJ)/crc32.$o: $(ENGINE_SRC)/crc32.cpp $(ENGINE_INC)/crc32.h
$(ENGINE_OBJ)/defs.$o: $(ENGINE_SRC)/defs.cpp $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/scriptfile.h $(ENGINE_INC)/compat.h
$(ENGINE_OBJ)/engine.$o: $(ENGINE_SRC)/engine.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/a.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/baselayer.h $(ENGINE_SRC)/engine_priv.h $(ENGINE_SRC)/engine_oldmap.h $(ENGINE_INC)/polymost.h $(ENGINE_INC)/hightile.h $(ENGINE_INC)/mdsprite.h $(ENGINE_INC)/polymer.h
$(ENGINE_OBJ)/2d.$o: $(ENGINE_SRC)/2d.cpp $(ENGINE_INC)/build.h
$(ENGINE_OBJ)/tiles.$o: $(ENGINE_SRC)/tiles.cpp $(ENGINE_INC)/build.h
$(ENGINE_OBJ)/clip.$o: $(ENGINE_SRC)/clip.cpp $(ENGINE_INC)/build.h $(ENGINE_INC)/clip.h
$(ENGINE_OBJ)/screenshot.$o: $(ENGINE_SRC)/screenshot.cpp
$(ENGINE_OBJ)/hash.$o: $(ENGINE_SRC)/hash.cpp $(ENGINE_INC)/hash.h
$(ENGINE_OBJ)/colmatch.$o: $(ENGINE_SRC)/colmatch.cpp
$(ENGINE_OBJ)/mhk.$o: $(ENGINE_SRC)/mhk.cpp
$(ENGINE_OBJ)/palette.$o: $(ENGINE_SRC)/palette.cpp $(ENGINE_INC)/palette.h
$(ENGINE_OBJ)/polymost.$o: $(ENGINE_SRC)/polymost.cpp $(ENGINE_INC)/lz4.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_SRC)/engine_priv.h $(ENGINE_INC)/polymost.h $(ENGINE_INC)/hightile.h $(ENGINE_INC)/mdsprite.h $(ENGINE_INC)/texcache.h
$(ENGINE_OBJ)/texcache.$o: $(ENGINE_SRC)/texcache.cpp $(ENGINE_INC)/texcache.h $(ENGINE_INC)/polymost.h $(ENGINE_INC)/dxtfilter.h $(ENGINE_INC)/kplib.h
$(ENGINE_OBJ)/dxtfilter.$o: $(ENGINE_SRC)/dxtfilter.cpp $(ENGINE_INC)/dxtfilter.h $(ENGINE_INC)/texcache.h
$(ENGINE_OBJ)/hightile.$o: $(ENGINE_SRC)/hightile.cpp $(ENGINE_INC)/kplib.h $(ENGINE_INC)/hightile.h
$(ENGINE_OBJ)/voxmodel.$o: $(ENGINE_SRC)/voxmodel.cpp $(ENGINE_SRC)/engine_priv.h $(ENGINE_INC)/polymost.h $(ENGINE_INC)/hightile.h $(ENGINE_INC)/mdsprite.h $(ENGINE_INC)/texcache.h
$(ENGINE_OBJ)/mdsprite.$o: $(ENGINE_SRC)/mdsprite.cpp $(ENGINE_SRC)/engine_priv.h $(ENGINE_INC)/polymost.h $(ENGINE_INC)/hightile.h $(ENGINE_INC)/mdsprite.h $(ENGINE_INC)/texcache.h
$(ENGINE_OBJ)/textfont.$o: $(ENGINE_SRC)/textfont.cpp
$(ENGINE_OBJ)/smalltextfont.$o: $(ENGINE_SRC)/smalltextfont.cpp
$(ENGINE_OBJ)/glbuild.$o: $(ENGINE_SRC)/glbuild.cpp $(ENGINE_INC)/glbuild.h $(ENGINE_INC)/baselayer.h
$(ENGINE_OBJ)/kplib.$o: $(ENGINE_SRC)/kplib.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/kplib.h
$(ENGINE_OBJ)/lz4.$o: $(ENGINE_SRC)/lz4.c $(ENGINE_INC)/lz4.h
$(ENGINE_OBJ)/md4.$o: $(ENGINE_SRC)/md4.cpp $(ENGINE_INC)/md4.h
$(ENGINE_OBJ)/osd.$o: $(ENGINE_SRC)/osd.cpp $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/baselayer.h
$(ENGINE_OBJ)/pragmas.$o: $(ENGINE_SRC)/pragmas.cpp $(ENGINE_INC)/compat.h
$(ENGINE_OBJ)/scriptfile.$o: $(ENGINE_SRC)/scriptfile.cpp $(ENGINE_INC)/scriptfile.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/compat.h
$(ENGINE_OBJ)/sdlayer.$o: $(ENGINE_SRC)/sdlayer.cpp $(ENGINE_SRC)/sdlayer12.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/sdlayer.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/a.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/glbuild.h
$(ENGINE_OBJ)/winlayer.$o: $(ENGINE_SRC)/winlayer.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/winlayer.h $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/a.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/dxdidf.h $(ENGINE_INC)/glbuild.h $(ENGINE_INC)/rawinput.h $(ENGINE_INC)/winbits.h
$(ENGINE_OBJ)/gtkbits.$o: $(ENGINE_SRC)/gtkbits.cpp $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/dynamicgtk.h
$(ENGINE_OBJ)/dynamicgtk.$o: $(ENGINE_SRC)/dynamicgtk.cpp $(ENGINE_INC)/dynamicgtk.h
$(ENGINE_OBJ)/polymer.$o: $(ENGINE_SRC)/polymer.cpp $(ENGINE_INC)/polymer.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/glbuild.h $(ENGINE_INC)/osd.h $(ENGINE_INC)/pragmas.h $(ENGINE_INC)/mdsprite.h $(ENGINE_INC)/polymost.h
$(ENGINE_OBJ)/mutex.$o: $(ENGINE_SRC)/mutex.cpp $(ENGINE_INC)/mutex.h
$(ENGINE_OBJ)/rawinput.$o: $(ENGINE_SRC)/rawinput.cpp $(ENGINE_INC)/rawinput.h
$(ENGINE_OBJ)/wiibits.$o: $(ENGINE_SRC)/wiibits.cpp $(ENGINE_INC)/wiibits.h
$(ENGINE_OBJ)/winbits.$o: $(ENGINE_SRC)/winbits.cpp $(ENGINE_INC)/winbits.h
$(ENGINE_OBJ)/xxhash.$o: $(ENGINE_SRC)/xxhash.c $(ENGINE_INC)/xxhash.h
$(ENGINE_OBJ)/pngwrite.$o: $(ENGINE_SRC)/pngwrite.cpp $(ENGINE_INC)/pngwrite.h
$(ENGINE_OBJ)/miniz.$o: $(ENGINE_SRC)/miniz.c $(ENGINE_INC)/miniz.h

$(ENGINE_OBJ)/lunatic.$o: $(ENGINE_SRC)/lunatic.cpp $(ENGINE_INC)/lunatic.h $(ENGINE_INC)/cache1d.h $(ENGINE_INC)/osd.h

$(ENGINE_OBJ)/startwin.editor.$o: $(ENGINE_SRC)/startwin.editor.cpp $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/editor.h $(ENGINE_INC)/winlayer.h $(ENGINE_INC)/compat.h $(ENGINE_INC)/startwin.editor.h
$(ENGINE_OBJ)/startgtk.editor.$o: $(ENGINE_SRC)/startgtk.editor.cpp $(ENGINE_INC)/baselayer.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/editor.h $(ENGINE_INC)/compat.h

$(TOOLS_OBJ)/compat_tools.$o: $(TOOLS_SRC)/compat_tools.cpp
$(TOOLS_OBJ)/kextract.$o: $(TOOLS_SRC)/kextract.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/kgroup.$o: $(TOOLS_SRC)/kgroup.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/transpal.$o: $(TOOLS_SRC)/transpal.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/pragmas.h
$(TOOLS_OBJ)/wad2art.$o: $(TOOLS_SRC)/wad2art.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/pragmas.h
$(TOOLS_OBJ)/wad2map.$o: $(TOOLS_SRC)/wad2map.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/pragmas.h
$(TOOLS_OBJ)/kmd2tool.$o: $(TOOLS_SRC)/kmd2tool.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/md2tool.$o: $(TOOLS_SRC)/md2tool.cpp $(ENGINE_INC)/compat.h $(ENGINE_INC)/build.h $(ENGINE_INC)/buildtypes.h $(ENGINE_INC)/glbuild.h $(ENGINE_INC)/mdsprite.h
$(TOOLS_OBJ)/generateicon.$o: $(TOOLS_SRC)/generateicon.cpp $(ENGINE_INC)/kplib.h $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/cacheinfo.$o: $(TOOLS_SRC)/cacheinfo.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/enumdisplay.$o: $(TOOLS_SRC)/enumdisplay.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/getdxdidf.$o: $(TOOLS_SRC)/getdxdidf.cpp $(ENGINE_INC)/compat.h
$(TOOLS_OBJ)/makesdlkeytrans.$o: $(TOOLS_SRC)/makesdlkeytrans.cpp
$(TOOLS_OBJ)/arttool.$o: $(TOOLS_SRC)/arttool.cpp
$(TOOLS_OBJ)/givedepth.$o: $(TOOLS_SRC)/givedepth.cpp
$(TOOLS_OBJ)/mkpalette.$o: $(TOOLS_SRC)/mkpalette.cpp
$(TOOLS_OBJ)/unpackssi.$o: $(TOOLS_SRC)/unpackssi.cpp
$(TOOLS_OBJ)/bsuite.$o: $(TOOLS_SRC)/bsuite.cpp
$(TOOLS_OBJ)/ivfrate.$o: $(TOOLS_SRC)/ivfrate.cpp $(ENGINE_INC)/animvpx.h
$(TOOLS_OBJ)/map2stl.$o: $(TOOLS_SRC)/map2stl.cpp
