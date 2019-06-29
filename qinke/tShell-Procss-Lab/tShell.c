#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
* Todo
* 1.Delete only find pcb in ReadyList not include waitinglist
* 2.There is a lot of malloc but not properly free
* 3.Release Resource only from current running process Not the resource' Owner so it must be dead-lock
* 4. There is a lot of redundancy when finding  and insert  node and selecting 
*/

/*
* ------------- Global Var ------------------------
* -------------------------------------------------
*/

#define True 1
#define BUFFER_LEN 64
#define NAME_LEN 16

#define CR 0
#define DE 1
#define REQ 2
#define REL 3
#define TO 4
#define LS 5

#define READY 1
#define BLOCKED 2
#define RUNNING 3
/*
* ------------- Data Structure --------------------
* -------------------------------------------------
*/

struct PCB
{
    char pid[16];
    int priority;
    int status;
    struct rcbNode *blocked_list;
    int blocked_num;

    struct PCB *parent;
    struct pcbList *child;

    struct rcbList *resours;
};
typedef struct PCB PCB;

struct pcbNode
{

    PCB *pcb;
    struct pcbNode *next;

    //PCB *last;
};

typedef struct pcbNode pcbNode, pcbList;
// Priority List Todo
pcbList System_header;
pcbList User_header;
pcbList Init_header;

pcbList *System = &System_header;
pcbList *User = &User_header;
pcbList *Init = &Init_header;

// Resource Struct
typedef struct RCB
{
    int RID;
    int status;
    pcbList *waitListHeader;
} RCB;

struct rcbNode
{
    RCB *rcb;
    struct rcbNode *next;
};
typedef struct rcbNode rcbNode, rcbList;

RCB *R1;
RCB *R2;
RCB *R3;
RCB *R4;

// Current Running Process
PCB *CURRENT_RUNNING_PCB = NULL;

/*
* ------------- Helper Function -------------------
* -------------------------------------------------
*/
char *parser(char *des, char *src)
{
    char *str = src;
    int n = 0;
    while (*str == ' ' && *(str++) != '\n')
    {
    }

    while (*str != '\n' && *(str++) != ' ')
    {
        n++;
    }
    strncpy(des, src, n);

    return str;
}

int cmd_type(char *cmd)
{
    if (strcmp(cmd, "cr") == 0)
    {
        return CR;
    }
    else if (strcmp(cmd, "de") == 0)
    {
        return DE;
    }
    else if (strcmp(cmd, "req") == 0)
    {
        return REQ;
    }
    else if (strcmp(cmd, "rel") == 0)
    {
        return REL;
    }
    else if (strcmp(cmd, "to") == 0)
    {
        return TO;
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        return LS;
    }
    return 1000;
}

int Insert(pcbList *list, PCB *pcb)
{

    // find last pcb
    pcbList *tem = list;
    while (tem->next != NULL)
    {
        tem = tem->next;
    }

    pcbList *newNode = (pcbList *)malloc(sizeof(pcbList));
    newNode->pcb = pcb;
    newNode->next = NULL;

    tem->next = newNode;
    return 1;
}

int Insert_Rcb(rcbList *list, RCB *rcb)
{

    // find last pcb
    rcbList *tem = list;
    while (tem->next != NULL)
    {
        tem = tem->next;
    }

    rcbList *newNode = (rcbList *)malloc(sizeof(rcbList));
    newNode->rcb = rcb;
    newNode->next = NULL;

    tem->next = newNode;
    return 1;
}

void Insert_Priority(PCB *pcb)
{
    if (pcb->priority == 2)
    {
        Insert(System, pcb);
    }
    else if (pcb->priority == 1)
    {
        Insert(User, pcb);
    }
    else if (pcb->priority == 0)
    {
        Insert(Init, pcb);
    }
}

void Resource_Alloc(RCB *rcb, PCB *pcb)
{
    rcbNode *rNode = (rcbList *)malloc(sizeof(rcbNode));
    rNode->rcb = rcb;
    rNode->next = NULL;

    // add RCB to Pcb's resourceList's end
    rcbList *tem = pcb->resours;
    while (tem->next != NULL)
    {
        tem = tem->next;
    }
    tem->next = rNode;
}
/*
* Delete Resource From Pcb's rcblist
*/
void Resource_Free(RCB *rcb, PCB *pcb, int n)
{
    rcbNode *beforeRcbNode = pcb->resours;
    rcbNode *tem = beforeRcbNode->next;

    if (tem == NULL)
    {
        fprintf(stderr, "Resource Free Failed in Pcb %s for R%d of %d\n",
                pcb->pid, rcb->RID, n);
        exit(0);
    }

    while (tem != NULL && n > 0)
    {
        if (tem->rcb->RID == rcb->RID)
        {
            beforeRcbNode->next = tem->next;
            tem->next = NULL;
            //Todo Bug free delete resource Node
            
            tem = beforeRcbNode->next;
            n--;
        }else{
            beforeRcbNode = tem;
            tem = tem->next;
        }
    }

    return;
}

void Remove_Ready_Single(pcbList *list, PCB *pcb)
{
    pcbNode *before_node = list;
    pcbNode *temNode = before_node->next;
    PCB *temPcb = temNode->pcb;

    // Todo
    while (temNode != NULL && strcmp(temPcb->pid, pcb->pid) != 0)
    {
        before_node = temNode;
        temNode = temNode->next;
        temPcb = temNode->pcb;
    }

    // delete
    before_node->next = temNode->next;
    temNode->next = NULL;
}

void Remove_Ready(PCB *pcb)
{
    // remove from Ready List
    if (pcb->priority == 2)
    {
        Remove_Ready_Single(System, pcb);
    }
    else if (pcb->priority == 1)
    {
        Remove_Ready_Single(User, pcb);
    }
    else if (pcb->priority == 0)
    {
        Remove_Ready_Single(Init, pcb);
    }
}

PCB *Remove_Waiting(pcbList *waitList)
{
    if (waitList->next == NULL)
    {
        return NULL;
    }

    //Todo Bug
    PCB *first_wait_pcb = waitList->next->pcb;
    return first_wait_pcb;
}

PCB *Find_PCB()
{

    pcbNode *tem = System->next;
    if (tem != NULL)
    {
        return tem->pcb;
    }

    tem = User->next;
    if (tem != NULL)
    {
        return tem->pcb;
    }
    tem = Init->next;
    if (tem != NULL)
    {
        return tem->pcb;
    }
    return NULL;
}

/*
* ------------- Implementation Function -----------
* -------------------------------------------------
*/

// current process' parent and it's resource
int Create(char *name, char *pri)
{
    //1. Create PCB data Structure initialize PCB using parameters
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    strcpy(pcb->pid, name);
    pcb->priority = (int)((*pri) - '0');
    pcb->status = READY;

    // Not blocked by any Resource
    rcbNode *rcbBlockList = (rcbNode *)malloc(sizeof(rcbNode));
    rcbBlockList->next = NULL;
    pcb->blocked_list = rcbBlockList;
    pcb->blocked_num = 0;

    // resource Node init to 0 not allocated
    rcbNode *pcbResourceHeader = (rcbNode *)malloc(sizeof(rcbNode));
    pcbResourceHeader->next = NULL;
    pcb->resours = pcbResourceHeader;

    //2. link PCB to creation tree
    pcbList *childList = (pcbList *)malloc(sizeof(pcbList));
    pcb->child = childList;
    // Todo current process' parent
    pcb->parent = CURRENT_RUNNING_PCB;

    if (CURRENT_RUNNING_PCB != NULL)
    {
        Insert(CURRENT_RUNNING_PCB->child, pcb);
    }

    //3. insert(RL, PCB)//插入相应优先级队列的尾部
    Insert_Priority(pcb);
    //4. Scheduler()
    Scheduler();
}

/*
* Destroy Pcb by pName bug Not find in waiting List
*/

PCB *Find_Name_PCB(char *pName)
{
    pcbNode *tem = System->next;

    while (tem != NULL)
    {
        if (strcmp(tem->pcb->pid, pName) == 0)
        {
            return tem->pcb;
        }
        tem = tem->next;
    }

    tem = User->next;
    while (tem != NULL)
    {
        if (strcmp(tem->pcb->pid, pName) == 0)
        {
            return tem->pcb;
        }
        tem = tem->next;
    }

    tem = Init->next;
    while (tem != NULL)
    {
        if (strcmp(tem->pcb->pid, pName) == 0)
        {
            return tem->pcb;
        }
        tem = tem->next;
    }
    return NULL;
}

void Destroy(char *pName)
{
    PCB *toDestroy = Find_Name_PCB(pName);
    if (toDestroy == NULL)
    {
        fprintf(stderr, "Not find %s pcb\n", pName);
        return;
    }
    Kill_Tree(toDestroy);
    Scheduler();
}
/*
* list is the Header of List Not real
*/

void Delete_Node_List(pcbList* list,PCB* pcb){
    if(list->next==NULL){
        return;
    }

    pcbNode *beforeNode = list;
    pcbNode *tem = beforeNode->next;
    while (tem != NULL)
    {
        if (strcmp(tem->pcb->pid,pcb->pid) == 0)
        {
            beforeNode->next = tem->next;
            tem->next = NULL;
            free(tem);
            return;
        }
        beforeNode = tem;
        tem = tem->next;
    }
    return;
}

void Kill_Tree(PCB *pcb)
{
    pcbNode *childList = pcb->child;
    childList = childList->next;
    while (childList != NULL)
    {
        PCB *sonChild = childList->pcb;
        // Bug Todo
        Kill_Tree(sonChild);
        childList = childList->next;
    }

    // Find Occupied resource
    rcbNode *occResourceHeader = pcb->resours;
    rcbNode *occResource = occResourceHeader->next;
    while (occResource != NULL)
    {
        //Bug Todo
        occResource->rcb->status += 1;
        occResource = occResource->next;
    }
    occResourceHeader->next = NULL;

    // Delete form ReadyList and Waiting List
    if(pcb->priority==0){
        Delete_Node_List(Init, pcb);
    }    
    else if(pcb->priority==1){
        Delete_Node_List(User, pcb);
    }else  if(pcb->priority==2){
        Delete_Node_List(System, pcb);
    }
    // Bug Todo Block List
    //while(pcb->blocked_list->next!=NULL){}
}



void Request(RCB *rid, PCB *pid, int n)
{
    // Alloc
    if (((rid->status) - n) >= 0)
    {
        (rid->status) = (rid->status) - n;

        int t = n;
        while ((t--) > 0)
        {
            Resource_Alloc(rid, pid);
        }
    }
    // No resources Blocked
    else
    {
        // request too many
        if (n > rid->RID)
        {
            fprintf(stderr, "pid %s request %d for %d but ony have %d\n",
                    pid->pid, rid->RID, n, rid->RID);
            exit(0);
        }

        // set process Blocked and the blocked list
        pid->status = BLOCKED;

        // Bug
        Insert_Rcb(pid->blocked_list, rid);
        pid->blocked_num = n;

        // remove from Ready List
        Remove_Ready(pid);

        // add pid to Resource Waiting List Bug
        Insert(rid->waitListHeader, pid);

        Scheduler();
    }
}

void Release(RCB *rcb, PCB *pcb, int n)
{
    // remove resource from pcb
    Resource_Free(rcb, pcb, n);
    // Bug
    rcb->status += n;
    //  Some Pcb is waiting for current Resource
    pcbList *waitList = rcb->waitListHeader->next;
    while (waitList != NULL && rcb->status >= waitList->pcb->blocked_num)
    {
        rcb->status -= waitList->pcb->blocked_num;
        PCB *q = Remove_Waiting(rcb->waitListHeader);
        q->status = READY;
        // q->Status.List = RL;//就绪队列 Todo Bug
        int t = waitList->pcb->blocked_num;
        while ((t--) > 0)
        {
            Resource_Alloc(rcb, pcb);
        }

        //3. insert(RL, PCB)//插入相应优先级队列的尾部
        Insert_Priority(pcb);
        Scheduler();
        waitList = waitList->next;
    }
}

void Scheduler()
{

    // Bug Todo May get Null Return
    PCB *curPCB = Find_PCB();
    PCB *pcb = CURRENT_RUNNING_PCB;

    if (pcb == NULL || pcb->status != RUNNING)
    {
        preempt(curPCB);
    }
    else if (pcb->priority < curPCB->priority)
    {
        pcb->status = READY;
        preempt(curPCB);
    }
}

void preempt(PCB *curr)
{
    CURRENT_RUNNING_PCB = curr;

    printf("\nPID %s Running at Priority %d \n", curr->pid, curr->priority);
    printf("Occupied Resource :");
    rcbList *cur = curr->resours;
    cur = cur->next;

    while (cur != NULL)
    {
        printf("  R%d  left:%d", cur->rcb->RID, cur->rcb->status);
        cur = cur->next;
    }

    printf("\n");
}

void init_resource()
{
    // Resource Init Todo
    R1 = (RCB *)malloc(sizeof(RCB));
    R1->RID = 1;
    R1->status = 1;
    pcbNode *waitNode1 = (pcbNode *)malloc(sizeof(pcbNode));
    R1->waitListHeader = waitNode1;

    R2 = (RCB *)malloc(sizeof(RCB));
    R2->RID = 2;
    R2->status = 2;
    pcbNode *waitNode2 = (pcbNode *)malloc(sizeof(pcbNode));
    R2->waitListHeader = waitNode2;

    R3 = (RCB *)malloc(sizeof(RCB));
    R3->RID = 3;
    R3->status = 3;
    pcbNode *waitNode3 = (pcbNode *)malloc(sizeof(pcbNode));
    R3->waitListHeader = waitNode3;

    R4 = (RCB *)malloc(sizeof(RCB));
    R4->RID = 4;
    R4->status = 4;
    pcbNode *waitNode4 = (pcbNode *)malloc(sizeof(pcbNode));
    R4->waitListHeader = waitNode4;
}

void init_process()
{
    char *pName = "init";
    char pri = '0';
    Create(pName, &pri);
}

void Time_Out()
{
    PCB *curr = CURRENT_RUNNING_PCB;

    Remove_Ready(curr);
    curr->status = READY;
    Insert_Priority(curr);
    Scheduler();
}

int main()
{
    //0. Init
    init_resource();
    init_process();

    char *buffer = (char *)malloc(BUFFER_LEN * sizeof(char));
    char *command = (char *)malloc(4 * sizeof(char));

    char *pName = (char *)malloc(sizeof(char) * 16);
    char *rNum = (char *)malloc(sizeof(char));

    printf("tShell>");
    //1. gets command line
    while (NULL != fgets(buffer, BUFFER_LEN, stdin))
    {
        //2. gets command type
        printf("tShell>");

        char *tem = parser(command, buffer);
        //printf("Command is %s\ntype: %d\n", command, cmd_type(command));

        // 3. Deal with command
        int Type = cmd_type(command);

        // Create name priority
        if (Type == CR)
        {
            // get initialization parameters
            tem = parser(pName, tem);
            parser(rNum, tem);
            Create(pName, rNum);
        }
        // Request rid
        else if (Type == REQ)
        {
            tem = parser(pName, tem);
            //printf("res is %s\n", rName);
            parser(rNum, tem);
            int num = (int)((*rNum) - '0');

            if (strcmp(pName, "R1") == 0)
            {
                Request(R1, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R2") == 0)
            {
                Request(R2, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R3") == 0)
            {
                Request(R3, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R4") == 0)
            {
                Request(R4, CURRENT_RUNNING_PCB, num);
            }
        }
        else if (Type == REL)
        {
            tem = parser(pName, tem);
            parser(rNum, tem);
            int num = (int)((*rNum) - '0');

            if (strcmp(pName, "R1") == 0)
            {
                Release(R1, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R2") == 0)
            {
                Release(R2, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R3") == 0)
            {
                Release(R3, CURRENT_RUNNING_PCB, num);
            }
            if (strcmp(pName, "R4") == 0)
            {
                Release(R4, CURRENT_RUNNING_PCB, num);
            }
        }
        else if (Type == DE)
        {
            parser(pName, tem);
            Destroy(pName);
        }
        // Time Out
        else if (Type == TO)
        {
            Time_Out();
        }

        memset(buffer, 0, sizeof(char) * BUFFER_LEN);
        memset(command, 0, sizeof(char) * 4);
        memset(pName, 0, sizeof(char) * 16);
        memset(rNum, 0, sizeof(char));
    }
    free(buffer);
    free(command);
    free(pName);
    free(rNum);
}
