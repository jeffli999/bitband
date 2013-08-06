SRC=main.c common.c bitband.c rule.c trie.c stats.c

all: $(SRC)
	gcc -g $(SRC) -o main
