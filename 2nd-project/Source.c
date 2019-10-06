// zadanie2.c -- Timotej Zaťko, 24.10.2017 14:48

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
	#include <windows.h>
#endif

#ifdef unix
	#include <sys/time.h>
#endif

#define min(a, b) (a > b ? b : a)
#define max(a, b) (a > b ? a : b)
#define ALPHA 0.8
#define RE_HASH_TABLE 1

int primes[] = { 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877, 205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 105359939, 210719881, 421439783, 842879579, 1685759167, 3371518343 };

typedef struct strcut_node {
	int priority;
	int weight;
	char * name;
	struct struct_node * left;
	struct struct_node * right;
} tree_node;

typedef struct struct_table_node {
	char * name;
	tree_node * root;
	struct struct_table_node * next;
} table_node;

typedef struct struct_table {
	table_node ** table;
	unsigned int size;
	unsigned int items;
	unsigned int prime_index;
	double target_alpha;
} table;

table * table_ptr;

int get_hash(char * str) {
	int n = 0;
	for (int i = 0; i < strlen(str); i++)
		n += 33 * i + str[i];
	return n % table_ptr->size;
}

int get_weight(tree_node * node) {
	return !node ? 0 : node->weight;
}

void update_weight(tree_node ** node) {
	if (!*node) return;
	(*node)->weight = 1 + get_weight((*node)->left) + get_weight((*node)->right);
}

void treap_split(tree_node * node, char * name, tree_node ** left, tree_node ** right) {

	if (!node) {
		*left = *right = NULL;
		return;
	}

	if (strcmp(name, node->name) < 0) {
		treap_split(node->left, name, &(*left), &node->left);
		*right = node;
	} else {
		treap_split(node->right, name, &node->right, &(*right));
		*left = node;
	}

	update_weight(&node);

}

void treap_merge(tree_node ** node, tree_node * left, tree_node * right) {

	if (!left || !right) {
		*node = left ? left : right;
		return;
	}

	if (left->priority > right->priority) {
		treap_merge(&left->right, left->right, right);
		*node = left;
	} else {
		treap_merge(&right->left, left, right->left);
		*node = right;
	}

	update_weight(&*node);

}

void treap_insert(tree_node ** root, tree_node * node) {

	if (!*root) {
		*root = node;
		return;
	}

	if (node->priority > (*root)->priority) {
		treap_split(*root, node->name, &node->left, &node->right);
		*root = node;
		update_weight(&*root);
		return;
	}

	treap_insert(strcmp(node->name, (*root)->name) < 0 ? &(*root)->left : &(*root)->right, node);
	update_weight(&*root);

}

int treap_remove(tree_node ** node, char * name) {

	if (!*node) return 0;

	if (strcmp((*node)->name, name) == 0) {
		tree_node * temp = *node;
		treap_merge(&(*node), (*node)->left, (*node)->right);
		free(temp);
		update_weight(&*node);
		return 1;
	}

	treap_remove(strcmp(name, (*node)->name) < 0 ? &((*node)->left) : &((*node)->right), name);
	update_weight(&*node);

}

tree_node * treap_create_node(char * name) {

	tree_node * new_user = (tree_node*)malloc(sizeof(tree_node));
	new_user->priority = rand();
	new_user->weight = 1;
	new_user->left = NULL;
	new_user->right = NULL;
	new_user->name = (char *)malloc(strlen(name) + 1);
	strcpy(new_user->name, name);

	return new_user;

}

tree_node * get_kth_node(tree_node * node, int k) {
	if (!node || k > get_weight(node)) return NULL;
	int left_weight = get_weight(node->left);
	if (left_weight == k) return node;
	return k < left_weight ? get_kth_node(node->left, k) : get_kth_node(node->right, k - left_weight - 1);
}

table_node * table_create_node(char * name) {
	table_node * node = (table_node*)malloc(sizeof(table_node));
	node->next = NULL;
	node->root = NULL;
	node->name = (char *)malloc(strlen(name) + 1);
	strcpy(node->name, name);
	return node;
}

void table_list_re_hash(table_node ** root, table_node * node) {
	if (!*root) {
		*root = node;
		return;
	}
	int cmp = strcmp(node->name, (*root)->name);
	if (cmp > 0) {
		return table_list_re_hash(&(*root)->next, node);
	} else if (cmp < 0) {
		node->next = *root;
		*root = node;
	}
	return;
}

void table_walk_list(table_node * node) {
	if (!node) return;
	table_walk_list(node->next);
	node->next = NULL;
	table_list_re_hash(&table_ptr->table[get_hash(node->name)], node);
}

table_node * table_re_hash(table_node * last_ptr) {

	table_ptr->items++;

	double alpha = table_ptr->items / table_ptr->size;

	if (RE_HASH_TABLE && alpha > table_ptr->target_alpha) {

		int old_table_size = table_ptr->size;
		table_ptr->prime_index++;
		table_ptr->size = primes[table_ptr->prime_index];

		//printf("Prehashoval som, velkost novej tabulky je %d\n", table_ptr->size);

		table_node ** old_table = table_ptr->table;
		table_ptr->table = (table_node **)calloc(table_ptr->size, table_ptr->size * sizeof(table_node *));

		for (int i = 0; i < old_table_size; i++) {
			table_walk_list(old_table[i]);
		}

		free(old_table);

	}

	return last_ptr;

}

table_node * table_insert(table_node ** root, char * name) {
	if (!*root) {
		*root = table_create_node(name);
		return table_re_hash(*root);
	}
	int cmp = strcmp(name, (*root)->name);
	if (cmp > 0) return table_insert(&(*root)->next, name);
	if (cmp == 0) return *root;
	table_node * node = table_create_node(name);
	node->next = *root;
	*root = node;
	return table_re_hash(*root);
}

tree_node ** table_get(char * page) {
	table_node * node = table_insert(&table_ptr->table[get_hash(page)], page);
	return &node->root;
}

void free_tree(tree_node * root) {
	if (!root) return;
	free_tree(root->left);
	free_tree(root->right);
	free(root);
}

void free_list(table_node * node) {
	if (!node) return;
	free_list(node->next);
	free_tree(node->root);
	free(node);
}

void free_pls() {

	if (!table_ptr) return;

	table_node ** table_ = table_ptr->table;

	for (int i = 0; i < table_ptr->size; i++) {
		free_list(table_[i]);
	}

	free(table_);
	free(table_ptr);

}

void init() {
	//srand(time(NULL));
	free_pls();
	table_ptr = (table*)malloc(sizeof(table));
	table_ptr->items = 0;
	table_ptr->prime_index = 0;
	table_ptr->size = primes[table_ptr->prime_index];
	table_ptr->target_alpha = ALPHA;
	table_ptr->table = (table_node **)calloc(table_ptr->size, table_ptr->size * sizeof(table_node *));
}

void like(char * page, char * user) {
	treap_insert(table_get(page), treap_create_node(user));
}

void unlike(char * page, char *user) {
	treap_remove(table_get(page), user);
}

char * getuser(char * page, int k) {
	tree_node * user = get_kth_node(*table_get(page), k - 1);
	return user ? user->name : NULL;
}

char * get_random_name() {
	char * name = (char *)malloc(32);
	for (int i = 0; i < 31; i++) {
		name[i] = (rand() % 2 ? 'a' : 'A') + rand() % ('z' - 'a');
	}
	name[31] = '\0';
	return name;
}

void inorder(tree_node * root, char *** arr, int * index) {
	if (!root) return;
	inorder(root->left, arr, index);
	(*arr)[*index] = &root->name;
	(*index)++;
	inorder(root->right, arr, index);
}

void random_tree_balance_test(int size) {

	init();

	printf("Nahodny test vyvazovania stromu, ci su useri abecedne za sebou - pocet userov %d\n", size);

	for (int i = 0; i < size; i++) {
		like("page", get_random_name());
	}

	printf("Vsetci useri oznacili ze sa im paci stranka, testujem vyvazovanie...\n");

	tree_node ** root = table_get("page");
	int index = 0;
	char ** arr = (char **)malloc(sizeof(char **) * size);

	inorder(*root, &arr, &index);
	int err = 0;

	for (int i = 1; i < size; i++) {
		if (strcmp(arr[i - 1], arr[i]) > 0) {
			printf("Chyba v strome %d-ty prvok je > ako %d-ty prvok %s, %s\n", i - 1, i, arr[i - 1], arr[i]);
			err = 1;
		}
	}

	if (err == 0) printf("Pocas testu nenastala ziadna chyba");

}

void get_tree_depth(tree_node * root, int * shortest_branch, int * longest_branch, int index) {
	if (!root) return;
	if (!root->left && !root->right) {
		*shortest_branch = min(*shortest_branch, index);
		*longest_branch = max(*longest_branch, index);
	};
	get_tree_depth(root->left, shortest_branch, longest_branch, index + 1);
	get_tree_depth(root->right, shortest_branch, longest_branch, index + 1);
}

int tree_depth_test(int size) {

	init();

	for (int i = 0; i < size; i++) {
		like("page", get_random_name());
	}

	printf("Test vysky stromu, pocet userov: %d\n", size);

	int shortest_branch = size, longest_branch = 0;

	tree_node ** root = table_get("page");
	get_tree_depth(*root, &shortest_branch, &longest_branch, 0);

	int logn2 = -1;
	#ifdef log
		logn2 = log(size) / log(2);
	#endif

	printf("Najdlhsia vetva: %d\nNajkratsia vetva: %d\nIdealna hlbka: %d\n", longest_branch, shortest_branch, logn2);

}

void get_user_err(int i) {
	printf("GetUser vratil nespravnu hodnotu - %d\n", i);
}

void kth_user_test() {

	init();

	char * user;

	like("Afoj", "Adam");
	like("Afoj", "Kristian");
	like("Afoj", "Denis");

	user = getuser("Afoj", 4);
	if (user != NULL) return get_user_err(1);

	unlike("Afoj", "Denis");

	user = getuser("Afoj", 3);
	if (user != NULL) return get_user_err(2);

	like("Afoj", "Tibor");

	user = getuser("Afoj", 2);
	if (!user || strcmp(user, "Kristian") != 0) return get_user_err(3);

	like("Afoj", "Lukas");
	like("Afoj", "Brano");

	user = getuser("Afoj", 2);
	if (!user || strcmp(user, "Brano") != 0) return get_user_err(4);

	like("afoj", "Brano");

	if (!user || strcmp(user, "Brano") != 0) return get_user_err(5);

	printf("Test prebehol uspesne\n");

}

int random_names_like_unlike() {

	printf("Random names, 50 like, 25 unlike, 50 like, 75 unlike\n");
	init();

	char ** names = (char **)malloc(100 * sizeof(char *));

	for (int i = 0; i < 100; i++) {
		names[i] = get_random_name();
	}

	for (int i = 0; i < 50; i++) {
		like("page", names[i]);
	}

	for (int i = 0; i < 25; i++) {
		unlike("page", names[i]);
	}

	for (int i = 50; i < 100; i++) {
		like("page", names[i]);
	}

	for (int i = 25; i < 100; i++) {
		unlike("page", names[i]);
	}

	char * user = getuser("page", 1);

	if (!user) {
		printf("Test prebehol uspesne\n");
	} else {
		printf("Test neprebehol uspesne\n");
	}

}

void table_alpha_speed_test(char ** names, int size, double alpha) {

	double interval = 0;

	for (int j = 0; j < 10; j++) {

		init();
		table_ptr->target_alpha = alpha;

		if (!RE_HASH_TABLE) {
			table_ptr->size = (double)(1. / alpha) * size;
			table_ptr->table = (table_node **)calloc(table_ptr->size, table_ptr->size * sizeof(table_node *));
		}

		#ifdef _WIN32
		LARGE_INTEGER frequency;
		LARGE_INTEGER start;
		LARGE_INTEGER end;
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start);
		#endif;

		for (int i = 0; i < size; i++) {
			table_get(names[i]);
		}

		for (int k = 0; k < 10; k++) {
			for (int i = 0; i < size; i++) {
				table_get(names[i]);
			}
		}

		#ifdef _WIN32
		QueryPerformanceCounter(&end);
		interval += (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
		#endif;

	}

	printf("Alpha: %f, average time: %f\n", alpha, interval / 10);

}

void table_test(int size) {

	printf("Hash tabulka alpha test, pocet stranok %d\n", size);

	char ** names = (char **)malloc(sizeof(char *) * size);
	for (int i = 0; i < size; i++) {
		names[i] = get_random_name();
	}

	for (double a = 0.3; a < 1; a += 0.05) {
		table_alpha_speed_test(names, size, a);
	}

	for (int i = 0; i < size; i++) {
		free(names[i]);
	}

	free(names);

	printf("Test ukonceny\n");

}

int main() {

	int seed = time(NULL);
	//int seed = 1234567890;
	//int seed = 2531998;
	srand(seed);
	printf("Seed bol nastaveny na %d\n", seed);
	
	//random_tree_balance_test(10000);
	tree_depth_test(10000);
	//kth_user_test();
	//random_names_like_unlike();
	//table_test(1000);

	getchar();

	return 0;

}