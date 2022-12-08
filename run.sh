clang-7 -pthread -lm -o brie build/ast.c build/bytecode.c build/compiler.c build/error.c build/hash.c build/parser.c build/scanner.c build/value.c build/vm.c main.c
./brie source.br