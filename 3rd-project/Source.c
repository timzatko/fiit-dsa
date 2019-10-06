// zadanie3.c -- Timotej Zaťko, 13.11.2017 14:37
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#define min(a, b) (a > b ? b : a)
#define DEBUG 1
#define DBG if (DEBUG > 0)
#define TABLE_SIZE 256
#define hash(n) (n & 0xFF)

typedef unsigned long long __flag__;
//typedef unsigned int __flag__;

typedef struct Vertex_state {
	__flag__ flag;
	short prev_x;
	short prev_y;
	unsigned int path_length;
	struct Vertex_state * next;
} vertex_state;

typedef struct Vertex {
	vertex_state ** table;
} vertex;

vertex_state * get_vertex_data_list(vertex_state * v, __flag__ flag) {
	if (!v) return NULL;
	if (v->flag < flag) return get_vertex_data_list(v->next, flag);
	if (v->flag == flag) return v;
	return NULL;
}

vertex_state * set_vertex_data_list(vertex_state * v, __flag__ flag, unsigned int path_length, short prev_x, short prev_y) {
	if (!v || v->flag > flag) {
		vertex_state * node = (vertex_state *)malloc(sizeof(vertex_state));
		node->flag = flag;
		node->path_length = path_length;
		node->prev_x = prev_x;
		node->prev_y = prev_y;
		node->next = v;
		return node;
	}
	if (v->flag < flag) v->next = set_vertex_data_list(v->next, flag, path_length, prev_x, prev_y);
	return v;
}

void set_vertex_data(vertex * v, __flag__ flag, unsigned int path_length, int prev_x, int prev_y) {
	int h = hash(flag);
	v->table[h] = set_vertex_data_list(v->table[h], flag, path_length, prev_x, prev_y);
}

vertex_state * get_vertex_data(vertex * v, __flag__ flag) {
	return get_vertex_data_list(v->table[hash(flag)], flag);
}

void print_map(char ** map, int n, int m) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) printf("%c", map[i][j]);
		printf("\n");
	}
}

void print_data_map(int n, int m, vertex_state ** data, __flag__ flag) {
	for (int y = 0; y < n; y++) {
		for (int x = 0; x < m; x++) {
			vertex_state * v = get_vertex_data(&data[y][x], flag);
			printf("%.3d|", v->path_length);
		}
		printf("\n");
	}
	printf("\n");
}

typedef struct Point {
	short x;
	short y;
} point;

typedef struct Vector {
	unsigned int size;
	unsigned int length;
	point * data;
} vector;

void vector_resize(vector * v, unsigned int size) {
	v->size = size;
	v->data = v->data ? (point *)realloc(v->data, size * sizeof(point)) : (point *)malloc(sizeof(point) * size);
}

void vector_free(vector * v) {
	if (!v->data) return;
	free(v->data);
	v->size = v->length = v->data = 0;
}

void vector_push_back(vector * v, short x, short y) {
	if (v->length == v->size) vector_resize(v, v->size * 2);
	v->data[v->length].x = x;
	v->data[v->length].y = y;
	v->length++;
}

void vector_init(vector * v, unsigned int size) {
	v->length = 0;
	v->data = NULL;
	vector_resize(v, min(size, 1));
}

/* HEAP */

typedef struct Heap_item {
	short x;
	short y;
	short prev_x;
	short prev_y;
	unsigned int path_length;
	__flag__ flag;
} heap_item;

typedef struct Heap {
	heap_item * heap;
	unsigned int size;
	unsigned int reserved;
} heap;

void swap(heap_item * a, heap_item * b) {
	heap_item tmp = *a;
	*a = *b;
	*b = tmp;
}

heap * heap_init(unsigned int size) {
	heap * h = (heap *)malloc(sizeof(heap));
	h->size = 0;
	h->reserved = size;
	h->heap = (heap_item *)malloc(sizeof(heap_item) * size);
	return h;
}

void heap_insert(heap * h, short x, short y, unsigned int path_length, short prev_x, short prev_y, int flag) {

	if (h->size == h->reserved) {
		h->reserved = h->reserved << 1;
		h->heap = (heap_item *)realloc(h->heap, h->reserved * sizeof(heap_item));
	}

	h->heap[h->size].x = x;
	h->heap[h->size].y = y;
	h->heap[h->size].prev_x = prev_x;
	h->heap[h->size].prev_y = prev_y;
	h->heap[h->size].path_length = path_length;
	h->heap[h->size].flag = flag;
	int index = h->size;
	h->size++;

	while (index) {
		int parent = (index - 1) >> 1;
		if (h->heap[index].path_length < h->heap[parent].path_length) {
			swap(&h->heap[index], &h->heap[parent]);
			index = parent;
		} else {
			break;
		}
	}

}

heap_item * heap_top(heap * h) {
	if (!h->size) return NULL;
	return &h->heap[0];
}

void heap_free(heap * h) {
	free(h->heap);
	h->reserved = 0;
	h->size = 0;
	h->heap = NULL;
}

heap_item * heap_pop(heap * h) {

	if (!h->size) return 0;
	h->size--;
	swap(&h->heap[h->size], &h->heap[0]);
	int index = 0;

	while (1) {
		int dest = index, left = (index << 1) + 1, right = (index << 1) + 2;
		if (left < h->size && h->heap[left].path_length < h->heap[dest].path_length) dest = left;
		if (right < h->size && h->heap[right].path_length < h->heap[dest].path_length) dest = right;
		if (dest != index) {
			swap(&h->heap[index], &h->heap[dest]);
			index = dest;
		} else {
			return &h->heap[h->size];
		}
	}

	return NULL;
}

/* DJIKSTRA */
/*	Flag je pamati reprezentovana ako short, jednotlive stavy reprezentuju
0	000	Default
1	001	Nasli sme generator
2	010	Nasli sme draka
3	011 Nasli sme prvu princeznu
4	100 Nasli sme druhu princeznu
5	101 Nasli sme tretiu princeznu
6	110, 111 Nasli sme stvrtu, piatu.. princeznu
*/

#define DEFAULT 0
#define	GENERATOR_ENABLED 1
#define DRAGON_KILLED_STATE 2
#define PRINCESS_FOUND 3
//#define PRINCESS_COUNT 4
#define STATE_BIT_LENGTH 3
int PRINCESS_COUNT = 0;

#define FLAG_SIZE 2 + PRINCESS_COUNT //Pocet stavov ktore mozu byt zakodovane
#define STATE_MASK ~(~0 << STATE_BIT_LENGTH)

#define get_kth_state(flag, k) ((flag & (STATE_MASK << k * STATE_BIT_LENGTH)) >> (k * STATE_BIT_LENGTH))
#define set_kth_state(flag, k, state) (flag = flag | (state << (k * STATE_BIT_LENGTH)))
#define unset_kth_state(flag, k) flag = ((flag & ~(STATE_MASK << ((k) * STATE_BIT_LENGTH))))

int flag_has_state(__flag__ flag, int state) {
	for (int k = 0; k < FLAG_SIZE; k++) if (get_kth_state(flag, k) == state) return 1;
	return 0;
}

int flag_set_state(unsigned int * flag, int state) {
	for (int k = 0; k < FLAG_SIZE; k++) if (get_kth_state(*flag, k) == DEFAULT) return set_kth_state(*flag, k, state);
	return 0;
}

#define is_princess_rescued(flag, princess) (flag_has_state(flag, PRINCESS_FOUND + princess))

int princesses_rescued(__flag__ flag) {
	for (int k = 0; k < PRINCESS_COUNT; k++) if (!is_princess_rescued(flag, k)) return 0;
	return 1;
}

#define is_dragon_killed(flag) (flag_has_state(flag, DRAGON_KILLED_STATE) > 0)
#define is_generator_enabled(flag) (flag_has_state(flag, GENERATOR_ENABLED) > 0)
#define is_rescue_finished(flag) (is_dragon_killed(flag) && princesses_rescued(flag))
#define set_dragon_killed(flag) (flag_set_state(&flag, DRAGON_KILLED_STATE))
#define set_generator_enabled(flag) (flag_set_state(&flag, GENERATOR_ENABLED))
#define set_princess_rescued(flag, princess) (flag_set_state(&flag, PRINCESS_FOUND + princess))

int get_princess_number(int x, int y, vector * v) {
	for (int i = 0; i < v->length; i++) if (v->data[i].x == x && v->data[i].y == y) return i;
	return -1;
}

#define is_valid_point(x, y, n, m) (x >= 0 && y >= 0 && x < m && y < n)
#define is_road(x, y, map) (map[y][x] == 'C')
#define is_blocked(x, y, map) (map[y][x] == 'N')
#define is_slow_road(x, y, map) (map[y][x] == 'H')
#define is_dragon(x, y, map) (map[y][x] == 'D')
#define is_princess(x, y, map) (map[y][x] == 'P')
#define is_generator(x, y, map) (map[y][x] == 'G')
#define is_teleport(x, y, map) (map[y][x] >= '0' && map[y][x] <= '9')

#define heap_item_dbg(s, h) DBG printf("%s: %dx%d, len:%d flag:%d\n", s, h->x, h->y, h->path_length, h->flag);

int * zachran_princezne(char ** mapa, int n, int m, int t, int * dlzka_cesty) {

	const int d[4][2] = { { -1, 0 },{ 0, -1 },{ 1, 0 },{ 0, 1 } }; //Directions
	char ** map = mapa; //print_map(map, n, m);
	vertex ** data = (vertex **)malloc(n * sizeof(vertex *));
	vector teleports[10], princesses; //Tu budu nase teleporty
	vector_init(&princesses, 3);
	for (int i = 0; i < 10; i++) vector_init(&teleports[i], 32);

	for (int i = 0; i < n; i++) { //Teraz najdeme vsetky teleporty a princezne ktore si ocislujeme
		data[i] = (vertex *)malloc(m * sizeof(vertex));
		for (int j = 0; j < m; j++) {
			if (is_teleport(j, i, map)) vector_push_back(&teleports[map[i][j] - '0'], j, i);
			if (is_princess(j, i, map)) vector_push_back(&princesses, j, i);
			data[i][j].table = (vertex_state **)calloc(TABLE_SIZE, sizeof(vertex_state *));
		}
	}

	PRINCESS_COUNT = princesses.length;

	vertex_state * solution = NULL;
	point last; //Suradnice posledneho bodu ktory sme navstivili
	heap * h = heap_init(n * m * 8); //Vytvorime si haldu
	heap_insert(h, 0, 0, 1, -1, -1, 0); // Pridame prvy vrchol do haldy
	heap_item * c_vertex = malloc(sizeof(heap_item)), *h_top; //Sem si budeme ukaldat popnuty prvok z haldy

	while (heap_top(h)) {

		while (h_top = heap_top(h)) { //Popujeme hrany ku ktorym uz vieme cestu
			if (!solution && !get_vertex_data(&data[h_top->y][h_top->x], h_top->flag)) break;
			heap_pop(h); //heap_item_dbg("Pop", h_top);
		}

		if (!heap_top(h)) break;
		memcpy(c_vertex, heap_pop(h), sizeof(heap_item)); //heap_item_dbg("Visit", c_vertex);

		set_vertex_data(&data[c_vertex->y][c_vertex->x], c_vertex->flag, c_vertex->path_length, c_vertex->prev_x, c_vertex->prev_y); //Zapiseme si ze sme to tu navstivili
		__flag__ flag = c_vertex->flag; //Novy flag

		if (is_dragon(c_vertex->x, c_vertex->y, map) && !is_dragon_killed(flag)) {
			set_dragon_killed(flag);
		} else if (is_princess(c_vertex->x, c_vertex->y, map) && is_dragon_killed(flag)) {
			int p_id = get_princess_number(c_vertex->x, c_vertex->y, &princesses);
			if (!is_princess_rescued(flag, p_id)) set_princess_rescued(flag, p_id);
		} else if (is_generator(c_vertex->x, c_vertex->y, map) && !is_generator_enabled(flag)) {
			set_generator_enabled(flag);
		}

		if (flag != c_vertex->flag && is_rescue_finished(flag)) { //Ak sme zachranili princezne
			solution = get_vertex_data(&data[c_vertex->y][c_vertex->x], c_vertex->flag); //Ulozime si ho
			last.x = c_vertex->x;
			last.y = c_vertex->y;
		} else if (flag != c_vertex->flag) { //Ak sme nezachranili princezne, tak do hlady dame aktualny vrchol s novy stavom
			heap_insert(h, c_vertex->x, c_vertex->y, c_vertex->path_length, -1, -1, flag);
		}

		for (int i = 0; i < 4; i++) {

			int x = c_vertex->x + d[i][0], y = c_vertex->y + d[i][1], path_length = c_vertex->path_length;
			if (!is_valid_point(x, y, n, m) || is_blocked(x, y, map)) continue; //Ak je point mimo mapu alebo je nepriechodny skipneme ho
			if (get_vertex_data(&data[y][x], c_vertex->flag)) continue; //Ak je vrchol uz navstiveny, tiez ho skipneme
			path_length += is_slow_road(x, y, map) ? 2 : 1; //Vypocitame si dlzku cesty
			heap_insert(h, x, y, path_length, c_vertex->x, c_vertex->y, (int)c_vertex->flag);

		}

		if (is_generator_enabled(c_vertex->flag) && is_teleport(c_vertex->x, c_vertex->y, map)) { //Ak su zapnute teleporty a sme na teleporte
			vector * tp = &teleports[map[c_vertex->y][c_vertex->x] - '0']; //Pozrieme sa na pole teleportov
			for (int i = 0; i < tp->length; i++) { //Prehladame ho
				int x = tp->data[i].x, y = tp->data[i].y;
				if (get_vertex_data(&data[y][x], c_vertex->flag)) continue; //Pozrieme sa ci sme uz v tych bodoch neboli, a ak sme neboli
				heap_insert(h, x, y, c_vertex->path_length, c_vertex->x, c_vertex->y, (int)c_vertex->flag); //Dame ich do haldy
			}
		}

	}
	if (!solution) return NULL;
	vector path;
	point p;
	vector_init(&path, 64);
	vector_push_back(&path, last.x, last.y);
	int flag = solution->flag, k = FLAG_SIZE + 1;

	while (1) {
		//DBG printf("flag:%d\n", flag);
		while (solution->prev_x != -1 && solution->prev_y != -1) {
			p.x = solution->prev_x;
			p.y = solution->prev_y;
			vector_push_back(&path, p.x, p.y); //DBG printf("%dx%d\n", p.x, p.y);
			solution = get_vertex_data(&data[p.y][p.x], flag); //Posuvame sa v aktualnom stave
		}
		if (!--k) break;
		unset_kth_state(flag, k - 1);
		solution = get_vertex_data(&data[p.y][p.x], flag); //Posunieme sa do dalsieho stavu
	}

	int * result = (int *)malloc(2 * sizeof(int) * path.length), index = 0;
	for (int i = path.length - 1; i >= 0; i--) {
		result[index++] = path.data[i].x;
		result[index++] = path.data[i].y; //DBG printf("%d %d\n", path.data[i].x, path.data[i].y);
	}

	*dlzka_cesty = path.length;
	return result;

}

/* TESTOVANIE */

void heap_test(unsigned int size) { //Test minimovej haldy

	heap * h = heap_init(size);
	int * result = (int *)malloc(sizeof(int) * size);

	for (int i = 0; i < size; i++) {
		heap_insert(h, 0, 0, rand(), 0, 0, 0);
	}

	for (int i = 0; i < size; i++) {
		result[i] = heap_pop(h)->path_length;
	}

	for (int i = 1; i < size; i++) {
		if (result[i - 1] > result[i]) {
			printf("Test bol neuspesny\n");
			return;
		}
	}

	printf("Test bol uspesny\n");

}

void cmp_path(int * result, int result_len, int * path, int path_len) {
	int i;
	for (i = 0; i < path_len * 2; i += 2) {
		if (i >= result_len * 2) {
			printf("Spravny vysledok ocakava este dalsie suradnice!\n");
			break;
		}
		printf("#%d, Vysledok: %d %d ", i / 2, result[i], result[i + 1]);
		if (result[i] != path[i] || result[i + 1] != path[i + 1]) printf("Nespravne!\n"); else printf("Spravne\n");
	}
	if (i < result_len * 2) printf("Spravny vysledok neocakava uz dalsie suradnice, ale vystup pokracuje!\n");
	printf("Test bol dokonceny\n");
}

void test_1() { //Test 1, 3 princezne za sebou a za nimi drak, najskor musime zabit draka a potom princezne

	char map[5][5] = {
		{ 'P', 'C', 'C', 'C', 'C' },
		{ 'P', 'C', 'C', 'C', 'C' },
		{ 'P', 'C', 'C', 'C', 'C' },
		{ 'D', 'C', 'C', 'C', 'C' },
		{ 'C', 'C', 'C', 'C', 'C' }
	};

	char * map_p[5] = { map[0], map[1], map[2], map[3], map[4] }, ** map_ptr = map_p;
	int path_len = 0, *result = zachran_princezne(map_ptr, 5, 5, 0, &path_len);
	int path[14] = { 0, 0, 0, 1, 0, 2, 0, 3, 0, 2, 0, 1, 0, 0 };

	cmp_path(result, path_len, path, 7);

}

void test_2() { //Test na nepriechodne bloky, a hustiny

	char map[5][5] = {
		{ 'C', 'C', 'C', 'C', 'P' },
		{ 'C', 'C', 'P', 'N', 'C' },
		{ 'C', 'C', 'H', 'H', 'C' },
		{ 'N', 'N', 'N', 'N', 'C' },
		{ 'P', 'C', 'C', 'C', 'D' }
	};

	char * map_p[5] = { map[0], map[1], map[2], map[3], map[4] }, ** map_ptr = map_p;
	int path_len = 0, *result = zachran_princezne(map_ptr, 5, 5, 0, &path_len);
	int path[48] = { 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 3, 4, 2, 4, 1, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 4, 3, 4, 2, 4, 1, 4, 0, 3, 0, 2, 0, 2, 1 };

	cmp_path(result, path_len, path, 24);

}

void test_3() { //Test na nepriechodne bloky, a hustiny, a teleporty, cesta s teleportami je o 1, kratsia ako bez

	char map[5][5] = {
		{ 'C', 'D', 'H', 'H', 'H' },
		{ 'C', 'N', 'G', 'N', '0' },
		{ 'C', 'N', '0', 'N', '1' },
		{ 'C', 'N', 'N', 'N', '1' },
		{ 'C', 'C', 'P', 'P', 'P' }
	};

	char * map_p[5] = { map[0], map[1], map[2], map[3], map[4] }, ** map_ptr = map_p;
	int path_len = 0, *result = zachran_princezne(map_ptr, 5, 5, 0, &path_len);
	int path[22] = { 0, 0, 1, 0, 2, 0, 2, 1, 2, 2, 4, 1, 4, 2, 4, 3, 4, 4, 3, 4, 2, 4 };

	cmp_path(result, path_len, path, 11);

}

void test_4() { //Test na 4 princezne

	char map[5][5] = {
		{ 'C', 'C', 'C', 'C', 'P' },
		{ 'C', 'P', 'P', 'N', 'C' },
		{ 'C', 'C', 'H', 'H', 'C' },
		{ 'N', 'N', 'N', 'N', 'C' },
		{ 'P', 'C', 'C', 'C', 'D' }
	};

	char * map_p[5] = { map[0], map[1], map[2], map[3], map[4] }, ** map_ptr = map_p;
	int path_len = 0, *result = zachran_princezne(map_ptr, 5, 5, 0, &path_len);
	int path[50] = { 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 3, 4, 2, 4, 1, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 4, 3, 4, 2, 4, 1, 4, 0, 3, 0, 2, 0, 2, 1, 1, 1 };

	cmp_path(result, path_len, path, 25);

}

// Vlastna funkcia main() je pre vase osobne testovanie. Dolezite: pri testovacich scenaroch sa nebude spustat!
int main() {

	int seed = time(NULL);
	srand(seed);
	printf("Seed: %d\n", seed);

	//heap_test(1000);
	test_1();
	test_2();
	test_3();
	test_4();

	getchar();
	getchar();

	return 0;

}