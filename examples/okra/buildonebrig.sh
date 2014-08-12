export LD_LIBRARY_PATH=/home/amd/Prakash/Okra-Obsidian/dist/bin
CLOC_DIR=/home/amd/CLOC
D2_DIR=/home/amd/Prakash/BenTest/bin/D2/
SAMPLES_DIR=/home/amd/Prakash/Okra-Obsidian/samples
g++ -g -O2 -I../dist/include -I$SAMPLES_DIR/src/cpp -L../dist/bin -o ./dist/$1 $SAMPLES_DIR/src/cpp/$1/$1.c -lokra_x86_64 -ldl
$CLOC_DIR/cloc -hsail -p $D2_DIR $SAMPLES_DIR/src/cpp/$1/$1.cl -o $SAMPLES_DIR/src/cpp/$1/$1.brig
cp $SAMPLES_DIR/src/cpp/$1/$1.brig dist
