#include <stdio.h>
#include <ctype.h>
#include "define.h"

//#define QUEUE_MAX 5     // �L���[�̌�
#define QUEUE_SIZE 400     // �L���[�̑傫��
#define INF -99    // �L���[����ŁA�f�L���[�������ɕԂ閳���Ȓl
typedef unsigned char ELEM;        // �L���[�̗v�f�̌^
ELEM queue[QUEUE_MAX][QUEUE_SIZE];  // �L���[�̒�`
int front[QUEUE_MAX];               // �L���[�̐擪
int rear[QUEUE_MAX];                // �L���[�̖��� �����̎��̗v�f���w��

// ���̗v�f�̓Y���������߂�
int next(int index) {
  return (index + 1) % QUEUE_SIZE;
}

// ���ׂẴL���[������������
void init_all_queue(void) {
	int i;
	for( i = 0; i < QUEUE_MAX; i++) {
		front[i] = rear[i] = 0;
	}
}

// �L���[������������
void init_queue(int p) {
  front[p] = rear[p] = 0;
}

// �L���[�Ƀf�[�^������
void enqueue(int p, ELEM x) {
  if (next(rear[p]) == front[p]) {
//     printf("cannot enqueue. Because queue is full.\n");
     return;
  }
  queue[p][rear[p]] = x;
  rear[p] = next(rear[p]);
}

// �L���[����f�[�^�����o���B�f�[�^���Ȃ���INF(�����Ȓl)��Ԃ��B
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

// �L���[���󂩂ǂ����𒲂ׂ�B��Ȃ�P�A��łȂ���΂O��Ԃ�
int empty(int p) {
  return  front[p] == rear[p];
}

// �L���[�̂���f�[�^�̃T�C�Y�����߂�
int lenqueue(int p) {
	int len = rear[p] - front[p];
	if (len < 0) {
		len += QUEUE_SIZE;
	}
	return len;
}

// �L���[�̎w��ʒu�̃f�[�^��ǂ�
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

// �ȉ��A�g�p��
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
  enqueue(p, 45);    // �L���[���t���ŁA�ǉ��ł��Ȃ��B

  while(!empty(p))
    printf("%d ", dequeue(p));
  putchar('\n');
    
  dequeue(p);      // �L���[����Ȃ̂ŁA���o�����̂͂Ȃ��B

  return 0;
}
