
o=o

NAME:=libcompat-to-msvc
OBJS:=io_math.$o


%.$o: %.c
	gcc -Wall -Wextra -O3 -c $< -o $@

$(NAME).a: $(OBJS)
	ar r $@ $<
	ranlib $@

all : $(NAME).a
	@ls -l $^

clean:
	-rm -f *.a *.o
