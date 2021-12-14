# Makefile
.PHONY: all pdflatex clean

all: pdflatex
TEX	:= pdflatex
DOC	:= paper
OPTPDF := yags
OUTPUT := RA95108

pdflatex:
	@$(TEX) -jobname=${OUTPUT} $(DOC).tex 
	@echo '${OUTPUT}.pdf criado!'

clean:
	@rm -rf $(DOC).log $(OPTPDF).pdf $(DOC).aux ; \
	rm -rf $(OUTPUT).log $(OUTPUT).aux $(OUTPUT).out ; \
	echo 'Removido arquivos auxiliares do pdflatex!' ; \