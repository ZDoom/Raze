FILES=lunatic.html lunacon.html

# NOTE: the 'source-highlight-args' attribute is not present in the default
# AsciiDoc distribution. It has to be hacked into its
# filters/source/source-highlight-filter.conf like this:
#
# filter="source-highlight -f xhtml -s {language} (...)"
#  -->
# filter="source-highlight {source-highlight-args} -f xhtml -s {language} (...)"
%.html: %.txt Makefile lunatic_sh.style
	asciidoc -v -a latexmath -a source-highlight-args="--style-file=$(shell pwd)/lunatic_sh.style" $<

all: $(FILES)

clean:
	rm -f $(FILES)
