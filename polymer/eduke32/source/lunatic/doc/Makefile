FILES=lunatic.html lunacon.html

%.html: %.txt
	asciidoc $<

all: $(FILES)

clean:
	rm -f $(FILES)
