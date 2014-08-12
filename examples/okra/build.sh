rm -rf dist
mkdir -p dist
./buildone.sh CalcPI
./buildone.sh Cooparray
./buildone.sh CSquares
./buildone.sh CSquaresDbl
./buildone.sh MatMul
./buildAndCopyBrig.sh VectorCopy

