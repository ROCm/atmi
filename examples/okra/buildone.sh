export LD_LIBRARY_PATH=/home/amd/Prakash/Okra-Obsidian/dist/bin
CLOC_DIR=/home/amd/CLOC
D2_DIR=/home/amd/Prakash/BenTest/bin/D2/
SAMPLES_DIR=/home/amd/Prakash/Git/CLOC/example/samples
OKRA_DIR=/home/amd/Prakash/Okra-Obsidian
g++ -g -O2 -I$OKRA_DIR/dist/include -I$SAMPLES_DIR/src/cpp -L$OKRA_DIR/dist/bin -o ./dist/$1 src/cpp/$1/$1.cpp -lokra_x86_64
$CLOC_DIR/cloc -hsail -p $D2_DIR $SAMPLES_DIR/src/cpp/$1/$1.cl -o $SAMPLES_DIR/src/cpp/$1/$1.hsail
cp $SAMPLES_DIR/src/cpp/$1/$1.hsail dist
