# taskparts
Task-parallel runtime system: A C++ library to support task parallelism on multicore platforms

We need the compiler flag `-fno-stack-protector` for correct compilation of native fork join. 

## Configuration

### Compiler flags

#### `TASKPARTS_OVERRIDE_PAUSE_INSTR`

By default, all spin-wait loops issue the `PAUSE` instruction every
trip around the loop in order to let the processor know the caller is
spinning.

This instruction can affect performance in some cases. This complier
flag replaces all uses of `PAUSE` with `NOP`s.

## TODOs

- Enable separate compilation
- Make deque data structure a template parameter of the work-stealing scheduler.
- Document all environment-variable arguments
- Set up continuous integration of unit tests
