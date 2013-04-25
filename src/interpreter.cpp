#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h>

#define OP_NON 0
#define OP_ADD 1
#define OP_SUB 2
#define OP_MUL 3
#define OP_DIV 4
#define OP_LDR 5
#define OP_JMP 6 
#define OP_JMZ 7
#define OP_COUNT 8

#define OPR_DIR 0
#define OPR_LIFE 1
#define OPR_AGE 2
#define OPR_MATE 3
#define OPR_SPAWN 4
#define OPR_NEIGH 5
#define OPR_REG 14
#define OPR_NUM 22

typedef unsigned char word_t;

struct instruction_t
{
  word_t op;
  word_t a, b, c;
};

word_t next_id = 0;

class creature_t
{
public:
  word_t id;
  word_t dir, life, age;
  word_t mate, spawn;
  word_t regs[8];
  word_t pc;
  instruction_t code[10];
  creature_t() {
    id = next_id ++;
    dir = 0;
    life = 100;
    age = 0;
    mate = 0;
    spawn = 0;
    pc = 0;
    memset(regs, 0, sizeof(word_t) * 8);
    memset(code, 0, sizeof(instruction_t) * 10);
  }
};

class cursor_t 
{
public:
  creature_t **map;
  int x, y;
  int  width, height;

  creature_t ** get_position(int dir)
  {
    int dx, dy;
    int nx, ny;
    dx = dir % 3 - 1;
    dy = dir / 3 - 1;
    nx = (x + dx + width) % width;
    ny = (y + dy + height) % height;
    return &map[ny * width + nx];
  }
  
  creature_t * get(int dir)
  {
    return *get_position(dir);
  }
};

word_t get_value(word_t opr, creature_t &creature, cursor_t & cursor)
{
  creature_t * prt;
  if (opr == OPR_DIR)
    return creature.dir;
  if (opr == OPR_LIFE)
    return creature.life;
  if (opr == OPR_AGE)
    return creature.age;
  if (opr == OPR_MATE)
    return creature.mate;
  if (opr == OPR_SPAWN)
    return creature.spawn;
  if (opr < OPR_REG) {
    prt = cursor.get(opr - OPR_NEIGH);
    return !prt ? 0 : prt->life + 1;
  }
  if (opr < OPR_NUM)
    return creature.regs[opr - OPR_REG];
  return opr - OPR_NUM;
}

word_t* get_var(word_t opr, creature_t &creature, cursor_t & cursor)
{
  if (opr == OPR_DIR)
    return &creature.dir;
  if (opr == OPR_MATE)
    return &creature.mate;
  if (opr == OPR_SPAWN)
    return &creature.spawn;
  if (opr < OPR_REG)
    return NULL;
  if (opr < OPR_NUM)
    return &creature.regs[opr - OPR_REG];
  return NULL;
}

creature_t * mate(creature_t &c1, creature_t &c2)
{
  creature_t *c3;
  int side;
  c3 = new creature_t();
  for (int i = 0; i < 10; i++) {
    side = rand() % 2;
    if (side)
      c3->code[i] = c1.code[i];
    else
      c3->code[i] = c2.code[i];
  } 
  return c3;
}

creature_t * clone(creature_t &c1)
{
  creature_t *c2;
  c2 = new creature_t();
  for (int i = 0; i < 10; i++)
    c2->code[i] = c1.code[i];
  c1.life /= 2;
  return c2;
}

creature_t * zap_into_existence()
{
  creature_t *c1;
  c1 = new creature_t();
  for (int i = 0; i < 10; i++) {
    c1->code[i].op = rand() % OP_COUNT;
    c1->code[i].a = rand() % 256;
    c1->code[i].b = rand() % 256;
    c1->code[i].c = rand() % 256;
  } 
  return c1;
}

void kill(creature_t *creature)
{
  delete creature;
}

void run_creature(creature_t &creature, cursor_t &cursor)
{
  word_t v1, v2;
  word_t *var;
  instruction_t inst;
  for (int i = 0; i < 10 && creature.pc < 10; i++, creature.pc++) {
    inst = creature.code[creature.pc];
    v1 = get_value(inst.a, creature, cursor);
    v2 = get_value(inst.b, creature, cursor);
    var = get_var(inst.c, creature, cursor);
    if (inst.op < OP_JMP && var == NULL) {
      creature.life = 0;
      return;
    }
    switch (inst.op) {
    case OP_ADD:
      *var = v1 + v2;
      break;
    case OP_SUB:
      *var = v1 - v2;
      break;
    case OP_MUL:
      *var = v1 * v2;
      break;
    case OP_DIV:
      *var = v1 / v2;
      break;
    case OP_LDR:
      if (v1 >= 8) {
	creature.life = 0;
	return;
      }	
      *var = creature.regs[v1];
      break;
    case OP_JMP:
      if (v1 >= 10) {
	creature.life = 0;
	return;
      }	
      i = v1;
      break;
    case OP_JMZ:
      if (v2 >= 10) {
	creature.life = 0;
	return;
      }	
      if (v1 == 0)
	i = v2;
      break;
    }
  }
}

#define MIN(x,y) ((x) > (y) ? (x) : (y))

void run_cycle(creature_t **old_map, creature_t **new_map, int width, int height)
{
  int x, y, i, life;
  creature_t *c1, *c2, **position;
  cursor_t cursor;
  i = 0;
  cursor.width = width;
  cursor.height = height;
  cursor.map = old_map;
  memset(new_map, 0, sizeof(creature_t *) * width * height);
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++, i++) {
      c1 = old_map[i];
      if (c1 == NULL)
	continue;
      if (!c1->life) {
	kill(c1);
	continue;
      }
      cursor.x = x;
      cursor.y = y;
      cursor.map = old_map;
      run_creature(*c1, cursor);
      c1->life--;
      c1->age++;
      if (c1->spawn) {
	if (new_map[i])
	  kill(new_map[i]);
	new_map[i] = clone(*c1);
      }
      cursor.map = new_map;
      position = cursor.get_position(c1->dir);
      c2 = *position;
      printf("--> %d %p\n", i, c2);
      if (c2 != NULL) {
	if (c2->mate || c1->mate) {
	  if (new_map[i])
	    kill(new_map[i]);
	  new_map[i] = mate(*c1, *c2);	  
	}
	life = MIN(c2->life + c1->life, 100);
	if (c2->life > c1->life || c2->age < c1->age)
	  c2->life = life;
	else {
	  c1->life = life;
	  *c2 = *c1;
	}   
	kill(c1);
      } else
	*position = c1;
    }
}

void print_map(creature_t **map, int width, int height)
{
  int x, y, i;
  creature_t *c1;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++, i++) {
      c1 = map[y * width + x];
      printf("|%03d", c1 == NULL ? 0 : c1->id + 1);
    }
    printf("|\n");
  }
  printf("\n");
  printf("\e[1;1H\e[2J");
  usleep(50000);
}

creature_t ** create_map(int width, int height)
{
  creature_t **new_map;
  new_map = (creature_t **) malloc(sizeof(creature_t *) * width * height);
  memset(new_map, 0, sizeof(creature_t *) * width * height);
  return new_map;
}

void run(creature_t **map, int width, int height, int cycles)
{
  creature_t **new_map, **tmp;
  new_map = create_map(width, height);
  for (int i = 0; i < cycles; i++) {
    run_cycle(map, new_map, width, height);
    tmp = map;
    map = new_map;
    new_map = tmp;
    print_map(map, width, height);
  }
}

int main()
{
  creature_t **new_map;
  creature_t creature;
  int width, height;
  width = 30;
  height = 30;
  new_map = create_map(width, height);
  new_map[height / 2 * width + width / 2] = &creature;
  creature.code[0].op = OP_ADD;
  creature.code[0].a = OPR_NUM;
  creature.code[0].b = OPR_NUM + 1;
  creature.code[0].c = OPR_DIR;
  printf("--> %d\n",creature.life);
  run(new_map, width, height, 200);
}
