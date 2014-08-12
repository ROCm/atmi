export LD_LIBRARY_PATH=/home/amd/Prakash/Okra-Obsidian/dist/bin
CLOC_DIR=/home/amd/CLOC
D2_DIR=/home/amd/Prakash/BenTest/bin/D2/
SAMPLES_DIR=/home/amd/Prakash/Git/CLOC/example/samples
OKRA_DIR=/home/amd/Prakash/Okra-Obsidian
gcc -O3 -I$OKRA_DIR/dist/include -L$OKRA_DIR/dist/bin -o ./dist/$1 src/cpp/$1/$1.c -ldl
cp src/cpp/$1/$1.brig dist
