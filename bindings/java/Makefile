
all : 
	mkdir -p build;
	javac -d build src/gingko/*.java
	mkdir -p build/include;
	for i in `cd src/gingko && ls *.java`; do javah -d build/include -classpath build gingko.`echo $$i | sed 's/.java//g'`; done

clean : 
	rm -rf build;
