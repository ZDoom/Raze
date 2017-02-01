# ensure dinput.h can be found before running

%.o: %.c
	gcc -I../../include -Wall -Wextra -c $< -o $@

lib%.a: %.def %.o
	dlltool -k --output-lib $@ --def $*.def
	ar r $@ $*.o
	ranlib $@

lib%.a: %.def
	dlltool -k --output-lib $@ --def $<



all : libdsound.a libdxguid.a libdxguid_sdl.a
	@ls -l $^

full : libd3d8.a libd3dx8d.a libd3dxof.a libddraw.a libdinput.a libdinput8.a libdplayx.a \
libdpnaddr.a libdpnet.a libdpnlobby.a libdpvoice.a libdsetup.a \
libdsound.a libdxguid.a libdxguid_sdl.a
	@ls -l $^
	@echo move the created libraries to the lib dir.
	@echo happy coding !!



libdxguid_sdl.a : dxguid_sdl.o
	ar -r libdxguid_sdl.a dxguid_sdl.o
	ranlib libdxguid_sdl.a

libdxguid.a : dxguid.o
	ar -r libdxguid.a dxguid.o
	ranlib libdxguid.a

libdinput_custom.a : dinput.o
	ar r libdinput_custom.a dinput.o
	ranlib libdinput_custom.a

libdinput8.a : dinput8.def dinput.o
	dlltool -k --output-lib libdinput8.a --def dinput8.def
	ar r libdinput8.a dinput.o
	ranlib libdinput8.a



clean:
	-rm -f *.a *.o
