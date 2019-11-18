#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    struct RCB *blocked_list;
    int blocked_num;

    struct PCB *parent;
    struct pcbList *child;

    struct rcbList *resours;
};
typedef struct PCB PCB;

struct pcbNode
{

    PCB pcb;
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
    RCB rcb;
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
    newNode->pcb = *pcb;
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
    rNode->rcb = *rcb;
    rNode->next = NULL;

    // add RCB to Pcb's resourceList's end
    rcbNode *tem = pcb->resours;
    while (tem->next != NULL)
    {
        tem = tem->next;
    }
    tem->next = rNode;
}

void Resource_Free(RCB *rcb, PCB *pcb, int n)
{
    
}

void Remove_Ready_Single(pcbList *list, PCB *pcb)
{
    pcbNode *before_node = list;
    pcbNode *temNode = before_node->next;
    PCB temPcb = temNode->pcb;

    // Todo
    while (temNode != NULL && strcmp(temPcb.pid, pcb->pid) != 0)
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
    PCB *first_wait_pcb = &(waitList->next->pcb);
    return first_wait_pcb;
}

pcbNode *Find_PCB()
{

    pcbNode *tem = System->next;
    if (tem != NULL)
    {
        return tem;
    }
    tem = User->next;
    if (tem != NULL)
    {
        return tem;
    }
    tem = Init->next;
    if (tem != NULL)
    {
        return tem;
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
    pcb->blocked_list = NULL;
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

    //3. insert(RL, PCB)//插入相应优先级队列的尾部
    Insert_Priority(pcb);
    //4. Scheduler()

    Scheduler();
}

void Request(RCB *rid, PCB *pid, int n)
{
    // Alloc
    if ((rid->status - n) >= 0)
    {
        rid->status = rid->status - n;

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
            fprintf(stderr, "pid %s request %s for %d but ony have %d\n",
                    pid->pid, rid->RID, n, rid->RID);
            exit(0);
        }

        // set process Blocked and the blocked list
        pid->status = BLOCKED;

        // Todo Bug
        pid->blocked_list = rid;
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
    while (waitList != NULL && rcb->status >= waitList->pcb.blocked_num)
    {
        rcb->status -= waitList->pcb.blocked_num;
        PCB *q = Remove_Waiting(rcb->waitListHeader);
        q->status = READY;
        // q->Status.List = RL;//就绪队列 Todo Bug
        int t = waitList->pcb.blocked_num;
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
    pcbNode *curNode = Find_PCB();
    // Bug Todo
    PCB *curPCB = &(curNode->pcb);
    PCB *pcb = CURRENT_RUNNING_PCB;

    if (pcb->priority < curPCB->priority ||
        pcb->status != RUNNING ||
        pcb == NULL)
    {
        pcb->status = READY;
        preempt(curPCB);
    }
    else if (pcb->status != RUNNING || pcb == NULL)
    {
        preempt(curPCB);
    }
}

void preempt(PCB *curr)
{
    CURRENT_RUNNING_PCB = curr;

    printf("PID %s Running at Priority %d \n", curr->pid, curr->priority);
    printf("Occupied Resource :");
    rcbList *cur=curr->resours;
    cur = cur->next;

    if (cur == NULL)
    {
        printf("None\n");
    }
    else
    {
        while (cur != NULL)
        {
            printf("%d  Left: %d", cur->rcb.RID, cur->rcb.status);
            cur = cur->next;
        }
        printf("\n");
    }
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
    //1. gets command line
    while (NULL != fgets(buffer, BUFFER_LEN, stdin))
    {
        //2. gets command type
        char *command = (char *)malloc(4 * sizeof(char));
        char *tem = parser(command, buffer);
        printf("Command is %s\ntype: %d\n", command, cmd_type(command));

        // 3. Deal with command
        int Type = cmd_type(command);

        // Create name priority
        if (Type == CR)
        {
            //3.1 get initialization parameters
            char *pName = (char *)malloc(sizeof(char) * 16);
            tem = parser(pName, tem);

            char *pPriority = (char *)malloc(sizeof(char));
            parser(pPriority, tem);
            printf("commd is %s  Name is %s, Pri : %s\n", command, pName, pPriority);

            Create(pName, pPriority);
        }
        // Request rid
        else if (Type == REQ)
        {
            char *rName = (char *)malloc(sizeof(char) * 4);
            tem = parser(rName, tem);
            printf("res is %s\n", rName);
        }
    }
    free(buffer);
}