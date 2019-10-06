#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a, b) (a > b ? b : a)
#define max(a, b) (a > b ? a : b)

#define get_size(size)  ((size << 1) >> 1)
#define is_free(size) (!(size >> (sizeof(size) * 8 - 1)))

typedef struct free_block_head { //Blok pamate volneho bloku - node v strome
	unsigned int size; //Kluc v nasom treape, velkost daneho bloku
	unsigned int priority; //Priorita v treape
	struct free_block_head * l; //Smernik na lave dieta
	struct free_block_head * r; //Smernika na prave dieta
} FREE_BLOCK_HEAD;

typedef struct free_block_tail { //Paticka volneho pamatoveho bloku
	unsigned int size; //Velkost daneho bloku
} FREE_BLOCK_TAIL;

typedef struct head { //Blok pamate
	unsigned int size; //Velkost daneho bloku
} BLOCK_HEAD;

typedef struct tail { //Blok pamate
	unsigned int size; //Velkost daneho bloku
} BLOCK_TAIL;

typedef struct root { //Koren nasho stromu, smernik na prvy vrchol
	struct free_block_head * tree; //Strom volnych blokov
	unsigned int size; //Velkost celej pamatovej casti
} ROOT;

ROOT * memory;

void treap_split(FREE_BLOCK_HEAD * node, unsigned int size, FREE_BLOCK_HEAD ** l, FREE_BLOCK_HEAD ** r) {

	if (!node) {
		*l = *r = NULL;
		return;
	}

	if (size < get_size(node->size)) {
		treap_split(node->l, size, &(*l), &node->l);
		*r = node;
	} else {
		treap_split(node->r, size, &node->r, &(*r));
		*l = node;
	}

}

void treap_merge(FREE_BLOCK_HEAD ** node, FREE_BLOCK_HEAD * l, FREE_BLOCK_HEAD * r) {

	if (!l || !r) {
		*node = l ? l : r;
		return;
	}

	if (l->priority > r->priority) {
		treap_merge(&l->r, l->r, r);
		*node = l;
	} else {
		treap_merge(&r->l, l, r->l);
		*node = r;
	}

}

void treap_insert(FREE_BLOCK_HEAD ** root, FREE_BLOCK_HEAD * node) {

	if (!*root) {
		*root = node;
		return;
	}

	if (node->priority > (*root)->priority) {
		treap_split(*root, get_size(node->size), &node->l, &node->r);
		*root = node;
	} else {
		treap_insert(get_size(node->size) < get_size((*root)->size) ? &(*root)->l : &(*root)->r, node);
	}

}

int treap_remove(FREE_BLOCK_HEAD ** node, FREE_BLOCK_HEAD ** remove) {

	if (!*node) return 0;

	if (*node == *remove) {
		treap_merge(&(*node), (*node)->l, (*node)->r);
		return 1;
	} else {
		treap_remove((*remove)->size < (*node)->size ? &((*node)->l) : &((*node)->r), &(*remove));
	}

}

unsigned int treap_search_nearest_small(FREE_BLOCK_HEAD * node, unsigned int size) {

	if (node == NULL)
		return INT_MAX;

	if (size == get_size(node->size))
		return get_size(node->size);

	if (size < node->size) {
		return min(treap_search_nearest_small(node->l, size), get_size(node->size));
	}

	return treap_search_nearest_small(node->r, size);

}

FREE_BLOCK_HEAD * treap_find_block(FREE_BLOCK_HEAD * node, unsigned int size) {

	if (node == NULL)
		return NULL;

	if (size == get_size(node->size))
		return node;

	if (size < get_size(node->size)) {
		return treap_find_block(node->l, size);
	}

	return treap_find_block(node->r, size);

}

FREE_BLOCK_HEAD * treap_create_block(void * ptr, unsigned int size) { //The absolute size of the block including header + footer + data_size

	FREE_BLOCK_HEAD * block = (FREE_BLOCK_HEAD *)(char *)ptr;

	unsigned int block_size = size & ((~0 << 1) >> 1);

	block->size = block_size;
	block->priority = rand();
	block->l = NULL;
	block->r = NULL;

	FREE_BLOCK_TAIL * foot = (FREE_BLOCK_TAIL *)(void *)((char *)ptr + size - sizeof(FREE_BLOCK_TAIL));
	foot->size = block_size;

	return block;

}

void * memory_alloc(unsigned int size) {

	unsigned int block_size = max(sizeof(BLOCK_HEAD) + sizeof(BLOCK_TAIL) + size, sizeof(FREE_BLOCK_HEAD) + sizeof(FREE_BLOCK_TAIL)); //velkost alokovaneho bloku musi byt dostatocne velka na to aby ked ho uvolnime mozeme tam dat free FREE_BLOCK_HEAD strukturu

	FREE_BLOCK_HEAD * free_block = treap_find_block(memory->tree, treap_search_nearest_small(memory->tree, block_size));

	if (!free_block)
		return NULL;

	int free_block_size = get_size(free_block->size); //velkost voleho bloku

	if (free_block_size - block_size < sizeof(FREE_BLOCK_HEAD) + sizeof(FREE_BLOCK_TAIL)) { //ak nam ostane v bloku miesto tak male ze sa nam tam nezmesti struktura voleho bloku alokujeme ho
		block_size = free_block_size;
	}

	treap_remove(&memory->tree, &free_block); //Vymazeme stary blok zo stromu

	if (free_block_size != block_size) { //Velkost alokovaneho bloku != velkosti volneho bloku tj, vratime zvysne miesto do pamate
		FREE_BLOCK_HEAD * new_free_block = treap_create_block((void *)((char *)free_block + block_size), free_block_size - block_size);
		treap_insert(&memory->tree, new_free_block);
	}

	int full_block_size = block_size | ((~0 >> (sizeof(block_size) * 8 - 1)) << (sizeof(block_size) * 8 - 1)); //Nastavim prvy bit na 1 = block je plny
	BLOCK_HEAD * block_head = (BLOCK_HEAD *)(void *)((char *)free_block); //Nastavime hlavicku alokovaneho bloku
	BLOCK_TAIL * block_tail = (BLOCK_TAIL *)(void *)((char *)free_block + block_size - sizeof(BLOCK_TAIL)); //Nastavime hlavicku alokovaneho bloku
	block_head->size = full_block_size;
	block_tail->size = full_block_size;

	return (void *)((char *)free_block + sizeof(BLOCK_HEAD));

}

int memory_free(void * valid_ptr) {

	if ((char *)valid_ptr >= (char *)memory + memory->size || (char *)valid_ptr < (char *)memory + sizeof(ROOT))
		return 1;

	BLOCK_HEAD * block = (BLOCK_HEAD *)((char *)valid_ptr - sizeof(BLOCK_HEAD));

	if (is_free(block->size))
		return 1;

	unsigned int old_block_size = get_size(block->size);
	unsigned int block_size = old_block_size;

	if ((char *)block - (sizeof(FREE_BLOCK_TAIL) + sizeof(FREE_BLOCK_HEAD)) >= (char *)memory + sizeof(ROOT)) { //Pozrieme sa, ci mozeme mat vlavo este volny blok

		FREE_BLOCK_TAIL * prev_block_tail = (FREE_BLOCK_TAIL *)((char *)block - sizeof(FREE_BLOCK_TAIL)); //Paticka volneho bloku

		if (is_free(prev_block_tail->size)) {
			FREE_BLOCK_HEAD * prev_block_head = (FREE_BLOCK_HEAD *)((char *)block - prev_block_tail->size);
			if (treap_remove(&memory->tree, &prev_block_head)) {
				block = prev_block_head;
				block_size += prev_block_tail->size;
			}
		}

	}

	if ((char *)block + old_block_size < (char *)memory + memory->size - sizeof(FREE_BLOCK_HEAD)) { //Existuje nejaky blok za nasim blokom

		FREE_BLOCK_HEAD * next_block_head = (FREE_BLOCK_HEAD *)((char *)block + old_block_size);

		if (is_free(next_block_head->size)) {
			if (treap_remove(&memory->tree, &next_block_head)) {
				block_size += next_block_head->size;
			}
		}

	}

	treap_insert(&memory->tree, treap_create_block((char *)block, block_size));

	return 0;

}

int memory_check(void * ptr) {

	char * memory_start = (char *)memory + sizeof(ROOT);
	char * memory_end = (char *)memory + memory->size - sizeof(BLOCK_HEAD);

	if (ptr < memory_start + sizeof(BLOCK_HEAD) || ptr >= memory_end)
		return 0;

	char * curr = memory_start;

	while (curr < memory_end) {

		BLOCK_HEAD * block = (BLOCK_HEAD *)(char *)curr;

		if (ptr >= curr && ptr < curr + get_size(block->size)) {
			return ptr == (char *)curr + sizeof(BLOCK_HEAD) && !is_free(block->size);
		}

		curr = curr + block->size;

	}

}

void memory_init(void *ptr, unsigned int size) {

	memory = (ROOT *)(char *)ptr;
	memory->tree = NULL;
	memory->size = size;

	treap_insert(&memory->tree, treap_create_block((void *)((char *)memory + sizeof(ROOT)), size - sizeof(ROOT)));

}

int print(char * p, int size) {
	for (int i = 0; i < size; i++) {
		printf("%d ", p[i]);
	}
	printf("\n");
}

/*Alokovaný blok je väèší ako pamä – p1 musí by NULL */
void test_1() {

	char region[100];
	memory_init(region, 100);

	char * p1 = (char *)memory_alloc(145);

	printf("%p\n", p1);
	printf("%d\n", memory_check(p1));

}

/*Alokujeme bloky menšie ako ve¾kos vo¾ného bloku – alokova by sa mali bloky aspoò o ve¾kosti vo¾ného
bloku pamäte, mali by sa da vedie uvo¾ni a spoji (nedošlo k prepísaniu pätièiek) a znovu alokova,
v tomto prípade skúsime alokova ve¾kos celej vo¾nej pamäte*/
void test_2() {

	char region[100];
	memory_init(region, 100);

	char * p1 = (char *)memory_alloc(1);
	char * p2 = (char *)memory_alloc(1);
	char * p3 = (char *)memory_alloc(1);

	p1[0] = 1;
	p2[0] = 2;
	p3[0] = 3;

	print(p1, 1);
	print(p2, 1);
	print(p3, 1);

	printf("%d ", memory_free(p2));
	printf("%d ", memory_free(p1));
	printf("%d\n", memory_free(p3));

	char * p4 = (char *)memory_alloc(76);
	memset(p4, 8, 76);
	print(p4, 76);

}

/*Náhodne alokujeme pamä od ve¾kosti 1 do ve¾kosti 128, pokia¾ máme pamä.*/
#define MAX_RAND 128
#define MIN_RAND 1
#define MEMORY 10000

void test_3() {

	srand(time(NULL));

	char region[MEMORY], *p1;
	memory_init(region, MEMORY);
	int size, i = 0;

	while (1) {

		size = rand() % (MAX_RAND + 1 - MIN_RAND) + MIN_RAND;

		p1 = (char *)memory_alloc(size);

		if (!memory_check(p1))
			break;

		memset(p1, rand() % 10, size);

		printf("Alokacia c. %d, velkost: %d\n", ++i, size);
		print(p1, size);

	};

}

/*Testovanie funkcie memory_check*/
void test_4() {

	char region[100];
	memory_init(region, 100);

	char * p1 = (char *)memory_alloc(76);
	printf("%d\n", memory_check(p1));
	memory_free(p1);
	printf("%d\n", memory_check(p1));

	getchar();

}

/*Alokujeme blok a potom ho odalokujeme, pokial nedostaneme nejaky error, alebo pokial nás to baví*/
void test_5() {

	char region[100];
	memory_init(region, 100);

	char * p1;

	for (int i = 1; ; i++) {

		p1 = (char *)memory_alloc(76);

		if (!memory_check(p1)) {
			printf("Hopla!\n");
			break;
		}

		memory_free(p1);

		if (i == 1000000) {
			printf("Ked to spravi milion krat bez chyby, asi je to dobre.\n");
			break;
		}

	}

}

int main() {

	//test_1();
	//test_2();
	test_3();
	//test_4();
	//test_5();

	getchar();

	return 0;

}