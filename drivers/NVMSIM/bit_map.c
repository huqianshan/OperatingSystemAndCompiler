#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* for uint32_t */
#include <string.h>
#include <math.h>

#include "bit_map.h"
#include "mapping.h"
#include "kth.h"

#define DEBUG

int init_bitmap(int num)
{
  int size = (num / BIT_WIDTH_IN_BITS) + 1; // ceil
  word_t *tem = calloc(size, BIT_WIDTH_IN_BYTES);
  if (tem == NULL)
  {
    printf("Init BitMap Failed;Malloc return Null\n");
    return -1;
  }
  BitMap = tem;
  printf("Bitmap address 0x%x  size: %d (int) %u(bytes)\n", BitMap, size, size * BIT_WIDTH_IN_BYTES);
  return 0;
}

word_t query_bitmap(word_t pos)
{

  // find the first free block of page pos
  word_t page_off = INT_OFFSET(pos);
  word_t block_begin = page_off * BIT_WIDTH_IN_BITS;
  word_t block_end = (page_off + 1) * BIT_WIDTH_IN_BITS;

  int i;
  for (i = block_begin; i < block_end; i++)
  {
    if (BOOL(i) == 0)
    {
      return i;
    }
  }
#ifdef DEBUG
  printf("Query pos %u return %u Failed\n", pos, i - block_begin);
#endif
  return -5;
}

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
word_t bitCount(word_t x)
{
  /* 
  This method is based on Divide and Conquer
  Is also known as haming weight "popcount" or "sideways addition"
  'variable-precision SWAR algorithm'
  References 1.https://stackoverflow.com/questions/3815165/how-to-implement-bitcount-using-only-bitwise-operators
             2.http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
             3.https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
  */
  word_t c = 0;
  word_t v = x;
  c = (v & 0x55555555) + ((v >> 1) & 0x55555555);
  c = (c & 0x33333333) + ((c >> 2) & 0x33333333);
  c = (c & 0x0F0F0F0F) + ((c >> 4) & 0x0F0F0F0F);
  c = (c & 0x00FF00FF) + ((c >> 8) & 0x00FF00FF);
  c = (c & 0x0000FFFF) + ((c >> 16) & 0x0000FFFF);
  return c;
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
  //print_ait(len);
  print_maptable(len);
  printf("\n");
}

void prints(int len)
{
  int size = (len / BIT_WIDTH_IN_BITS) + 1; //ceilq

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

word_t set_helper(word_t lbn)
{
  //1. find maptable
  word_t key = get_maptable(lbn);
  word_t pbn = PHYSICAL_PAGE_NUM(key);

  if (pbn == 0)
  {
    // not map
    pbn = query_bitmap(lbn);
    printf("pbn: %u\n", pbn);
    SET_BITMAP(pbn);
    map_maptable(lbn, pbn + 1); // map table not begin with 0 but 1
  }
  else
  {
    // only add acces time
    word_t num = ACCESS_TIME(key) + 1;
    word_t newkey = MAKE_KEY(pbn, num);
    if (update_maptable(lbn, newkey) != 0)
    {
      printf("update accesstime in maptable  failed\n");
      return 0;
    }
  }
  return pbn;
}

word_t read_helper(word_t lbn)
{
  // 1. find maptable
  word_t key = get_maptable(lbn);
  word_t pbn = PHYSICAL_PAGE_NUM(key);
  return pbn;
}

void clear_helper(word_t lbn)
{
  //1. find maptable
  word_t key = get_maptable(lbn);
  word_t pbn = PHYSICAL_PAGE_NUM(key);

  if (pbn == 0)
  {
    return;
  }

  printf("Clear lbn: %u pbn:%u\n", lbn, pbn);
  CLEAR_BITMAP((pbn - 1));
  demap_maptable(lbn);
}

word_t wear_leavel(word_t size)
{
  // 1. get keys and index array of maptable
  word_t *arr = NULL; // key
  word_t *index = NULL; // lbn
  word_t n = extract_maptable(size, &arr, &index);
  printf("[%s()] keys %x index %x size %u\n", __FUNCTION__, arr, index, n);

  if ((!arr) || (!index))
  {
    // checkpoint
    return 0;
  }

  // make an arry with key value of accesstime not key(pbn+accesstime)

  word_t *tarr = malloc(sizeof(word_t) * n);
  for (int j = 0; j < n; j++)
  {
    tarr[j] = ACCESS_TIME(arr[j]);
    printf("[%s()] tarr %d:%u:%u\n",__FUNCTION__,j, index[j], tarr[j]);
  }
  printf("\n");

  word_t k = ceil(0.2 * n);
  printf("[%s()] k:%u\n", __FUNCTION__, k);
  word_t *bigK = bigK_index(tarr, n, k);
  for (int j = 0; j < k;j++){
    printf("[%s()] bigK %d:%u lbn:%u pbn:%u times %u\n",__FUNCTION__,
     j, bigK[j],index[bigK[j]],PHYSICAL_PAGE_NUM(arr[bigK[j]]) ,tarr[bigK[j]]);
  }
  word_t *smallK = smallK_index(tarr, n, k);

  word_t tem, bindex, sindex, blbn, slbn, bkey, skey;
  for (int i = 0; i < k; i++)
  {
    // bindex one of the big k index
    bindex = bigK[i];
    blbn = index[bindex];
    bkey = arr[bindex];

    sindex = smallK[i];
    slbn = index[sindex];
    skey = arr[sindex];

    map_maptable(blbn, PHYSICAL_PAGE_NUM(skey));
    printf("[%s()] blbn %u skey %u pbn: %u access %u\n",
           __FUNCTION__, blbn, skey, PHYSICAL_PAGE_NUM(skey), ACCESS_TIME(skey));

    // here transfer data

    map_maptable(slbn, PHYSICAL_PAGE_NUM(bkey));
    printf("[%s()] slbn %u bkey %u pbn: %u access %u\n",
           __FUNCTION__, slbn, bkey, PHYSICAL_PAGE_NUM(bkey), ACCESS_TIME(bkey));
  }
  if (bigK && smallK && tarr)
  {
    free(bigK);
    free(smallK);
    free(tarr);
  }
  if (arr && index)
  {
    free(arr);
    free(index);
  }
  return 1;
}

int main()
{
  int size; //8338608
  scanf("%d", &size);
  init_bitmap(size);
  //init_ait(size);
  init_maptable(size);

  word_t lbn;
  char code;
  while (1)
  {
    scanf("%c%d", &code, &lbn);
    if (code == 's')
    {
      if (lbn >= size) //pos begin with 0
      {
        printf("%u exceed size %u \n", lbn, size);
      }
      set_helper(lbn);
    }
    else if (code == 'c')
    {
      clear_helper(lbn);
    }
    else if (code == 'q')
    {
      word_t pbn = query_bitmap(lbn);
      printf("Query lbn:%u return pbn:%u\n", lbn, pbn);
    }
    else if (code == 'r')
    {
      word_t pbn = read_helper(lbn); // map return bigger than bigtmap
      if (pbn == 0)
      {
        printf("Read failed\n");
      }
      else
      {
        printf("Read lbn:%u return pbn:%u\n", lbn, pbn);
      }
    }
    else if (code == 'e')
    {
      word_t *arr = NULL;
      word_t *index = NULL;
      word_t n = extract_maptable(size, &arr, &index);
      printf("%x %x\n", arr, index);
      if (arr && index)
      {
        for (int i = 0; i < n; i++)
        {
          printf("lbn %u pbn %u access %u\n", index[i], PHYSICAL_PAGE_NUM(arr[i]), ACCESS_TIME(arr[i]));
        }
        free(arr);
        free(index);
      }
    }
    else if (code == 'w')
    {
      wear_leavel(size);
    }else if(code=='p'){
      printb(size);
    }
    else if(code=='b'){
      word_t count = bitCount(lbn);
      printf("%u has %u 1 bits\n", lbn, count);
    }
    else
    {
      continue;
    }
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