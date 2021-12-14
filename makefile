# Makefile
.PHONY: all pin report

all: pin report

pin:
	@cd code/ ; \
	for variation in GAg GAs GAp PAg PAs PAp SAg SAs SAp ; do \
		sed -i "24s/.*/const char *b = \"$$variation\";/" bp_custom.cpp ; \
		make BP=custom ; \
		mv obj-intel64/bp_custom.so obj-intel64/$$variation.so ; \
		mv obj-intel64/bp_custom.o obj-intel64/$$variation.o ; \
	done

report:
	@cd report ; \
	make ; \
	make ; \
	mv RA95108.pdf ../ ; \
	make clean ; \
