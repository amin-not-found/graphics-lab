# Graphics Lab
My graphical lab where I implement different graphical and simulation programs that I find interesting.

Programs in here should work both on Linux and Windows(mingw) although I haven't tested them on Linux.

## Dependencies
- gnu make to build
- gcc for compilation
- emscripten to build for web(wasm). note that `emcc` should be available in PATH and 3.1.53 is the version used


## Usage
Source code of programs exists in `src` directory and using `make` you can build the programs inside `bin` directory:
```shell 
make program_name
```
Assigning DEBUG variable while using `make` will make it use flags that are more suited for debugging and development; Otherwise appropriate flags for a release build will be used:
```shell 
make DEBUG=1 program_name
```
You can also conveniently build and run any program for development(using debug flags) with `run.sh`:
```shell 
./run.sh src/program_name.c
```
To generate programs for web using wasm:
```shell
make gen-web
```