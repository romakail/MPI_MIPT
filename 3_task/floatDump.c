#include "stdio.h"
#include "unistd.h"

#define SGN_SIZE 1
#define EXP_SIZE 8
#define MAN_SIZE 23

#define SGN_SHIFT (EXP_SIZE + MAN_SIZE)
#define EXP_SHIFT (MAN_SIZE)

#define EXP_MASK ((1 << EXP_SIZE) - 1)
#define MAN_MASK ((1 << MAN_SIZE) - 1)

#define SGN(x) (x >> (EXP_SIZE + MAN_SIZE))
#define EXP(x) ((x >> MAN_SIZE) & EXP_MASK)
#define MAN(x) (x & MAN_MASK)

void printBin(unsigned int val, unsigned int size)
{
  while (size > 0)
  {
    size--;
    printf("%01d", (val >> size) & 1);
  }
  printf(" ");
}

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    printf("Usage:\n -f for float\n -i for int\n -x for hex\n exit: (value == 0)\n");
    return 0;
  }
  int opt = getopt(argc, argv, "fix");
  unsigned int val = 0;
  float* valAsFloat = (float*) &val;
  *valAsFloat = 2.0f;
  
  while (1)  
  {
    printf("\n%f : %08x\n", *valAsFloat, val);
    printBin(SGN(val), SGN_SIZE);
    printBin(EXP(val), EXP_SIZE);
    printBin(MAN(val), MAN_SIZE);
    printf("\n\n");

    if (val == 0)
    {
      return 0;
    }

    val = 0;
    switch (opt)
    {
    case 'f':
      printf("New value (float): ");
      scanf("%f", valAsFloat);
      break;
    case 'i':
      printf("New value (int):\n sgn: ");
      unsigned int tmp = 0;
      scanf("%d", &tmp);
      val |= tmp << SGN_SHIFT;
      printf(" exp: ");
      scanf("%d", &tmp);
      val |= tmp << EXP_SHIFT;
      printf(" man: ");
      scanf("%d", &tmp);
      val |= tmp;
      break;
    case 'x':
      printf("New value (hex): ");
      scanf("%x",&val);
      break;
    default:
      printf("Wrong argument: %c\n", opt);
      printf("Usage:\n -f for float\n -i for int\n -x for hex\n");
      return 0;
    }
  }
  return 0;
}
