typedef unsigned char ELEM;        // キューの要素の型
void init_all_queue(void);
void init_queue(int p);
void enqueue(int p, ELEM x);
ELEM dequeue(int p);
int empty(int p);
int lenqueue(int p);
ELEM peek(int p,int loc);
