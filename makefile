all:
	cc main.c rply.c glad_gl.c -I inc -Ofast -lglfw -lm -o plv

clean:
	rm -f plv
