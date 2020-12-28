# Tips

# Checking test coverage

On Linux, configure the build using clang, and use the `make coverage-todo` target to see missing test coverage.

```
./configure CC=clang CXX=clang++ CFLAGS=-Wall
```
