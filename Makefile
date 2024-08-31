build:
	gcc *.c -o cerver -I/usr/local/include -L/usr/local/lib -lcjson -Wl,-rpath,/usr/local/lib

clean:
	rm cerver
run:
	./cerver
