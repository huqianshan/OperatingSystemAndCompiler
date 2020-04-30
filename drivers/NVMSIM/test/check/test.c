#include <linux/kernel.h>
#include <linux/module.h>
#include "test.h"
#include "syncer.h"

word_t *MapTable;
word_t *BitMap;
word_t *InfoTable;

void pbi_test_init(void)
{
  MapTable = init_maptable(map_table_size);
  BitMap = init_bitmap(bit_table_size);
  InfoTable = init_infotable(info_table_size);
}

void pbi_test_alloc(word_t begin, word_t end, word_t repeat)
{

  word_t i, j;

  if ((!MapTable) || (!BitMap) || (!InfoTable))
  {
    return;
  }
  for (i = begin; i < end; i += 1)
  {
    map_table(MapTable, i, i + 1);
    SET_BITMAP(i, BitMap);
    for (j = 0; j < repeat; j++)
    {
      update_infotable(InfoTable, i >> INFO_PAGE_SIZE_SHIFT);
    }
  }
}

void pbi_weal_test(void)
{
  if ((!MapTable) || (!BitMap) || (!InfoTable))
  {
    return;
  }

  word_t *tem, rtn;
  word_t k;
  k = 1;
  rtn = extract_big(BitMap, InfoTable, bit_table_size >> 5, k, &tem);
  if (rtn)
  {
    word_t j;
    for (j = 0; j < k; j++)
    {
      printk("lpn %u times:%u avg: %u\n", tem[j], InfoTable[tem[j]], rtn);
    }
  }

  wear_level(BitMap, InfoTable, MapTable, tem, k);
  auto_free(tem, k * sizeof(word_t));
}

void pbi_test_destroy(void)
{
  destroy_maptable(MapTable);
  destroy_infotable(InfoTable);
  destroy_bitmap(BitMap);
}

void pbi_test_print(word_t len)
{
  if ((!MapTable) || (!BitMap) || (!InfoTable))
  {
    return;
  }
  print_maptable(MapTable, len);
  print_bitmap(BitMap, len);
  print_infotable(InfoTable, len, 1);
}

void syncer_malloc_test(void)
{
  word_t *p;
  word_t size, word_t_t_size;

  size = 32;
  word_t_t_size = sizeof(word_t);
  printk("p %p\n", p);
  p = auto_malloc(size * word_t_t_size);
  auto_free(p, size * word_t_t_size);
  printk("p %p\n", p);

  size = KZALLOC_MAX_BYTES;
  printk("p %p\n", p);
  p = auto_malloc(size * word_t_t_size);
  auto_free(p, size * word_t_t_size);
  printk("p %p\n", p);

  size = 64 * 1024 * 1024;
  printk("p %p\n", p);
  p = auto_malloc(size * word_t_t_size);
  auto_free(p, size * word_t_t_size);
  printk("p %p\n", p);

  p = NULL;
  auto_free(p, 32);
  auto_free(p, KZALLOC_MAX_BYTES + 1);
}

void syncer_find_test(void)
{
  word_t array[5] = {2, 2, 2, 2, 2};
  word_t index, key;

  index = minium(array, 5);
  key = array[index];
  printk("min index: %u key %u\n", index, key);

  index = maxium(array, 5);
  key = array[index];
  printk("max index: %u key %u\n", index, key);

  word_t *karr, k, i;
  k = 3;
  karr = bigK_index(array, 5, k);
  for (i = 0; i < k; i++)
  {
    printk("big K:%u %u:%u Real %u\n", k, i, karr[i], array[karr[i]]);
  }

  //word_t *karr, k, i;
  k = 5;
  karr = smallK_index(array, 5, k);
  for (i = 0; i < k; i++)
  {
    printk("small K:%u %u:%u Real %u\n", k, i, karr[i], array[karr[i]]);
  }
  auto_free(karr, k * sizeof(word_t));
}

void syncer_extracted_test(void)
{
  word_t *bitmap, *infotable, *maptable;
  word_t s;
  s = 5;
  //bitmap = init_bitmap(s * BIT_WIDTH_IN_BITS);
  //infotable = init_infotable(s);
  bitmap = auto_malloc(s * sizeof(word_t));
  infotable = auto_malloc(s * sizeof(word_t));
  maptable = auto_malloc(s * sizeof(word_t) * BIT_WIDTH_IN_BITS);
  if (!bitmap || !infotable || !maptable)
    return;

  SET_BITMAP(1, bitmap);
  map_table(maptable, 1, 2);
  SET_BITMAP(2, bitmap);
  map_table(maptable, 2, 3);
  SET_BITMAP(33, bitmap);
  map_table(maptable, 33, 34);
  SET_BITMAP(65, bitmap);
  map_table(maptable, 65, 66);
  SET_BITMAP(97, bitmap);
  map_table(maptable, 97, 98);
  SET_BITMAP(129, bitmap);
  map_table(maptable, 129, 130);

  infotable[0] = 1;
  infotable[1] = 2;
  infotable[2] = 3;
  infotable[3] = 4;
  infotable[4] = 5;
  print_bitmap(bitmap, 32 * s);
  print_infotable(infotable, s, 1);
  print_maptable(maptable, 160);

  word_t *tem, rtn;
  word_t k;
  k = 1;
  rtn = extract_big(bitmap, infotable, s, k, &tem);
  if (rtn)
  {
    word_t j;
    for (j = 0; j < k; j++)
    {
      printk("lpn %u times:%u \n", tem[j], infotable[tem[j]]);
    }
  }
  wear_level(bitmap, infotable, maptable, tem, k);
  print_bitmap(bitmap, 32 * s);
  print_infotable(infotable, s, 1);
  print_maptable(maptable, 160);

  auto_free(tem, k * sizeof(word_t));

  auto_free(infotable, s * sizeof(word_t));

  auto_free(bitmap, s * sizeof(word_t));
}

static int __init test_init(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Init\n", __FUNCTION__, __LINE__);

  pbi_test_init();
  pbi_test_alloc(0, 36, 10);
  pbi_test_print(64);

  pbi_weal_test();
  pbi_test_print(64);

  //syncer_malloc_test();
  //syncer_find_test();
  //syncer_extracted_test();
  return 0;
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Exit\n", __FUNCTION__, __LINE__);
  pbi_test_destroy();
  return;
}

module_init(test_init);
module_exit(test_exit);
