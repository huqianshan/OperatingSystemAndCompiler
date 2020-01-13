#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* for uint32_t */
#include <string.h>

#include "bit_map.h"

int init_bitmap(int num)
{
  int size = (num / BIT_WIDTH_IN_BITS) + 1;
  int *tem = calloc(size, BIT_WIDTH_IN_BYTES);
  if (tem == NULL)
  {
    printf("Init BitMap Failed;Malloc return Null\n");
    return -1;
  }
  BitMap = tem;
  printf("Bitmap address 0x%x  size: %d (int) %d(bytes)\n", BitMap, size, size << 2);
  return 0;
}

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x)
{
  /* So So difficult
  This method is based on Divide and Conquer
  Is also known as haming weight "popcount" or "sideways addition"
  'variable-precision SWAR algorithm'
  References 1.https://stackoverflow.com/questions/3815165/how-to-implement-bitcount-using-only-bitwise-operators
             2.http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
             3.https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
  */
  int c = 0;
  int v = x;
  c = (v & 0x55555555) + ((v >> 1) & 0x55555555);
  c = (c & 0x33333333) + ((c >> 2) & 0x33333333);
  c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
  c = (c & 0x00FF00FF) + ((c >> 8) & 0x00FF00FF);
  c = (c & 0x0000FFFF) + ((c >> 16) & 0x0000FFFF);
  return c;
}

/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x)
{
  //This program takes refrence to the Internet
  //First consider the First 16 bit of x,if x>0xffff then the last 16 bit is useless so we can do right shift
  //After the right shift,what is left is the original First 16 bits
  //t records the answer
  //use (!!x) as a representation of (x!=0)
  //use bit-or to do add operation
  //return 2;
  return 2;
}

void printb(int len)
{
  int i;
  int tem;
  printf("\nBitMap Information\n");
  for (i = 0; i < len; i++)
  {
    tem = BOOL(i);
    printf("%d", tem);

    if ((i + 1) % (BIT_WIDTH_IN_BITS * 2) == 0)
    {
      printf("\n");
    }
    else if ((i + 1) % BIT_WIDTH_IN_BITS == 0)
    {
      printf("    ");
    }
    else if ((i + 1) % 8 == 0)
    {
      printf("  ");
    }
    else if ((i + 1) % 4 == 0)
    {
      printf(" ");
    }
  }
  prints(len);
  print_ait(len);
  printf("\n");
}

void prints(int len)
{
  int size = (len / BIT_WIDTH_IN_BITS) ;

  // calcaulate every usage of int in bitmap
  int *tem = malloc(sizeof(int) * size);
  int i, total = 0;
  for (i = 0; i < size; i++)
  {
    tem[i] = bitCount(BitMap[i]);
    total += tem[i];
  }
  printf("\nUsed bits: %d     Free bits %d \n", total, len - total);

  if (tem)
    free(tem);
}

int init_ait(word_t size)
{
  int page_num = (size / PAGE_SIZE) + 1;
  int *tmp_pointer = calloc(page_num, sizeof(word_t));
  if (tmp_pointer == NULL)
  {
    printf("malloc for Access Info Table failed pagesize: %d\n", page_num);
    return -3;
  }
  AIT = tmp_pointer;
  printf("AIT address: 0x%x page_num: %d\n", AIT, page_num);
  return 0;
}

int update_ait(word_t pos)
{
  int key = KEY_TABLE(pos);

  AIT[key]++;

  return 0;
}

word_t get_ait(word_t pos)
{
  return AIT[KEY_TABLE(pos)];
}

void print_ait(int len)
{
  int i;
  int length = len / PAGE_SIZE + 1;

  printf("------------Access Information Table-------------------\n");
  for (i = 0; i < length; i++)
  {
    printf("Page:%u Access:%u    ", i, AIT[i]);
  }
  printf("\n");
}

int main()
{
  int size;
  scanf("%d", &size);
  init_bitmap(size);
  init_ait(size);

  int num;
  char code;
  while (1)
  {
    scanf("%c%d", &code, &num);

    if (code == 's')
    { 
      if(num>size){
        printf("%u exceed size %u \n", num, size);
        continue;
      }
      SET_BITMAP(num);
      update_ait(num);
    }
    else if (code == 'c')
    {
      CLEAR_BITMAP(num);
    }
    else
    {
      continue;
    }

    printb(size);
  }

  if (BitMap)
  {
    free(BitMap);
  }
  if (AIT)
  {
    free(AIT);
  }
}