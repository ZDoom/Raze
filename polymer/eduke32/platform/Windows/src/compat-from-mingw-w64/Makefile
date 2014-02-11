
o=o

NAME:=libcompat-from-mingw-w64


%.$o: %.c
	gcc -Wall -Wextra -O3 -c $< -o $@

$(NAME).a: vsnprintf.$o
	ar rc $@ $^
	ranlib $@

all : $(NAME).a
	@ls -l $^

clean:
	-rm -f *.a *.o
