#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/module.h>

#define DEVICES_NAME "test"

int info_table_size = 262144;
typedef unsigned int word_t;
word_t *init_infotable(word_t size)
{
  word_t v_size;
  word_t *tem;

  v_size = size * (sizeof(word_t));
  tem = vzalloc(v_size);

  if (tem == NULL)
  {
    printk(KERN_ERR "NVMSIM: [%s(%d)]: init InfoTable size: %u failed  \n",
           __FUNCTION__, __LINE__, size);
  }
  else
  {
    printk(KERN_INFO
           "NVMSIM: [%s(%d)]: init InfoTable size: %u success addr: %x\n",
           __FUNCTION__, __LINE__, size, tem);
  }
  return tem;
}

int update_infotable(word_t *InfoTable, word_t lbn)
{
  // exceed the info table range
  if (lbn >= info_table_size)
  {
    return 1;
  }

  InfoTable[lbn] += 1;
  return 0;
}

word_t get_infotable(word_t *InfoTable, word_t lbn)
{
  // exceed the info table range
  if (lbn >= info_table_size || InfoTable == NULL)
  {
    return 0;
  }

  return InfoTable[lbn];
}

int destroy_infotable(word_t *InfoTable)
{
  if (InfoTable != NULL)
  {
    vfree(InfoTable);
    printk(KERN_INFO "NVM_SIM [%s(%d)]: free Infotable \n", __FUNCTION__,
           __LINE__);
    return 0;
  }
  return 1;
}

void print_infotable(word_t *InfoTable, word_t size, word_t step)
{
  if (size >= info_table_size)
  {
    return;
  }
  word_t i, tem;
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Information Table Summary \n",
         __FUNCTION__, __LINE__);
  printk(KERN_INFO "   LBN    AccessTime\n", __FUNCTION__, __LINE__);
  for (i = 0; i < size; i += step)
  {
    tem = get_infotable(InfoTable, i);
    if (tem != 0)
    {
      printk(KERN_INFO " %6u  %6u \n", i, tem);
    }
  }
}
word_t *t;

void info_init_test(void)
{
  t = init_infotable(info_table_size);
  update_infotable(t, 0);
  update_infotable(t, 1);
  update_infotable(t, 2);
  update_infotable(t, 2);
  update_infotable(t, 33);
  print_infotable(t, 100, 1);
}

void info_exit_test(void)
{
  if (t == NULL)
  {
    printk(KERN_INFO "NVM_SIM [%s(%d)]: t is NULL\n", __FUNCTION__, __LINE__);
  }
  else
  {
    printk(KERN_INFO "NVM_SIM [%s(%d)]: %p\n", __FUNCTION__, __LINE__, t);
    word_t tem;
    tem = get_infotable(t, 33); // t=NULL.&t equals to (&NULL)=
    printk("%u", tem);
    tem = get_infotable(t, 3300); // t=NULL.&t equals to (&NULL)=
    printk("%u", tem);
    destroy_infotable(t);
  }
}

int map_table_size = 8388608;
word_t *init_maptable(word_t size)
{
  word_t table_num;
  word_t *tem;
  unsigned long v_size;

  table_num = size + 1;
  v_size = (unsigned long)table_num * sizeof(word_t);
  tem = vzalloc(v_size);
  if (tem == NULL)
  {
    printk(KERN_ERR "NVMSIM: %s(%d) init maptable size: %u failed  \n",
           __FUNCTION__, __LINE__, table_num);
  }
  else
  {
    printk(KERN_INFO "NVMSIM: %s(%d) init MapTable size: %u success addr: %x\n",
           __FUNCTION__, __LINE__, table_num, tem);
  }
  return tem;
};

int update_maptable(word_t *map_table, word_t lbn, word_t pbn)
{
  if (lbn > map_table_size || lbn < 0)
  {
    printk(KERN_ERR "NVMSIM: [%s(%d)] Error index %u exceed %u,update maptble failed\n",
           __FUNCTION__, __LINE__, lbn, map_table_size);
    return 0;
  }
  map_table[lbn] = pbn;
  return 1;
};

word_t get_maptable(word_t *map_table, word_t lbn)
{
  if (lbn > map_table_size || (lbn < 0))
  {
    printk(KERN_ERR "NVMSIM: [%s(%d)] Error index %u exceed %u,gete maptble failed\n",
           __FUNCTION__, __LINE__, lbn, map_table_size);
    return 0; //return -EINVAL; bug
  }
  return map_table[lbn];
};

word_t map_table(word_t *map_table, word_t lbn, word_t pbn)
{
  word_t raw_pbn;
  // check for return value
  raw_pbn = get_maptable(map_table, lbn);
  // if pbn=0 indicates lbn not mapping
  //num = ACCESS_TIME(pbn) + 1;newkey = MAKE_KEY(pbn, num);
  if ((update_maptable(map_table, lbn, pbn) == 0))
  {
    printk(KERN_ERR "NVMSIM: [%s(%d)] Error Map lbn %u for pbn %u failed\n",
           __FUNCTION__, __LINE__, lbn, pbn);
    return 0;
  }
  return raw_pbn;
};

int demap_maptable(word_t *map_table, word_t lbn)
{
  word_t raw_pbn;
  // check for return value
  raw_pbn = get_maptable(map_table, lbn);
  if (raw_pbn == 0)
  {
    return 0;
  }
  // if pbn=0 indicates lbn not mapping
  //num = ACCESS_TIME(pbn) + 1;newkey = MAKE_KEY(pbn, num);
  if ((update_maptable(map_table, lbn, 0) == 0))
  {
    printk(KERN_ERR "NVMSIM: [%s(%d)] Error DeMap lbn %u failed its raw pbn %u n",
           __FUNCTION__, __LINE__, lbn, raw_pbn);
    return 0;
  }
  return raw_pbn;
}

void print_maptable(word_t *map_table, word_t lbn)
{
  word_t i, pbn;
  printk(KERN_INFO
         "NVMSIM: %s(%d)\n------------Mapping Table-----------------\n",
         __FUNCTION__, __LINE__);
  printk(KERN_INFO "    lbn      pbn      accesstime            \n");
  for (i = 0; i <= 100; i++)
  {
    pbn = get_maptable(map_table, i);

    if (pbn != 0)
    // bug ,=0->0 not show ;solved, pbn begin with [1,size]
    {
      printk(KERN_INFO "%8u%8u%8u\n", i, pbn, 0);
    }
  }

  for (i = 0; i <= lbn; i += 5000)
  {
    pbn = get_maptable(map_table, i);
    if (pbn != 0)
    // bug ,=0->0 not show ;solved, pbn begin with [1,size]
    {
      printk(KERN_INFO "%8u%8u%8u\n", i, pbn, 0);
    }
  }
};

void destroy_maptable(word_t *map_table)
{
  if (map_table != NULL)
  {
    vfree(map_table);
    printk("destroy maptable");
  }
};

word_t *map;
void map_init_test(void)
{
  map = init_maptable(map_table_size);
  int i;
  if (map == NULL)
  {
    return;
  }
  for (i = 0; i < 100; i += 1)
  {
    map_table(map, i, i + 1);
  }

  print_maptable(map, map_table_size);
}

void map_exit_test(void)
{
  destroy_maptable(map);
}

static int __init test_init(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Init\n", __FUNCTION__, __LINE__);
  info_init_test();
  map_init_test();
  return 0;
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Test [%s(%d)]: Exit\n", __FUNCTION__, __LINE__);
  info_exit_test();
  map_exit_test();
  return;
}

module_init(test_init);
module_exit(test_exit);
