#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_SIZE 50001

char* get_line(char* buffer) { // 한줄받기 
    if (fgets(buffer, 256, stdin) == NULL) {
        return NULL;
    }
    char* newline_char_ptr = strchr(buffer, '\n');

    if (newline_char_ptr != NULL) {
        *newline_char_ptr = '\0';
    } else {
        if (!feof(stdin)) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
        }
    }

    return buffer;
}

//Process
typedef struct Process {
    int process_id;
    int arrival_time;
    int cpu_burst_time;
    int priority;

    //int is_io_processing;
    int io_time;
    //int io_range;
    int io_finish_time;
    int io_num;
    int io_process_time; // io 작업시간 합계
    int io_rate; // io 틱당 발생확률 

    int remaining_time;
    int finished_time;
} Process;
Process* create_process(int id, int arrival, int burst, int prio, int io, int io_rate) {
    Process* np = (Process*)malloc(sizeof(Process));
    if (np==NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }

    np->process_id = id;
    np->arrival_time = arrival;
    np->cpu_burst_time = burst; 
    np->priority = prio; // 0~31

    //np->is_io_processing = 0; // 0=no 1=yes(blocked)
    np->io_time = io;
    //np->io_range = io_range; // actual io time will be determined randomly by the range (io+-io_range)
    np->io_finish_time = -1;
    np->io_num = 0;
    np->io_process_time = 0; // all io process time
    np->io_rate = io_rate; // %, probability that io will occur in 1 tick

    np->remaining_time = burst;
    np->finished_time = -1;

    return np;
}
Process* copy_process(Process* orig) {
    Process* np = (Process*)malloc(sizeof(Process));
    if (np==NULL) {
        perror("Failed to allocate memory");
        return NULL;
    }

    np->process_id = orig->process_id;
    np->arrival_time = orig->arrival_time;
    np->cpu_burst_time = orig->cpu_burst_time; 
    np->priority = orig->priority;

    //np->is_io_processing = 0;
    np->io_time = orig->io_time;
    //np->io_range = orig->io_range;
    np->io_finish_time = -1;
    np->io_num = 0;
    np->io_process_time = 0;
    np->io_rate = orig->io_rate;

    np->remaining_time = orig->cpu_burst_time; 
    np->finished_time = -1;

    return np;
}
void delete_process(Process* p) {
    if(p!=NULL) free(p);
}
void info_process(Process* p) {
    printf("-----------------------\nProcess %d Info\n", p->process_id);
    printf("Process_ID: %d   Arrival_Time: %d   Priority: %d\n", p->process_id, p->arrival_time, p->priority);
    printf("CPU_Burst_Time: %d   I/O_Occurance_Rate_per_Tick: %d%%", p->cpu_burst_time, p->io_rate);
    printf("-----------------------\n");
}
void result_process(Process* p) {
    printf("-----------------------\nProcess %d Result\n", p->process_id);
    printf("Process_ID: %d   Arrival_Time: %d   Finished_Time: %d   Priority: %d\n", p->process_id, p->arrival_time, p->finished_time, p->priority);
    int ta=p->finished_time-p->arrival_time;
    printf("Waiting_Time: %d   CPU_Burst_Time: %d   Turnaround_Time: %d\n", ta-p->cpu_burst_time-p->io_process_time, p->cpu_burst_time, ta);
    printf("I/O_called: %d   Total_I/O_Burst_Time: %d\n", p->io_num, p->io_process_time);
    printf("-----------------------\n");
}
void result_processf(FILE* f,Process* p) {
    fprintf(f,"-----------------------\nProcess %d Result\n", p->process_id);
    fprintf(f,"Process_ID: %d   Arrival_Time: %d   Finished_Time: %d   Priority: %d\n", p->process_id, p->arrival_time, p->finished_time, p->priority);
    int ta=p->finished_time-p->arrival_time;
    fprintf(f,"Waiting_Time: %d   CPU_Burst_Time: %d   Turnaround_Time: %d\n", ta-p->cpu_burst_time-p->io_process_time, p->cpu_burst_time, ta);
    fprintf(f,"I/O_called: %d   Total_I/O_Burst_Time: %d\n", p->io_num, p->io_process_time);
    fprintf(f,"-----------------------\n");
}


// ProcessSet
typedef struct ProcessSet {
    Process* p[MAX_SIZE];
    int num;
} ProcessSet;

ProcessSet* processsets[10] = {NULL};
int processsets_data[10][10][10] = {0,}; // 알고리즘 실행결과 저장 [ps_num][알고리즘 코드(예: 1=fcfs)][저장할 데이터종류]
// 3번 인덱스가 0일때는 데이터 존재여부 확인, 0이면 없음, 1이면 유효한 데이터
// 1: waiting time 2: turnaround time

void destroy_processset(ProcessSet *ps) {
    if(ps == NULL) return;
    for(int i=0;i<ps->num;++i) free(ps->p[i]);
    if(ps != NULL) free(ps);
}
void create_processset(int ps_id) {
    int process_n;
    int scanned=0;
    int process_info[8];

    printf("** Creating Process Set **\nSelect Mode:\nM: Manual R: Random\n");
    char input_buf[256];
    char mode = 'A';
    while (mode!='M' && mode!='R') {
        get_line(input_buf);
        input_buf[strcspn(input_buf, "\n ")] = '\0';
        mode = input_buf[0];
        if(strlen(input_buf)!=1 || (input_buf[0]!='M' && input_buf[0]!='R')) {
            printf("Invalid input\n");
            mode = 'A';
            continue;
        }
    }

    ProcessSet* ps = (ProcessSet*)malloc(sizeof(ProcessSet));
    ps->num = 0;
    for (int i=0;i<MAX_SIZE;++i) {
        ps->p[i] = NULL;
    }

    printf("Input number of processes:");
    while(!scanned) {
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &process_n);
        if(process_n>MAX_SIZE-1 || process_n<1 || scanned!=1) {
            printf("Invalid input. Number of processes should be value of 1~50000.\n");
            scanned=0;
            continue;
        }
    }

    ps->num = process_n;
    if(mode == 'M') {
        for(int i=0;i<process_n;++i) {
            scanned=0;
            int temp[5];
            while(scanned!=5) {
                printf("Input Process #%d info: (Arrival Time, CPU burst time, Priority, I/O burst time, I/O occurance rate(%%))\n", i+1);
                get_line(input_buf);
                scanned = sscanf(input_buf, "%d%d%d%d%d", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4]);
                if(scanned!=5) {
                    printf("Invalid input\n"); continue;
                }
                if(temp[0]<0) {
                    printf("Arrival Time should be non-negative value.\n"); scanned=0; continue;
                }
                if(temp[1]<0) {
                    printf("CPU burst time should be non-negaive value.\n"); scanned=0; continue;
                }
                if(temp[2]<0 || temp[2]>31) {
                    printf("Priority should be value of 0~31.\n"); scanned=0; continue;
                }
                if(temp[3]<0) {
                    printf("I/O burst time should be non-negative value.\n"); scanned=0; continue;
                }
                if(temp[4]<0 || temp[4]>99) {
                    printf("I/O occurance rate should be value of 0~99(%%).\n"); scanned=0; continue;
                }
            }
            ps->p[i]=create_process(i+1,temp[0],temp[1],temp[2],temp[3],temp[4]);
        }
    }
    if(mode == 'R') {
        srand(time(NULL));
        for(int i=0;i<process_n;++i) {
            int rv[5];
            rv[0]=rand()%10;
            rv[1]=rand()%15;
            rv[2]=rand()%32;
            rv[3]=rand()%3;
            rv[4]=rand()%8;
            ps->p[i]=create_process(i+1,rv[0],rv[1],rv[2],rv[3],rv[4]);
        }
    }
    
    processsets[ps_id-1] = ps;
    printf("Process Set saved at #%d\n",ps_id);
}
void info_processset(ProcessSet* ps) {
    //TODO
}


// PriorityQueue
typedef struct PriorityQueue {
    Process** heap; // Process pointers
    int current_size;
    int mode;
} PriorityQueue;
PriorityQueue* pq_create(int mode) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->heap = (Process**)malloc(sizeof(Process*)*MAX_SIZE);
    pq->current_size = 0;
    pq->mode = mode;
    return pq;
}
void pq_destroy(PriorityQueue* pq) {
    if (pq!=NULL) {
        free(pq->heap);
        free(pq);
    }
}
static void swap_process(Process** a, Process** b) {
    Process* temp = *a;
    *a = *b;
    *b = temp;
}
static int compare_process(Process* p1, Process* p2, int mode) { // 1 = p1이 더 높은 우선순위
    if(mode==1) { // Waiting Queue for I/O processing processes
        if (p1->io_finish_time < p2->io_finish_time) {
            return 1;
        }
        if (p1->io_finish_time > p2->io_finish_time) {
            return 0; 
        }
        if (p1->arrival_time < p2->arrival_time) {
            return 1; // FCFS
        }
        if (p1->arrival_time > p2->arrival_time) {
            return 0;
        }
        if (p1->process_id < p2->process_id) {
            return 1; // Tiebreaker
        }
        return 0;
    }
    else if (mode==2) { // Priority Queue for SJF
        if (p1->remaining_time < p2->remaining_time) {
            return 1;
        }
        if (p1->remaining_time > p2->remaining_time) {
            return 0; 
        }
        if (p1->process_id < p2->process_id) {
            return 1; // Tiebreaker
        }
        return 0;
    }
    return 0;
}
static void heapify_up(PriorityQueue* pq, int i) {
    if (!i) return;
    int parent = (i-1)>>1;

    if (compare_process(pq->heap[i], pq->heap[parent], pq->mode)) {
        swap_process(&pq->heap[i], &pq->heap[parent]);
        heapify_up(pq, parent);
    }
}
void pq_insert(PriorityQueue* pq, Process* process) {
    if(pq->current_size > MAX_SIZE-1) return;
    pq->heap[pq->current_size] = process;
    pq->current_size++;
    heapify_up(pq, pq->current_size - 1);
}
static void heapify_down(PriorityQueue* pq, int i) {
    int left = (i<<1)+1;
    int right = (i<<1)+2;
    int p = i;

    if (left < pq->current_size &&
        compare_process(pq->heap[left], pq->heap[p], pq->mode)) {
        p = left;
    }
    if (right < pq->current_size &&
        compare_process(pq->heap[right], pq->heap[p], pq->mode)) {
        p = right;
    }
    if (p != i) {
        swap_process(&pq->heap[i], &pq->heap[p]);
        heapify_down(pq, p);
    }
}
Process* pq_extract_min(PriorityQueue* pq) {
    if (pq == NULL || pq->current_size<1) {
        return NULL; // null 받을경우 다음 우선순위큐 체크하도록 하기
    }
    Process* mn = pq->heap[0];
    pq->heap[0] = pq->heap[pq->current_size - 1];
    pq->current_size--;

    if (pq->current_size > 0) {
        heapify_down(pq, 0);
    }
    return mn;
}
Process* pq_peekmin(PriorityQueue* pq) {
    if (pq == NULL || pq->current_size<1) {
        return NULL;
    }
    return pq->heap[0];
}

// CircularList (Each Level of Priority Queue, FCFS)
typedef struct Queue {
    Process* array[MAX_SIZE];
    int l,r;
} Queue;
Queue* create_queue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->l=0; q->r=0;

    return q;
}
void enqueue(Queue* q, Process* p) {
    if((q->r+1) % MAX_SIZE == q->l) return;
    q->array[q->r] = p;
    q->r=q->r==MAX_SIZE-1?0:q->r+1;
}
Process* dequeue(Queue* q) {
    if(q->l==q->r) return NULL;
    Process* ret=q->array[q->l];
    q->l=q->l==MAX_SIZE-1?0:q->l+1;
    return ret;
}
Process* queue_front(Queue* q) {
    if(q->l == q->r) return NULL;
    return q->array[q->l];
}
void destroy_queue(Queue* q) {
    if(q!=NULL) free(q);
}

// log
FILE* create_file() {
    time_t now;
    struct tm *ts;
    char time_str[101];
    char filename[256];
    char input_buf[252];

    time(&now);
    ts = localtime(&now);

    printf("Gantt chart will be saved as txt file.\nInput file name: ");
    if (get_line(input_buf) != NULL) {
        if (input_buf[0]=='\0') {
            if (ts == NULL) {
                strcpy(filename, "gantt_chart.txt"); 
            }
            else {
                strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", ts);
                snprintf(filename, sizeof(filename), "gantt_chart_%s.txt", time_str);
            }
        }
        else {
            snprintf(filename, sizeof(filename), "%s.txt", input_buf);
        }
    }
        
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Failed to open file\n");
        return NULL;
    }
    printf("File will be saved as %s\n", filename);
    return fp;
}

// 알고리즘 실행 
int reset(int ps_n) { // 가장 늦게 arrive하는 프로세스의 arrival time 반환
    printf("Resetting...\n");
    ProcessSet* ps=processsets[ps_n];
    int processn=ps->num;
    int latest_arrival = 0;

    for(int i=0;i<processn;i++) {
        //ps->p[i]->is_io_processing=0;
        ps->p[i]->io_finish_time=-1;
        ps->p[i]->io_num=0;
        ps->p[i]->io_process_time=0;
        ps->p[i]->remaining_time=ps->p[i]->cpu_burst_time;
        ps->p[i]->finished_time=-1;
        latest_arrival=latest_arrival>ps->p[i]->arrival_time?latest_arrival:ps->p[i]->arrival_time;
    }
    return latest_arrival;
}

void write_s_gantt(FILE* fp,char (*s_gantt)[130], int t, int id, int* w) {
    if(*w<120) {
        if (id==0) {
            s_gantt[1][*w]='I';
            s_gantt[1][*w+1]='D';
            s_gantt[1][*w+2]='L';
            s_gantt[1][*w+3]='E';
            s_gantt[1][*w+4]='|';
            *w=*w+5;
        }
        else {   
            s_gantt[1][*w]='P';
            *w=*w+1;
            int size=1;
            while(size*10<=id) size*=10;
            while(size) {
                s_gantt[1][*w]=(id/size)+'0';
                id%=size; size/=10; *w=*w+1;
            }
            s_gantt[1][*w]='|';
            *w=*w+1;
        }
        int q=1;
        while(t) {
            if(*w-q>0) s_gantt[0][*w-q]=(t%10)+'0';
            q++; t/=10;
        }
    }
    return;
}

void fprint_result(FILE* fp, ProcessSet* ps, double wt, double tt) {
    int processn=ps->num;

    for(int i=0;i<processn;i++) {
        result_processf(fp, ps->p[i]);
    }
    fprintf(fp, "\nAverage Waiting Time: %.3f\n",wt);
    fprintf(fp, "\nAverage Turnaround Time: %.3f\n",tt);
}

void fcfs(int ps_n) {
    FILE* file = create_file();
    int tick=0;
    ProcessSet* ps=processsets[ps_n];
    int processn=ps->num;
    Queue* q=create_queue();
    Process* processing = NULL;
    PriorityQueue* wq=pq_create(1);
    char s_gantt[2][130] = {"0", "|"};
    for (int i=0;i<129;i++) s_gantt[0][i] = s_gantt[1][i] = ' ';
    s_gantt[0][129] = s_gantt[1][129] = '\0';
    int w=1;
    int latest = reset(ps_n);

    fprintf(file,"ProcessSet #%d\nAlgorithm: FCFS\n\nGantt Chart:\n",ps_n+1);

    printf("Processing...\n");
    tick=1;
    for(int i=0;i<processn;i++) {
        if(0==ps->p[i]->arrival_time) {
            enqueue(q,ps->p[i]);
            if(processing == NULL) processing=ps->p[i];
        }
    }
    processing->remaining_time--;

    while(tick<=latest || q->l!=q->r || wq->current_size>0) { // process
        while(wq->current_size>0 && pq_peekmin(wq)->io_finish_time <= tick) enqueue(q, pq_extract_min(wq)); // check waiting queue

        for(int i=0;i<processn;i++) { // enqueue first arrived process
            if(tick==ps->p[i]->arrival_time) enqueue(q,ps->p[i]);
        }

        if(processing == NULL) { // was not processing
            if(q->l!=q->r) { // queue not empty -> start new process
                processing=queue_front(q);
                processing->remaining_time--;
                write_s_gantt(file, s_gantt, tick, 0, &w);
            }
        }

        else { // was processing
            if(processing->remaining_time < 1) { // process end
                processing->finished_time = tick;
                write_s_gantt(file, s_gantt, tick, processing->process_id, &w);
                dequeue(q);

                if(q->l!=q->r) { // queue not empty -> start new process
                    processing=queue_front(q);
                    processing->remaining_time--;
                }
                else processing=NULL;
            }

            else { // still processing, I/O can be called
                if(rand()%100 < processing->io_rate) { // I/O interrupt called
                    processing->io_num++;
                    processing->io_process_time += processing->io_time;
                    processing->io_finish_time = tick+processing->io_time;
                    pq_insert(wq,processing);
                    write_s_gantt(file, s_gantt, tick, processing->process_id, &w);
                    dequeue(q);
                }
                else processing->remaining_time--;
            }
        }
        tick++;
    }
    printf("Simulation Completed!\nGantt Chart:\n%s\n%s\n",s_gantt[0],s_gantt[1]);
    double wt=0;
    double tt=0;

    for(int i=0;i<processn;i++) {
        int ta=(ps->p[i]->finished_time)-(ps->p[i]->arrival_time);
        wt+=ta-(ps->p[i]->cpu_burst_time)-(ps->p[i]->io_process_time);
        tt+=ta;
    }

    wt/=(double)processn;
    tt/=(double)processn;
    fprint_result(file, ps, wt, tt);
    printf("\nAverage Waiting Time: %.3f\n",wt);
    printf("Average Turnaround Time: %.3f\n\n",tt);
    fclose(file);
}


int manage_process_set() { // 프로세스셋 관리모드 
    int scanned = 0;
    int action = 0;
    int selected = 0;
    char input_buf[256];

    while(!scanned) {
        printf("\nProcessSet Manager\n1~10: Select ProcessSet number to manage 0: Back to main\n");
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &action);
        if(action>10 || action<0 || scanned!=1) {
            printf("Invalid input.\n\n");
            scanned=0;
            continue;
        }
    }

    if(!action) {
        printf("Going back to main...\n");
        return 0; // 0이면 종료 1이면 재시작하게 만들기
    }
    selected = action;

    scanned = 0;
    while(!scanned) {
        printf("ProcessSet %d selected:\nChoose action:\n1: ProcessSet info 2: Create new ProcessSet here 3: Back\n", selected);
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &action);
        if(action>3 || action<1 || scanned!=1) {
            printf("Invalid input.\n\n");
            scanned=0;
            continue;
        }
    }

    if(action == 1) {
        info_processset(processsets[selected-1]); // 실제 주소는 0~9
    }
    else if(action == 2) {
        create_processset(selected); // 내부에서 처리 
    }
    return 1;
}

int run_algo() { // 알고리즘 실행기 
    int scanned = 0;
    int action = 0;
    int selected = 0;
    char input_buf[256];

    while(!scanned) {
        printf("\nScheduling Algorithm runner\n1~10: Select ProcessSet number to run 0: Back to main\n");
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &action);
        if(action>10 || action<0 || scanned!=1) {
            printf("Invalid input.\n\n");
            scanned=0;
            continue;
        }
    }

    if(!action) {
        printf("Going back to main...\n");
        return 0; // 0이면 종료 1이면 재시작하게 만들기
    }
    if(processsets[action-1] == NULL) {
        printf("No ProcessSet in #%d, Create ProcessSet first or select other ProcessSet\n", action);
        return 1;
    }
    selected = action;

    scanned = 0;
    while(!scanned) {
        printf("ProcessSet %d selected:\nChoose algorithm:\n 1: FCFS 2: TODO\n", selected);
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &action);
        if(action>6 || action<1 || scanned!=1) {
            printf("Invalid input.\n\n");
            scanned=0;
            continue;
        }
    }

    if(action == 1) {
        fcfs(selected-1);
    }
    return 1;
}


int main() {
    printf("** ************************************************ **\n");
    printf("** *********** CPU Scheduling Simulator *********** **\n");
    printf("**                                                  **\n");
    printf("**          Made by 2024320042 Kiyong Kim           **\n");
    printf("** ************************************************ **\n\n");
    while(1) {
        int scanned = 0;
        int action = 0;
        char input_buf[256];

        while(!scanned) {
            printf("\nChoose action:\n1: Manage Process Set  2: Run Scheduling Algorithm  3: Exit\n");
            get_line(input_buf);
            scanned = sscanf(input_buf, "%d", &action);
            if(action>3 || action<1 || scanned!=1) {
                printf("Invalid input.\n\n");
                scanned=0;
                continue;
            }
        }

        if(action == 1) {
            int ret=1;
            while(ret==1) {
                ret = manage_process_set();
            }
        }
        else if(action == 2) {
            int ret=1;
            while(ret==1) {
                ret = run_algo();
            }
        }
        else {
            printf("Goodbye!\n");
            return 0;
        }
    }
}