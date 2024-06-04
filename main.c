#include <stdio.h> // printf(), scanf(), puts(), sprintf()
#include <stdlib.h> // rand(), srand()
#include <string.h> // strcat
#include <time.h> // time
//#include <stdbool.h> // bool, true, false

#define MAX_SIZE 5 // 생성 프로세스 사이즈(개수) 지정
#define TIME_QUANTUM 3 // RR 스케줄링에 사용되는 time quantum 지정
#define MAX_CPU_BURST_TIME 10 // CPU burst time의 최대값 지정
#define MAX_IO_BURST_TIME 5 // IO burst time의 최대값 지정
#define MAX_PRIORITY 4 // 프로세스 우선순위의 최대값 지정 (0~4) cf. 낮은 숫자가 우선순위 높음
#define MAX_ARRIVAL_TIME 10 // 프로세스 도착 시간의 최대값 지정

typedef enum {false, true} bool; // c언어에는 불리언 자료형이 없어서 열거형으로 선언 (stdbool 있음)

// 스케줄링 알고리즘을 열거형으로 정리
typedef enum {
    FCFS_scheduling = 0,
    SJF_scheduling = 1,
    STRF_scheduling = 2, // *** 변수명 수정 SRTF
    PreemtivePriority_scheduling = 3,
    NonPreempviePriority_scheduling = 4,
    RR_scheduling = 5
} scheduling_method;


typedef struct {
    int process_id;
    int cpu_burst_time;
    int io_burst_time;
    int arrival_time;
    int terminate_time; // 프로세스 수행이 끝난 시간
    int continued_time; // 연속으로 CPU에 할당되어 수행된 시간 -> RR에 사용
    int priority;
} Process;

// Priority Queue 구조체 정의
typedef struct priority_queue {
    Process heap[MAX_SIZE]; // 최대 크기 MAX_SIZE인 배열로 Priority Queue를 나타냄 => 나중에 Heap 자료구조로 사용됨. 힙 속성 유지하는 알고리즘을 통해 PQ 기능 구현
    int size; // Priority Queue에 현재 저장된 요소의 수 => 큐가 비어있으면 size = 0, 가득하면 size = MAX_SIZE
} priority_queue;

/* 힙(heap) : Priority Queue를 위해 고안된 완전이진트리 형태의 자료구조
-> 부모 노드가 가진 값은 자식 노드의 값보다 무조건 크거나(Max Heap) 작아야(Min Heap) 한다
이진 트리 : 한 노드가 최개 두개의 자식 노드 갖는 트리
완전 이진 트리 : 마지막 레벨을 제외한 모든 레벨에는 노드들이 가득차있고, 마지막 레벨의 노드들도 좌측부터 순서로 들어가있는 형태의 이진 트리

https://currygamedev.tistory.com/20

=> 힙 속성 유지하는 알고리즘을 통해 PQ 기능 구현 : 요소 삽입, 삭제 시 힙 속성 유지를 위한 연산 - Heapify Up, Heapify Down
*/

// Priority Queue 관련 함수 선언
void swap(Process *a, Process *b);
int pq_push(priority_queue *q, Process value, scheduling_method scheduling_method); 
Process pq_pop(priority_queue *q, scheduling_method scheduling_method); 
Process createRandomProcess(int uniqueId);

// 전역변수
int currentTime = 0; // 현재까지 수행된 시간 (턴마다 1씩 증가)
bool isCpuBusy = false; // CPU가 다른 프로세스에 의해 수행되고 있는지 파악 (사용 중이면 true)
bool isIOBusy = false; // I/O가 다른 프로세스에 의해 수행되고 있는지 파악 (사용 중이면 true)

Process initialProcessArr[MAX_SIZE]; // 초기 프로세스 배열로, simulator 시작 시의 프로세스 상태를 저장 (나중에 값 계산 위함)
Process runningCPUProcess; // 현재 CPU를 수행하고 있는 프로세스
Process runningIOProccess; // 현재 I/O를 수행하고 있는 프로세스

char timeLine[65536]; // 간트 차트의 시간 라인 저장 (2^16 충분한 공간 확보하면서 메모리 사용 최적화)
char topLine[65536]; // 간트 파트의 윗부분 저장
char bottomLine[65536]; // 간트 차트의 아랫부분 저장
char middleLineIO[65536]; // 간트 차트의 IO 부분 저장
char middleLineCPU[65536]; // 간트 차트의 CPU 부분 저장


// 메인 알고리즘 관련
void doScheduling(scheduling_method sch_method, priority_queue *jobQueue, priority_queue *readyQueue, priority_queue *waitingQueue, priority_queue *terminatedQueue);
void doIOOperation(Process selectedProcess, priority_queue *readyQueue, priority_queue *waitingQueue, scheduling_method sch_method);
void doCPUOperation(Process selectedProcess, priority_queue *waitingQueue, priority_queue *terminatedQueue);


// 간트 차트 관련
void drawTimeLine(void);
void drawTopBottomLine(void);
void drawMiddleLine(bool isCpu, bool isIdle);
void printGanttChart(void);

// 처음 프로세스 상태 출력 with table
void showInitialProcess(void);

// 알고리즘 성능 체크 출력 with table
void evaluateAlgorithm(priority_queue *terminatedQueue);

// 초기환경 세팅
void initailizeQueue(priority_queue *queue);
void setEnv(priority_queue *jobQueue, priority_queue *readyQueue, priority_queue *waitingQueue, priority_queue *terminatedQueue);

int main(int argc, const char * argv[]) {
    
    // 진정한 난수 생성
    srand((unsigned int)time(NULL));
    // Queue 생성
    priority_queue jobQueue = {.size = 0}; // 큐 초기화(큐 비어있도록)
    priority_queue readyQueue = {.size = 0};
    priority_queue waitingQueue = {.size = 0};
    priority_queue terminatedQueue = {.size = 0};
  
    //초기에 프로세스 생성
    for (int i = 0; i<MAX_SIZE; i++) {
        initialProcessArr[i] = createRandomProcess(i); //나중에 값 계산하기 위함
    }
    
    //프로세스 상태 프린트하는 함수
    showInitialProcess();
        
    // printf("%s",guide_letter);
    
    // int sch_method = 0 ;
    // scanf("%d", &sch_method);
    
    for(int sch_method = 0 ; sch_method < 6 ; sch_method++){
        switch (sch_method) {
            case FCFS_scheduling:
                printf("<FCFS_scheduling>\n");
                break;
            case SJF_scheduling:
                printf("<SJF_scheduling>\n");
                break;
            case STRF_scheduling:
                printf("<STRF_scheduling>\n");
                break;
            case PreemtivePriority_scheduling:
                printf("<PreemtivePriority_scheduling>\n");
                break;
            case NonPreempviePriority_scheduling:
                printf("<NonPreempviePriority_scheduling>\n");
                break;
            case RR_scheduling:
                printf("<RR_scheduling>\n");
                break;
            default:
                break;
        }
        
        //큐 비워주고, 타임 0으로, CPU / IO 상태 false 로 등등..
        setEnv(&jobQueue, &readyQueue, &waitingQueue, &terminatedQueue);
        //스케쥴링 시작
        doScheduling(sch_method, &jobQueue, &readyQueue, &waitingQueue, &terminatedQueue);
        //모두 마친 후 스케쥴링 알고리즘 성능 평가
        evaluateAlgorithm(&terminatedQueue);
        //간트 차트 그림
        printGanttChart();
    }
    return 0;
}

//////////////Process//////////////

// uniqueId(무작위로 생성된 고유값?)를 사용하여 랜덤한 Process 구조체를 생성하고 반환 (pid 외 다른 필드는 무작위 값으로 초기화)
Process createRandomProcess(int uniqueId) {
    int process_id = uniqueId; // 고유한 pid 설정
    int cpu_burst_time = rand()%(MAX_CPU_BURST_TIME)+1; // 무작위 CPU burst time 생성 (cpu burst time 이 1 이상이어야 하니까 1 더해줌. 1 ~ MAX_CPU_BURST_TIME) cf. rand() : 0 ~ RAND_MAX(32767) 사이 값 무작위 반환
    int io_burst_time = cpu_burst_time <= 1 ? 0 : rand()%(MAX_IO_BURST_TIME+1); // CPU시간이 1 이하인데 IO가 있을 수는 없으니까 - CPU burst time이 1 이하일 때 IO burst time을 0으로 설정, 그렇지 않으면 무작위 값1 ~ MAX_IO_BURST_TIME) 생성
    int arrival_time = rand()%(MAX_ARRIVAL_TIME+1); // 0 ~ MAX_ARRIVAL_TIME
    int priority = rand() % (MAX_PRIORITY+1); // 0 ~ MAX_PRIORITY
    Process process = {.process_id = process_id, .cpu_burst_time = cpu_burst_time, .io_burst_time = io_burst_time, .arrival_time = arrival_time, .priority = priority}; // Process 구조체 초기화 및 생성
    return process; // 생성된 프로세스 반환
}

//////////////Queue/////////////////

// 두 Process 구조체의 포인터를 받아서 그 값을 서로 교환 (힙 자료구조에서 요소들을 재배치할 때 유용하게 사용됨)
void swap(Process *a, Process *b) { 
    Process tmp = *a;
    *a = *b;
    *b = tmp;
}

/* <pq_push, pq_pop에서 재정렬 기준>
FCFS, RR -> arrival_time 
SJF, SRTF -> cpu_burst_time
NPP, PP -> priority 
*/

// PQ에 새로운 프로세스를 추가 (성공적으로 추가 시 1 반환, 큐가 가득차서 추가할 수 없으면 0 반환)
int pq_push(priority_queue* q, Process value, scheduling_method scheduling_method) {

    int size = q -> size;
    
    if (size + 1 > MAX_SIZE) { // 큐에 데이터가 가득 찼으면 return 시킴 (새로운 요소 추가 시 MAX_SIZE 초과일 때)
        return 0;
    }

    q -> heap[size] = value; // 마지막 빈 자리에 value 할당

    //재정렬
    int current = size; //현재 위치한 인덱스
    int parent = (size - 1) / 2; //완전 이진트리에서 parent 인덱스

    switch(scheduling_method){
        case FCFS_scheduling : case RR_scheduling :
            while (current > 0 && (q ->heap[current].arrival_time) < (q ->heap[parent].arrival_time)) { // current=0(큐의 최상위 레벨에 도달할 때까지 반복), 도착시간이 더 빠른 노드가 자식 노드가 되도록 
                swap(&(q->heap[current]), &(q ->heap[parent])); // 값 교환
                current = parent; 
                parent = (parent - 1) / 2;
            }
            break;
        case SJF_scheduling: case STRF_scheduling :
            while (current > 0 && (q ->heap[current].cpu_burst_time) < (q ->heap[parent].cpu_burst_time)) {
                swap(&(q->heap[current]), &(q ->heap[parent]));
                current = parent;
                parent = (parent - 1) / 2;
            }
            break;
        case NonPreempviePriority_scheduling: case PreemtivePriority_scheduling :
            while (current > 0 && (q ->heap[current].priority) < (q ->heap[parent].priority)) {
                swap(&(q->heap[current]), &(q ->heap[parent]));
                current = parent;
                parent = (parent - 1) / 2;
            }
            break;
        default:
            break;
    }

    q ->size++;
    return 1;
}

// PQ에서 가장 높은 우선순위의 프로세스를 제거하고 반환 (큐가 비어있으면 초기화된 process(pid=-1) 반환, 아니면 루트 요소 제거 후 루트 요소 반환)
Process pq_pop(priority_queue* q, scheduling_method scheduling_method) {
    
    // 큐가 비어있으면 return
    Process process = {.process_id = -1}; // process를 초기화하면서 process_id를 -1로 설정 => 큐가 비어 있을 때 반환할 기본값 설정 (-1은 유효한 pid가 아닐 가능성이 높아 반환된 값이 유효하지 않음을 나타낼 수 있음)
    if (q->size <= 0) return process; // 큐가 비어 있으면 초기화된 process를 반환 (process_id가 -1로 설정되어 있어 큐가 비어 있음을 알릴 수 있음)

    // 우선 순위 큐에서 pop 할 것은 가장 우선 순위가 높은 노드, 즉 루트 노드를 제거하고 반환할 프로세스 저장
    Process ret = q->heap[0];
    q->size--; 

    // 재정렬
    q->heap[0] = q->heap[q->size]; // 루트에 가장 낮은거 올림 (Heapify Down : 요소를 제거할 때는 힙의 루트 요소(가장 높은 우선순위)를 제거하고, 마지막 요소를 루트 위치로 이동시킨 후, 힙 속성을 유지하기 위해 아래로 이동시킴)

    int current = 0;
    int leftChild = current * 2 + 1;
    int rightChild = current * 2 + 2;
    int maxNode = current;

    switch(scheduling_method){
        case FCFS_scheduling : case RR_scheduling :
            // leftChild가 있는 동안에만 실행
            while (leftChild < q->size) {
                //left child 가 있는데 max node의 arrival time 이 더 큰 경우
                if ((q->heap[maxNode]).arrival_time > (q->heap[leftChild]).arrival_time) {
                    maxNode = leftChild;
                }
                //right child 까지 있는데 max node(방금전까지 leftChild의 값)의 arrival time 이 더 큰 경우
                if (rightChild < q->size && q->heap[maxNode].arrival_time > q->heap[rightChild].arrival_time) {
                    maxNode = rightChild;
                }

                if (maxNode == current) {
                    break;
                }
                else {
                    swap(&(q->heap[current]), &(q->heap[maxNode]));
                    current = maxNode;
                    leftChild = current * 2 + 1;
                    rightChild = current * 2 + 2;
                }
            }
            break;
        case SJF_scheduling: case STRF_scheduling :
            while (leftChild < q->size) {
                if ((q->heap[maxNode]).cpu_burst_time > (q->heap[leftChild]).cpu_burst_time) {
                    maxNode = leftChild;
                }
                if (rightChild < q->size && q->heap[maxNode].cpu_burst_time > q->heap[rightChild].cpu_burst_time) {
                    maxNode = rightChild;
                }
                
                if (maxNode == current) {
                    break;
                }
                else {
                    swap(&(q->heap[current]), &(q->heap[maxNode]));
                    current = maxNode;
                    leftChild = current * 2 + 1;
                    rightChild = current * 2 + 2;
                }
            }
            break;
        case NonPreempviePriority_scheduling: case PreemtivePriority_scheduling :
            while (leftChild < q->size) {
                if ((q->heap[maxNode]).priority > (q->heap[leftChild]).priority) {
                    maxNode = leftChild;
                }
                if (rightChild < q->size && q->heap[maxNode].priority > q->heap[rightChild].priority) {
                    maxNode = rightChild;
                }
                
                if (maxNode == current) {
                    break;
                }
                else {
                    swap(&(q->heap[current]), &(q->heap[maxNode]));
                    current = maxNode;
                    leftChild = current * 2 + 1;
                    rightChild = current * 2 + 2;
                }
            }
            break;
        default:
            break;
    }
    return ret;
}


////////////메인 알고리즘//////////////
/*
다음 턴에 CPU를 할당받을 프로세스를 결정하고,  I/O를 수행중이거나, 수행해야하는 프로세스의 동작을 돕는다. CPU나 I/O의 idle 상태를 판단하기도 한다.

*/
void doScheduling(scheduling_method sch_method, priority_queue *jobQueue, priority_queue *readyQueue, priority_queue *waitingQueue, priority_queue *terminatedQueue){
    // tie break 상황은 그냥 queue에서 맨 상위 노드에 있는걸로.
    while (terminatedQueue-> size != MAX_SIZE) { // terminatedQueue의 사이즈가 프로세스의 전체 개수만큼 도달하지 않았을 때 계속 반복
        drawTimeLine(); 
        drawTopBottomLine();
        
        // 생성된 프로세스 arrivalTime 체크해서 job 큐에서 ready 큐로 할당. 원래는 랜덤으로 프로세스 들어오겠지만 각 스케쥴링마다 같은 프로세스 사용해야하므로 jobQueue라는 저장공간에 있는 프로세스 꺼내다 씀
        while(true){
            if (jobQueue->size > 0) {
                // arrivalTime 순으로 할당 된거 꺼냄. FCFS 방식
                Process tempPop = pq_pop(jobQueue, FCFS_scheduling);
                if(tempPop.arrival_time == currentTime){
                    // 현재 시간에 arrive 됐으면 readyQueue에 할당.
                    pq_push(readyQueue, tempPop, sch_method);
                } else {
                    // 현재 시간 아니면 다시 집어넣고 while 종료
                    pq_push(jobQueue, tempPop, FCFS_scheduling);
                    break;
                }
            } else {
                break;
            }
        }

        // 만약 CPU가 Busy 상태면 스케줄링 방법에 따라 runningProcess를 결정한다.
        if (isCpuBusy) {
            /* 
            1) non preemptive(FCFS, SJF, NPP) 는 하던 작업 마저 수행 - cpu가 busy 상태면 전에 수행하던 프로세스가 전역변수 runningCPUProcess 에 할당되어 있을 것
            2) preemtive 는 ready queue에 다음 프로세스가 있을때 현재 프로세스보다 더 우선순위 높은지 체크하고 현재가 높으면 하던거 수행, 아니면 원래 하던거(running CPU process)는 레디 큐로 할당해주고 우선순위 높은거를 running cpu process로 바꿔야함
            3) SRTF은 현재 진행 프로세스의 남은 cpu burst time이 readyQueue에 있는 것보다 길다면, 원래 프로세스 레디큐에 집어넣고 runningCPUProcess를 남은 cpu burst time이 짧은 것으로 바꾼다
            4) RR은 만약에 레디큐에 다른 프로세스가 있는데, 현재 수행되고 있는 프로세스가 time quantum 이상 수행 됐다면, 원래 프로세스 레디큐에 집어넣고 runningCPUProcess를 레디 큐에서 꺼낸 프로세스로 바꾼다. 이때 프로세스를 레디큐에 집어넣을 때 해당 process의 continued_time(지속 시간)은 0으로 초기화 한 뒤 넣어야 한다.
            밑에 공통적으로 doCPUOperation() 있음. switch 문에서는 runningCPUProcess 뭘로 할거냐 결정해주는 것 
            */
            switch (sch_method) {
                case FCFS_scheduling : case SJF_scheduling : case NonPreempviePriority_scheduling :
                    break;
                case PreemtivePriority_scheduling :
                    if(readyQueue -> size > 0){
                        Process tempProcess = pq_pop(readyQueue, sch_method);
                        if(runningCPUProcess.priority > tempProcess.priority) {
                            // 현재 진행 프로세스 우선순위가 더 낮은 상황(priority 값이 낮은게 우선순위 높은 것)
                            pq_push(readyQueue, runningCPUProcess, sch_method); // 원래 프로세스 레디큐에 집어넣고
                            runningCPUProcess = tempProcess;  // runningCPUProcess를 꺼낸걸로 바꿔준 다음에 수행
                        } else {
                            // 현재 진행 프로세스 우선순위가 같거나 높은 상황
                            pq_push(readyQueue, tempProcess, sch_method); // 비교 위해 꺼낸 것 다시 넣어줌
                        }
                    }
                    break;
                case STRF_scheduling :
                    if(readyQueue -> size > 0){
                        Process tempProcess = pq_pop(readyQueue, sch_method);
                        if(runningCPUProcess.cpu_burst_time > tempProcess.cpu_burst_time) { // cpu burst time 적게 남은 프로세스를 runningCPUProcess로 지정.
                            pq_push(readyQueue, runningCPUProcess, sch_method);
                            runningCPUProcess = tempProcess;
                        } else {
                             pq_push(readyQueue, tempProcess, sch_method);
                        }
                    }
                    break;
                case RR_scheduling:
                    // 만약에 레디큐에 프로세스 없으면 원래 수행되던 애가 time quantum 관계 없이 계속 수행될 것.
                    // 레디큐에 프로세스들 있으면 현재 돌리고 있는 프로세스의 time quantum 체크
                    if(readyQueue -> size > 0){
                        Process tempProcess = pq_pop(readyQueue, sch_method);
                        // RR - 현재 진행되는 프로세스가 time quantum 이상으로 수행됐나 체크. 안넘었으면 이전 프로세스 계속 수행,
                        // 넘었으면 레디큐에 있던 다른 프로세스 수행
                        if(runningCPUProcess.continued_time >= TIME_QUANTUM){
                            runningCPUProcess.continued_time = 0; // RR time quantum 다다르면 원래 프로세스 지속 시간 초기화
                            // 이번 타임(currentTime)에 마저 cpu에 할당해서 수행시킬 것이 아니라, readyQueue에 넣어줘야함
                            // 이때 시간 다시 설정해서 레디큐에 들어가게 해야함. 안그러면 얘네들 초기값으로 들어가서 우선적으로 다시 수행될 것
                            runningCPUProcess.arrival_time = currentTime;
                            pq_push(readyQueue, runningCPUProcess, sch_method);
                            runningCPUProcess = tempProcess;
                        } else {
                            pq_push(readyQueue, tempProcess, sch_method);
                        }
                    }
                    break;
                default:
                    break;
            }// switch
        
            doCPUOperation(runningCPUProcess, waitingQueue, terminatedQueue); //위의 코드에 의해 결정된 runningCPUProcess 시행
            drawMiddleLine(true, false); //수행 될 process 그리기
        } else {
            //cpu not busy 상태면 readyQueue에서 꺼내서 cpu 할당해줘야함. 그 전에 readyQueue 비어있는 상태인지 체크. 비어있다면 cpu idle 상태.
            if(readyQueue->size > 0){
                Process selectedProcess = pq_pop(readyQueue, sch_method);
                runningCPUProcess = selectedProcess;
                doCPUOperation(runningCPUProcess, waitingQueue, terminatedQueue);
                drawMiddleLine(true, false);
            } else {
                //cpu idle
                drawMiddleLine(true, true);
            }
        }
        
        if (isIOBusy) {
            //io busy 하면 하던 작업 마저 수행
            doIOOperation(runningIOProccess, readyQueue, waitingQueue, sch_method);
            drawMiddleLine(false, false);
        } else {
            //io not busy 상태면 waitingQueue에서 꺼내서 io 할당해줘야함. 그 전에 waitingQueue 비어있는 상태인지 체크. 비어있다면 io idle 상태.
            if(waitingQueue->size > 0){
                //IO 작업 시작.  arrival time 대로 진행되어야하므로 fcfs 스케쥴링.
                Process selectedProcess = pq_pop(waitingQueue, FCFS_scheduling);
                int tempCurrent = currentTime+1;
            
                if(selectedProcess.arrival_time == tempCurrent) {
                    //만약 위의 cpu 작업에서 바로 들어온거라면 넘어감. 다시 waitingQueue에 그대로 넣어주고 스루
                    pq_push(waitingQueue, selectedProcess, FCFS_scheduling);
                    drawMiddleLine(false, true);
                } else {
                    runningIOProccess = selectedProcess;
                    drawMiddleLine(false, false);
                    doIOOperation(runningIOProccess, readyQueue, waitingQueue, sch_method);
                }
            } else {
                //io idle
                drawMiddleLine(false, true);
            }
        }
        
        //cpu랑 io 체크 작업 다 했으니까 time 올려주고 다음 작업 수행 반복
        currentTime++;
    } //while
    drawTimeLine(); //마지막 시간 한번 더 그려줘야함
} //doScheduling

void doIOOperation(Process selectedProcess, priority_queue *readyQueue, priority_queue *waitingQueue, scheduling_method sch_method){
    
    isIOBusy = true;
    
    // 들어왔다는건 일단 수행되었다는 거니까 io burst time 감소
    selectedProcess.io_burst_time--;
    
    /*(io burst time 끝날때 (isr 처리 위한 cpu burst time 무조건 남아있을 것) || 1/2 확률로 IO 수행 끝낼 때) && (io busrt time 남아있는데 cpu burst time 1 이하가 아닐때)
     만약 io burst time 남아있는데 cpu burst time 1 이하이면 무조건 다음 io 도 해당 프로세스로 되어야함. 왜냐하면 io 수행하고 isr 처리하기 위한 cpu burst time 남아있어야하기 때문
     */ 
    //if(selectedProcess.io_burst_time == 0){ -> 프로세스가 CPU연산과 IO연산을 동시에 수행하는 경우가 생김.
    if((selectedProcess.io_burst_time == 0 || rand() % 2 == 0) && !(selectedProcess.io_burst_time > 0 && selectedProcess.cpu_burst_time <= 1)){
        // IO operation -> readyQueue 에 넣기. 스케쥴링 따라서 넣어줘야 함.
        int tempCurrent = currentTime + 1;
        selectedProcess.arrival_time = tempCurrent;
        pq_push(readyQueue, selectedProcess, sch_method);
        isIOBusy = false;
    } else {
        // 다음에도 IO operation을 해당 프로세스가 수행
        runningIOProccess = selectedProcess;
    }
}

void doCPUOperation(Process selectedProcess, priority_queue *waitingQueue, priority_queue *terminatedQueue){
    isCpuBusy = true;
    
    // 들어왔다는건 일단 수행되었다는 거니까 cpu burst time 감소
    selectedProcess.cpu_burst_time--;
    selectedProcess.continued_time++; //지속 시간 증가 (for RR)
    
    // cpu burst time 끝나면(0이면) 종료
    if(selectedProcess.cpu_burst_time == 0) {
        int tempCurrent = currentTime + 1;
        // 끝난 프로세스의 처음 상태에 terminate time만 추가해서 종료 큐에 담아줌.
        // selectedProcess는 runningCPUProcess가 넘어와서 cpu_burst_time등이 변경되어있는 상태니까(?) 초기의 상태로 evaluation 해야 함
        initialProcessArr[selectedProcess.process_id].terminate_time = tempCurrent;
        pq_push(terminatedQueue, initialProcessArr[selectedProcess.process_id], FCFS_scheduling); // 종료된 프로세스를 FCFS방식으로 terminatedQueue에 추가
        isCpuBusy = false;
        return;
    }
    
    // IO 수행여부 결정
    // IO burst time이 남아있으면 1/2 확률로 IO 수행  ||  IO burst가 남아있는데 cpu burst 1 이하로 남아있으면 waiting Queue로 넘어가서 수행 끝내고 와야함
    if((rand() % 2 == 0 && selectedProcess.io_burst_time > 0) || (selectedProcess.io_burst_time > 0 && selectedProcess.cpu_burst_time <= 1)){
        // cpu 끝내겠다 -> IO 에 넣기.
        int tempCurrent = currentTime + 1;
        selectedProcess.arrival_time = tempCurrent; // 프로세스가 CPU 작업을 완료하고 IO 대기열에 추가될 때 해당 프로세스의 도착 시간 설정
        selectedProcess.continued_time = 0; //cpu 연속된 시간 0으로 다시 초기화
        pq_push(waitingQueue, selectedProcess, FCFS_scheduling); // 선택된 프로세스는 CPU에서 IO로 전환되었으므로 FCFS방식으로 waintingQueue에 추가
        isCpuBusy = false;
    }
    else {
        // IO operation 발생 안하는 경우. 다음 수행될 프로세스도 동일
        runningCPUProcess = selectedProcess;
    }
}

//////////////간트 차트 관련///////////////
void drawTimeLine(){
    char time[10];
    sprintf(time, "%d   ", currentTime%10); // 간트차트 예쁘게 그려지기 위해서 %10 해줘야함
    strcat(timeLine, time); // 추가
}

void drawTopBottomLine(){
    strcat(topLine, " -- ");
    strcat(bottomLine, " -- ");
}

void drawMiddleLine(bool isCpu, bool isIdle){
    char* selectedLine = isCpu ? middleLineCPU : middleLineIO;
    if (isIdle){
        strcat(selectedLine, "|  |");
    } else {
        int pid = isCpu ? runningCPUProcess.process_id : runningIOProccess.process_id;
        char str[10];
        sprintf(str, "|P%d|", pid);
        strcat(selectedLine, str);
    }
}

void printGanttChart(){
    puts("<CPU Gantt Chart>");
    puts(topLine);
    puts(middleLineCPU);
    puts(bottomLine);
    puts(timeLine);
    puts("");
    puts("<IO Gantt Chart>");
    puts(topLine);
    puts(middleLineIO);
    puts(bottomLine);
    puts(timeLine);
}

/////////////처음 프로세스 상태////////////////
void showInitialProcess()
{
    puts("<생성된 프로세스>");
    puts("+-----+--------------+----------+----------------+---------------+");
    puts("| PID | Arrival Time | Priority | CPU Burst Time | IO Burst Time |");
    puts("+-----+--------------+----------+----------------+---------------+");
    
    for(int i=0; i<MAX_SIZE; i++) {
        printf("| %2d  |      %2d      |    %2d    |       %2d       |       %2d      |\n"
               , initialProcessArr[i].process_id, initialProcessArr[i].arrival_time, initialProcessArr[i].priority, initialProcessArr[i].cpu_burst_time, initialProcessArr[i].io_burst_time);
        puts("+-----+--------------+----------+----------------+---------------+");
    }
    
}

////////////////알고리즘 성능 체크////////////
void evaluateAlgorithm(priority_queue *terminatedQueue){
    int totalWaitingTime = 0;
    int totalTurnaroundTime = 0;
    puts("+-----+--------------+-----------------+--------------+-----------------+----------------+---------------+----------+");
    puts("| PID | Waiting Time | Turnaround Time | Arrival Time | Terminated Time | CPU Burst Time | IO Burst Time | Priority |");
    puts("+-----+--------------+-----------------+--------------+-----------------+----------------+---------------+----------+");
    while (terminatedQueue -> size != 0) {
        Process process = pq_pop(terminatedQueue, FCFS_scheduling);
        int waitingTime = process.terminate_time - process.arrival_time - process.cpu_burst_time;
        int turnaroundTime = process.terminate_time - process.arrival_time;
        totalWaitingTime += waitingTime;
        totalTurnaroundTime += turnaroundTime;
        printf("| %2d  |      %2d      |        %2d       |      %2d      |        %2d       |       %2d       |       %2d      |    %2d    |\n"
               , process.process_id, waitingTime, turnaroundTime, process.arrival_time, process.terminate_time, process.cpu_burst_time, process.io_burst_time, process.priority);
        puts("+-----+--------------+-----------------+--------------+-----------------+----------------+---------------+----------+");
    }
    printf("Average Waiting time : %f\n", totalWaitingTime/(float)MAX_SIZE);
    printf("Average Turnaround time : %f\n\n", totalTurnaroundTime/(float)MAX_SIZE);
}


////////////////초기환경 세팅////////////////
void initailizeQueue(priority_queue *queue){
    while (queue -> size > 0) { // 큐에 있는 모든 요소 제거
        pq_pop(queue, FCFS_scheduling);
    }
}


void setEnv(priority_queue *jobQueue, priority_queue *readyQueue, priority_queue *waitingQueue, priority_queue *terminatedQueue){
    currentTime = 0; // 현재 시간을 0으로 초기화
    isCpuBusy = false; // CPU 사용 중이지 않은 상태
    isIOBusy = false; // IO 사용 중이지 않은 상태
    // 각 큐들 초기화 
    initailizeQueue(jobQueue); 
    initailizeQueue(readyQueue);
    initailizeQueue(waitingQueue);
    initailizeQueue(terminatedQueue);
    // 초기 프로세스들(프로세스 배열(initalProcessArr)의 요소)을 jobQueue에 FCFS 방식으로 삽입 
    for (int i = 0; i < MAX_SIZE; i++) {
        pq_push(jobQueue, initialProcessArr[i], FCFS_scheduling); // 나중에 time 체크하면서 빼낼때 arrival time 대로 빼내야 하니까 FCFS 스케쥴링대로
    }
    // 타임라인 그리기 위한 문자열 초기화
    timeLine[0] = '\0';
    topLine[0] = '\0';
    bottomLine[0] = '\0';
    middleLineIO[0] = '\0';
    middleLineCPU[0] = '\0';
}