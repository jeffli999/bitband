SRC=main.c common.c bitband.c rule.c trie.c

all: $(SRC)
	gcc -g $(SRC) -o main
