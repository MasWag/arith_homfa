man: run_bg.1 run_vrss.1 

%.1: %.sh Makefile
	pandoc -s -f ./scrdoc/scrdoc.lua $< -t man -o $@  -V title:'$<' -V section:1 -V header:'ArithHomFA Examples' -V footer:'ArithHomFA'

clean:
	$(RM) run_bg.1 run_vrss.1

.PHONY: clean man
