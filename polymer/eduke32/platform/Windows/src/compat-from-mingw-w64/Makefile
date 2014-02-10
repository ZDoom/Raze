
o=o

NAME:=libcompat-from-mingw-w64
OBJS:=vsnprintf.$o


%.$o: %.c
	gcc -Wall -Wextra -O3 -c $< -o $@

$(NAME).a: $(OBJS)
	ar r $@ $<
	ranlib $@

all : $(NAME).a
	@ls -l $^

clean:
	-rm -f *.a *.o
