```
#For building OKRA examples
export OKRA_DISABLE_FIX_HSAIL=1
make all CFLAGS=-DDUMMY_ARGS=1
make test
```
