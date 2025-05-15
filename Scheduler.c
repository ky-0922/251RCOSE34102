#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX_SIZE 50001

char* get_line(char* buffer) {
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

    int is_io_processing;
    int io_time;
    //int io_range;
    int io_finish_time;
    int io_num;
    int io_process_time;
    int io_rate;

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

    np->is_io_processing = 0; // 0=no 1=yes(blocked)
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

    np->is_io_processing = 0;
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

// ProcessSet
typedef struct ProcessSet {
    Process* p[MAX_SIZE];
    int num;
} ProcessSet;

ProcessSet* processsets[10] = {NULL};

void destroy_processset(ProcessSet *ps) {
    for(int i=0;i<MAX_SIZE;++i) free(ps->p[i]);
    if(ps != NULL) free(ps);
}

ProcessSet* create_processset() {
    int ps_id;
    int process_n;
    int scanned=0;
    int process_info[8];

    printf("** Creating Process Set **\nSelect Mode:\nM: Manual R: Random\n");
    char input_buf[256];
    char mode = 'A';
    while (mode!='M' && mode!='R') {
        get_line(input_buf);
        input_buf[strcspn(input_buf, "\n")] = '\0';
        mode = input_buf[0];
        if(strlen(input_buf)!=1 && input_buf[0]!='M' && input_buf[0]!='R') {
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
                printf("Input %dth Process info: (Arrival Time, CPU burst time, Priority, I/O burst time, I/O occurance rate(%%))\n", i+1);
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
        for(int i=0;i<process_n;++i) {
            srand(time(NULL));
            int rv[5];
            rv[0]=rand()%30;
            rv[1]=rand()%50;
            rv[2]=rand()%32;
            rv[3]=rand()%5;
            rv[4]=rand()%10;
            ps->p[i]=create_process(i+1,rv[0],rv[1],rv[2],rv[3],rv[4]);
        }
    }
    
    printf("Process Set Created! Choose ProcessSet ID (0~9).\nIf there's already something in selected ID, it will be deleted.\n");
    scanned=0;
    while(!scanned) {
        get_line(input_buf);
        scanned = sscanf(input_buf, "%d", &ps_id);
        if(ps_id<0 || ps_id>9 || scanned!=1) {
            printf("Invalid input. ProcessSet ID should be value of 0~9.\nChoose ProcessSet ID (0~9).\n");
            scanned=0;
            continue;
        }
    }
    if(processsets[ps_id]!=NULL) {
        destroy_processset(processsets[ps_id]);
    }
    processsets[ps_id] = ps;
    printf("Process Set saved at #%d\n",ps_id);
}

void info_processset(ProcessSet* ps) {
    //TODO
}

// PriorityQueue
typedef struct PriorityQueue {
    Process** heap_array; // Process pointers
    int current_size;
    int mode;
} PriorityQueue;

PriorityQueue* pq_create(int mode) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->heap_array = (Process**)malloc(sizeof(Process*)*MAX_SIZE);
    pq->current_size = 0;
    pq->mode = mode;
    return pq;
}

void pq_destroy(PriorityQueue* pq) {
    if (pq!=NULL) {
        free(pq->heap_array);
        free(pq);
    }
}

static void swap_processes(Process** a, Process** b) {
    Process* temp = *a;
    *a = *b;
    *b = temp;
}

static int compare_processes_for_wq(Process* p1, Process* p2, int mode) { // return 1 = p1 has higher priority
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
}

static void heapify_up(PriorityQueue* pq, int index) {
    if (index == 0) return;
    int parent_index = (index - 1) / 2;

    if (compare_processes_for_wq(pq->heap_array[index], pq->heap_array[parent_index], pq->mode)) {
        swap_processes(&pq->heap_array[index], &pq->heap_array[parent_index]);
        heapify_up(pq, parent_index);
    }
}

void pq_insert(PriorityQueue* pq, Process* process) {
    if(pq->current_size > MAX_SIZE-1) return;
    pq->heap_array[pq->current_size] = process;
    pq->current_size++;
    heapify_up(pq, pq->current_size - 1);
}

static void heapify_down(PriorityQueue* pq, int index) {
    int left_child_idx = 2 * index + 1;
    int right_child_idx = 2 * index + 2;
    int preferred_idx = index;

    if (left_child_idx < pq->current_size &&
        compare_processes_for_wq(pq->heap_array[left_child_idx], pq->heap_array[preferred_idx], pq->mode)) {
        preferred_idx = left_child_idx;
    }

    if (right_child_idx < pq->current_size &&
        compare_processes_for_wq(pq->heap_array[right_child_idx], pq->heap_array[preferred_idx], pq->mode)) {
        preferred_idx = right_child_idx;
    }

    if (preferred_idx != index) {
        swap_processes(&pq->heap_array[index], &pq->heap_array[preferred_idx]);
        heapify_down(pq, preferred_idx);
    }
}

Process* pq_extract_min(PriorityQueue* pq) {
    if (pq == NULL || pq->current_size<1) {
        return NULL; // CPU must check next priority queue
    }
    Process* min_process = pq->heap_array[0];
    pq->heap_array[0] = pq->heap_array[pq->current_size - 1];
    pq->current_size--;

    if (pq->current_size > 0) {
        heapify_down(pq, 0);
    }
    return min_process;
}

Process* pq_peek_min(PriorityQueue* pq) {
    if (pq == NULL || pq->current_size<1) {
        return NULL;
    }
    return pq->heap_array[0];
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
    if(q->r+1==q->l || q->r==MAX_SIZE-1&&q->l==0) return;
    q->array[q->r] = p;
    q->r=q->r==MAX_SIZE-1?0:q->r+1;
}

Process* dequeue(Queue* q) {
    if(q->l==q->r) return NULL;
    Process* ret=q->array[q->l];
    q->l=q->l==MAX_SIZE-1?0:q->l+1;
    return ret;
}

void destroy_queue(Queue* q) {
    if(q!=NULL) free(q);
}

// log
void save_gantt_chart_to_file(const char* data_to_save) {
    time_t now;
    struct tm *ts;
    char time_str[80];
    char filename[256];

    time(&now);
    ts = localtime(&now);

    if (ts == NULL) {
        strcpy(filename, "gantt_chart.txt"); 
    } else {
        strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", ts);
        snprintf(filename, sizeof(filename), "gantt_chart_%s.txt", time_str);
    }

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Failed to open Gantt chart file for writing");
        return;
    }

    fprintf(fp, "%s", data_to_save); // 실제 간트 차트 데이터 쓰기
    // 예: generate_gantt_chart_string() 함수가 간트 차트 문자열을 반환한다고 가정
    
    fclose(fp);
    printf("Gantt chart successfully saved to: %s\n", filename);
}




int main() {
    printf("** ************************************************ **\n");
    printf("** *********** CPU Scheduling Simulator *********** **\n\n");
    printf("            Made by 2024320042 Kiyong Kim\n** ************************************************ **\n");

}