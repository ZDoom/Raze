
o=o

NAME:=libcompat-to-msvc


%.$o: %.c
	gcc -Wall -Wextra -O3 -c $< -o $@

$(NAME).a: dll_math.$o io_math.$o
	ar rc $@ $^
	ranlib $@

all : $(NAME).a
	@ls -l $^

clean:
	-rm -f *.a *.o
