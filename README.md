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

		```
		# apt-get install spim
		```

A Qt-version of `spim` can be found on [SPIM MIPS simulator](spimsimulator.sourceforge.net).

The lexical and syntax table is not available here due to copyright.
However, they can be inferred from `lexical.l` and `syntax.y`.
Contact me if you want one of these copies.

## Usage

``` shell
$ cd lab*	# substitute * with 1,2,3 or 4
$ make
$ ./parser inputcode.cmm
```
