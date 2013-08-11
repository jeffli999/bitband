SRC=main.c common.c bitband.c rule.c trie.c poset.c

all: $(SRC)
	gcc -g $(SRC) -o main
