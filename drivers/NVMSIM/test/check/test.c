#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/module.h>

#define DEVICES_NAME "test"

int info_table_size = 262144;
typedef unsigned int word_t;
word_t *init_infotable(word_t size) {
  word_t v_size;
  word_t *tem;

  v_size = size * (sizeof(word_t));
  tem = vzalloc(v_size);

  if (tem == NULL) {
    printk(KERN_ERR "NVMSIM: [%s(%d)]: init InfoTable size: %u failed  \n",
           __FUNCTION__, __LINE__, size);
  } else {
    printk(KERN_INFO
           "NVMSIM: [%s(%d)]: init InfoTable size: %u success addr: %x\n",
           __FUNCTION__, __LINE__, size, tem);
  }
  return tem;
}

int update_infotable(word_t *InfoTable, word_t lbn) {
  // exceed the info table range
  if (lbn >= info_table_size) {
    return 1;
  }

  InfoTable[lbn] += 1;
  return 0;
}

word_t get_infotable(word_t *InfoTable, word_t lbn) {
  // exceed the info table range
  if (lbn >= info_table_size||InfoTable==NULL) {
    return 0;
  }

  return InfoTable[lbn];
}

int destroy_infotable(word_t *InfoTable) {
  if (InfoTable!=NULL) {
    vfree(InfoTable);
    printk(KERN_INFO "NVM_SIM [%s(%d)]: free Infotable \n", __FUNCTION__,
           __LINE__);
    return 0;
  }
  return 1;
}

void print_infotable(word_t *InfoTable, word_t size, word_t step) { 
  if(size>=info_table_size){
    return;
  }
  word_t i,tem;
  printk(KERN_INFO "NVM_SIM [%s(%d)]: Information Table Summary \n",
         __FUNCTION__, __LINE__);
  printk(KERN_INFO"   LBN    AccessTime\n",__FUNCTION__,__LINE__);
  for (i = 0; i < size;i+=step){
    tem=get_infotable(InfoTable, i);
    if(tem!=0){
      printk(KERN_INFO" %6u  %6u \n",i,tem);
    }
  }
}
word_t *t;

void info_init_test(void){
    t = init_infotable(info_table_size);
    update_infotable(t, 0);
    update_infotable(t, 1);
    update_infotable(t, 2);
    update_infotable(t, 2);
    update_infotable(t, 33);
    print_infotable(t, 100, 1);
}

void info_exit_test(void){
    if(t==NULL){
      printk(KERN_INFO"NVM_SIM [%s(%d)]: t is NULL\n",__FUNCTION__,__LINE__);
    }else{
      printk(KERN_INFO"NVM_SIM [%s(%d)]: %p\n",__FUNCTION__,__LINE__,t);
    word_t tem;
    tem=get_infotable(t, 33);  // t=NULL.&t equals to (&NULL)=
    printk("%u",tem);
    tem=get_infotable(t, 3300);  // t=NULL.&t equals to (&NULL)=
    printk("%u",tem);
    destroy_infotable(t);
    }
}
static int __init test_init(void){
    printk(KERN_INFO"Test [%s(%d)]: Init\n",__FUNCTION__,__LINE__);
    info_init_test();
    return 0;
}

static void __exit test_exit(void){
    printk(KERN_INFO"Test [%s(%d)]: Exit\n",__FUNCTION__,__LINE__);
    info_exit_test();
    return;
}

module_init(test_init);
module_exit(test_exit);
