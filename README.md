# compiler-lab
Source code of lab on course Compiler Principles (in NJU)

## Directories
4 directories(lab1, lab2, lab3 and lab4) are listed.

Each directory correspond to one task.

- Lab1: Lexical and syntactic analysis. `flex` and `bison` are required to compile `.l` and `.y` files.
- Lab2: Semantic analysis.
- Lab3: Intermediate code generation. A intercode interpreter is provided at `lab3/irsim`.
- Lab4: Machine-code generation. The generated code is `spim` compatible.

## Notes

`spim` can be installed by (on Debian-based Linux distros):

```shell
# apt-get install spim
```

A Qt-version of `spim` can be found on [SPIM MIPS simulator](http://spimsimulator.sourceforge.net).

## About grammar

The grammar this project used is a simple version of C language.
The input files are assumed to be suffixed with `.cmm`.

The semantic rules are not available here.
Contact me if you want a copy of file descripting semantic rules.

## Usage

``` shell
$ cd lab*	# substitute * with 1,2,3 or 4
$ make
$ ./parser inputcode.cmm
```
