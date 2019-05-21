sources = main.cxx
objects = main.o

objects := $(addprefix objects/, $(objects))
sources := $(addprefix source/, $(sources))

compile_arguments = -std=c++17 -DWINDOWS
link_arguments = -std=c++17	-DWINDOWS

cafind.exe: $(objects)
	mkdir -p builds
	clang++ $(objects) -o builds/cafind.exe $(link_arguments) 


objects/main.o: source/main.cxx
	mkdir -p objects
	clang++ source/main.cxx -c -o objects/main.o $(compile_arguments)