TODOs
=====

- Find a way to make CPU frequency detection either succeed always or be optional.
- Apply fork-join optimization to the native fork join scheduler.
- Update the Chase-Lev Deque so that it's compatible with ARM64.

NOTES
=====

CMake command to build verbosely
--------------------------------

~~~~
$ cmake . -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
$ cmake --build .
~~~~
