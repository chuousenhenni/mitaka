#include <stdio.h>
#include <ctype.h>
#include "define.h"

//#define QUEUE_MAX 5     // キューの個数
#define QUEUE_SIZE 400     // キューの大きさ
#define INF -99    // キューが空で、デキューした時に返る無効な値
typedef unsigned char ELEM;        // キューの要素の型
ELEM queue[QUEUE_MAX][QUEUE_SIZE];  // キューの定義
int front[QUEUE_MAX];               // キューの先頭
int rear[QUEUE_MAX];                // キューの末尾 末尾の次の要素を指す

// 次の要素の添え字を求める
int next(int index) {
  return (index + 1) % QUEUE_SIZE;
}

// すべてのキューを初期化する
void init_all_queue(void) {
	int i;
	for( i = 0; i < QUEUE_MAX; i++) {
		front[i] = rear[i] = 0;
	}
}

// キューを初期化する
void init_queue(int p) {
  front[p] = rear[p] = 0;
}

// キューにデータを入れる
void enqueue(int p, ELEM x) {
  if (next(rear[p]) == front[p]) {
//     printf("cannot enqueue. Because queue is full.\n");
     return;
  }
  queue[p][rear[p]] = x;
  rear[p] = next(rear[p]);
}

// キューからデータを取り出す。データがないとINF(無効な値)を返す。
ELEM dequeue(int p) {
  ELEM x;
  if (front[p] == rear[p]) {
    printf("cannot dequeue. Because queue is empty.\n");
    return INF;
  }
  x = queue[p][front[p]];
	
//	if ( p == MONITOR_QUEUE) {
//		printf("%02X ",x);
//	}
	
  front[p] = next(front[p]);
  return  x;
}

// キューが空かどうかを調べる。空なら１、空でなければ０を返す
int empty(int p) {
  return  front[p] == rear[p];
}

// キューのあるデータのサイズを求める
int lenqueue(int p) {
	int len = rear[p] - front[p];
	if (len < 0) {
		len += QUEUE_SIZE;
	}
	return len;
}

// キューの指定位置のデータを読む
ELEM peek(int p,int loc) {
	ELEM x;
	int po = front[p] + loc;
	if (po >= QUEUE_SIZE) {
		po -= QUEUE_SIZE;
	}
	x = queue[p][po];
	return x;
}

void test(int p){
	int len;
	ELEM x;

  init_all_queue();
  enqueue(p, 12); enqueue(p, 2); enqueue(p, 19);
	len = lenqueue(p);
	printf("len=%d\n",len);
	dequeue(p);
	dequeue(p);
	dequeue(p);
	len = lenqueue(p);
	printf("len=%d\n",len);
  enqueue(p, 12); enqueue(p, 2); enqueue(p, 19);
	len = lenqueue(p);
	printf("len=%d\n",len);
	x= peek(p,0);
	printf("x=%d\n",x);
	x= peek(p,1);
	printf("x=%d\n",x);
	x= peek(p,2);
	printf("x=%d\n",x);
	
	dequeue(p);
	dequeue(p);
	dequeue(p);
	len = lenqueue(p);
	printf("len=%d\n",len);
  enqueue(p, 12); enqueue(p, 2); enqueue(p, 19);
	len = lenqueue(p);
	printf("len=%d\n",len);
	x= peek(p,0);
	printf("x=%d\n",x);
	x= peek(p,1);
	printf("x=%d\n",x);
	x= peek(p,2);
	printf("x=%d\n",x);
	
}

// 以下、使用例
int qmain(int p) {
  init_all_queue();

  printf("%s\n", empty(p) ? "queue is empty." : "queue is not empty.");  

  enqueue(p, 12); enqueue(p, 2); enqueue(p, 19);
  printf("%s\n", empty(p) ? "queue is empty." : "queue is not empty.");
    
  while(!empty(p))
    printf("*%d ", dequeue(p));
  putchar('\n');
  printf("%s\n", empty(p) ? "queue is empty." : "queue is not empty.");

  enqueue(p, 99); enqueue(p, 10); enqueue(p, 1);
  enqueue(p, 45);    // キューがフルで、追加できない。

  while(!empty(p))
    printf("%d ", dequeue(p));
  putchar('\n');
    
  dequeue(p);      // キューが空なので、取り出すものはない。

  return 0;
}
