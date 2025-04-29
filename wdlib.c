/*
	Developer: Alon Kalif
	File: 	   wd.c
	Reviewer:  Shimon
	Date: 	   12.10.2024
	Status:    Approved	
*/
#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>              /* For sprintf */
#include <stdlib.h>             /* For setenv, getenv */
#include <unistd.h>             /* For execvp, pipe, getpid, getppid */
#include <string.h>             /* For strlen, strcpy */
#include <pthread.h>            /* For multi-threading */
#include <semaphore.h>          /* For syncing threads */
#include <signal.h>             /* For for sig_atomic vars, kill, sigaction */
#include <time.h>               /* For time() */

#include <sys/ipc.h>            /* For ftok() */
#include <sys/shm.h>            /* For shmget() */

#include <fcntl.h>              /* For O_* constants */
#include <sys/stat.h>           /* For mode constants */
#include <assert.h>

#include "wd.h"                 /* For enum */
#include "wd_utils.h"           /* For defines */
#include "scheduler_v2.h"       /* For scheduler functions */

/* ================================= Globals ================================ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

char            my_sem_name[MAX_STR]        = {0};
char            friend_sem_name[MAX_STR]    = {0};
char*           shared_memory               = NULL;
int             g_connection_status         = SUCCESS;
int             g_friend_life_sign          = DEAD;
int             is_wd_process               = 0;
size_t          g_intervals_per_check       = 0;
time_t          g_interval_in_sec           = 0;
pid_t           friend_pid                  = 0;
sched_v2_t*     sched                       = NULL;
sem_t*          g_friend_is_alive_named_sem = NULL;
sem_t           g_good_to_go_unnamed_sem;
pthread_mutex_t g_lock;
pthread_t       worker_id;

/* ========================== Forward Declerations ========================== */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int  CheckOnFriend(void* argv);
static void ReciveLifeSign(int sig);
static void StopHandler(int sig);
static int  EstablishConnection(void* param);
static void DoNothing(void* param);
static int  SendLifeSign(void* param);


/* ============================ Helper Functions ============================ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int SetMySemName(void)
{ 
    if(0 > sprintf(my_sem_name, "/sem %d", getpid()))
    {
        return FAIL;
    }

    return SUCCESS;
}

static int SetFriendSemName(void)
{
    if(0 > sprintf(friend_sem_name, "/sem %d", friend_pid))
    {
        return FAIL;
    }

    return SUCCESS;
}

static int CreateFriendProcess(char* argv[])
{   
    friend_pid = fork();

    switch(friend_pid)
    {
        case CHILD:
            execvp("./" WD_EXE, argv);
            break;
        case ERROR:
            return FAIL;
            break;
    }

    return SUCCESS;
}

static int WriteToSharedMem(time_t interval_in_sec, size_t intervals_per_check)
{
    key_t key;
    int block_id = 0;

    key = ftok(WD_EXE, 0);
    if(ERROR == key)
    {
        return FAIL;
    }

    block_id = shmget(key, sizeof(time_t) + sizeof(size_t), SHM_PERMISIONS);
    if(ERROR == block_id)
    {
        return FAIL;
    }

    shared_memory = shmat(block_id, NULL, 0); 
    if(ERROR == (long int)shared_memory)
    {
        return FAIL;
    }

    memcpy(shared_memory, &interval_in_sec, sizeof(time_t));
    memcpy(shared_memory + sizeof(time_t), &intervals_per_check, sizeof(size_t));

    return SUCCESS;
}

static void WriteToLog(char* msg)
{
	char curr_time[MAX_STR] = {0};
    char* self = "(User)";
    FILE* log = NULL;
	time_t now = time(NULL);

    if(is_wd_process)
    {
        self = "(Watch Dog)";
    }

    pthread_mutex_lock(&g_lock);

        strftime(curr_time, MAX_STR, "%Y-%m-%d %H:%M:%S", localtime(&now));
        log = fopen(WD_LOG, "a");
        fprintf(log, "%s - pid %d %s\n%s\n\n", curr_time, getpid(), self, msg);	
        fclose(log);

    pthread_mutex_unlock(&g_lock);
}

static int IsFriendAlive(void)
{
    if(NULL != getenv("FRIEND_STATUS"))
    {
        return (0 == strcmp(getenv("FRIEND_STATUS"), "ALIVE"));
    }

    return FALSE;
}


/* =========================== Clean Up Functions =========================== */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void JoinThreadDestroyMtx(void)
{
    pthread_join(worker_id, NULL);

    WriteToLog(MSG14);
    printf("joined thread\n");
    pthread_mutex_destroy(&g_lock);
}

static int CleanUp(void)
{
    int status = SUCCESS;

    status |= shmdt(shared_memory);
    status |= sem_destroy(&g_good_to_go_unnamed_sem);
    status |= sem_close(g_friend_is_alive_named_sem);
    status |= sem_unlink(my_sem_name);

    if(SUCCESS != status)
    {
        return FAIL;
    }

    return SUCCESS;
}


/* ============================= Init Functions ============================= */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int InitMySem(void)
{
    g_friend_is_alive_named_sem = sem_open(my_sem_name, O_CREAT, IPC_SEM, 1);

    if(NULL == g_friend_is_alive_named_sem)
    {
        return FAIL;
    }

    return SUCCESS;
}

static void InitGlobalVars(time_t interval_in_sec, size_t intervals_per_check)
{
    g_interval_in_sec = interval_in_sec;
    g_intervals_per_check = intervals_per_check;

    g_connection_status = SUCCESS;

    if(0 == friend_pid)
    {
        is_wd_process = TRUE;
        friend_pid = getppid();
    }
}

static int InitSemaphores(void)
{
    int status = SUCCESS;

    status = sem_init(&g_good_to_go_unnamed_sem, SHARED_SEM, 0);

    status |= SetMySemName();

    status |= InitMySem();

    return status;
}

static int InitSignalHandlers(void)
{
    int status = SUCCESS;
    struct sigaction sa_struct = {0};

    sa_struct.sa_handler = ReciveLifeSign;
    status |= sigaction(SIGUSR1, &sa_struct, NULL);

    sa_struct.sa_handler = StopHandler;
    status |= sigaction(SIGUSR2, &sa_struct, NULL);

    return status;
}

static int InitScheduler(void* argv)
{
    ilrd_uid_t task_uid;

    sched = SchedV2Create();
    if(NULL == sched)
    {
        g_connection_status = ERROR;
        SchedV2Destroy(sched);
        return FAIL;
    }

    task_uid = SchedV2AddTask(sched, EstablishConnection, time(NULL), 1, NULL, DoNothing, NULL);
    if(TRUE == UIDIsSame(task_uid, UIDBadUID()))
    {
        g_connection_status = ERROR;
        SchedV2Destroy(sched);
        return FAIL;
    }

    task_uid = SchedV2AddTask(sched, SendLifeSign, time(NULL), g_interval_in_sec, NULL, DoNothing, NULL);
    if(TRUE == UIDIsSame(task_uid, UIDBadUID()))
    {
        g_connection_status = ERROR;
        SchedV2Destroy(sched);
        return FAIL;
    }

    task_uid = SchedV2AddTask(sched, CheckOnFriend, time(NULL) + g_interval_in_sec, g_interval_in_sec * g_intervals_per_check, argv, DoNothing, NULL);
    if(TRUE == UIDIsSame(task_uid, UIDBadUID()))
    {
        g_connection_status = ERROR;
        SchedV2Destroy(sched);
        return FAIL;
    }

    return SUCCESS;
}

static int Init(void)
{
    int status = SUCCESS;

    status = InitSignalHandlers();

    status |= InitSemaphores();
    
    status |= atexit(JoinThreadDestroyMtx);

    pthread_mutex_init(&g_lock, NULL);

    return status;
}


/* ============================= Scheduler Tasks ============================ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void DoNothing(void* param)
{
    UNUSED(param);
}

static int SendLifeSign(void* param)
{
    UNUSED(param);
    
    WriteToLog(MSG13);
    kill(friend_pid, SIGUSR1);

    return BACK_TO_QUEUE;
}

static int EstablishConnection(void* param)
{
    sem_t* friend_sem = NULL;

    UNUSED(param);

    friend_sem = sem_open(friend_sem_name, O_CREAT, IPC_SEM, 0);
    if(NULL == friend_sem)
    {
        g_connection_status = ERROR;
    }

    if(TRUE == is_wd_process)
    {   
        g_connection_status |= sem_post(friend_sem);
        g_connection_status |= sem_wait(g_friend_is_alive_named_sem);
    }
    else
    {
        g_connection_status |= sem_wait(g_friend_is_alive_named_sem);
        g_connection_status |= sem_post(friend_sem);
    }

    

    pthread_mutex_lock(&g_lock); 

        g_connection_status |= sem_post(&g_good_to_go_unnamed_sem);

    pthread_mutex_unlock(&g_lock);

    return OUT_OF_QUEUE;
}

static void ReviveUser(char* argv[])
{
    CleanUp();
    execvp(argv[0], argv);
}

static void ReviveWD(char* argv[])
{
    SchedV2AddTask(sched, EstablishConnection, TOP_PRIORITY, 1, NULL, DoNothing, NULL);
    InitMySem();
    CreateFriendProcess(argv);
    SetFriendSemName();
}

static void ReviveFriend(void* argv)
{
    kill(friend_pid, SIGKILL);

    if(is_wd_process)
    {
        ReviveUser((char**)argv);
    }
    else
    {
        ReviveWD((char**)argv);
    }
}

static int CheckOnFriend(void* argv)
{
    if(DEAD == g_friend_life_sign)
    {
        printf("friend is dead. (in %d)\n", getpid());

        WriteToLog(MSG11);

        setenv("FRIEND_STATUS", "DEAD", 1);

        ReviveFriend(argv);
    }
    else
    {
        WriteToLog(MSG12);

        pthread_mutex_lock(&g_lock); 

            g_friend_life_sign = DEAD;

        pthread_mutex_unlock(&g_lock);

    }

    return BACK_TO_QUEUE;
}


/* ============================= Signal Handlers ============================ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void ReciveLifeSign(int sig)
{
    UNUSED(sig);
    
    WriteToLog(MSG10);
    
    pthread_mutex_lock(&g_lock); 

        g_friend_life_sign = ALIVE;

    pthread_mutex_unlock(&g_lock);
}

static void StopHandler(int sig)
{
    UNUSED(sig);
    
    SchedV2Stop(sched);    
}


/* ========================= Worker Thread Function ========================= */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void* WatchDog(void* argv)
{    
    if(SUCCESS != InitScheduler(argv))
    {
        return NULL;
    }

    SchedV2Run(sched);

    SchedV2Destroy(sched);

    WriteToLog(MSG9);

    return NULL;
}


/* ============================== API Functions ============================= */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

wd_status_t WDStop(void)
{
    
    if(SUCCESS != kill(friend_pid, SIGUSR2))
    {
        WriteToLog(MSG7);
        return WD_TERMINATION_FAIL;
    }

    if(SUCCESS != pthread_kill(worker_id, SIGUSR2))
    {
        WriteToLog(MSG7);
        return WD_TERMINATION_FAIL;
    }
    
    WriteToLog(MSG8);
    return WD_SUCCESS;
}

wd_status_t WDStart(time_t interval_in_sec, size_t intervals_per_check, 
                    int argc, char* argv[])
{
    UNUSED(argc);
    
    intervals_per_check = MAX2(intervals_per_check, MIN_INTERVALS_PER_CHECK);

    if(SUCCESS != Init())
    {
        return WD_INIT_FAIL;
    }

    if(!IsFriendAlive())
    {
        if(SUCCESS != WriteToSharedMem(interval_in_sec, intervals_per_check))
        {
            CleanUp();
            WriteToLog(MSG1);
            return WD_INIT_FAIL;
        }

        if(SUCCESS != CreateFriendProcess(argv))
        {
            CleanUp();
            WriteToLog(MSG2);
            return WD_INIT_FAIL;
        }
    }

    InitGlobalVars(interval_in_sec, intervals_per_check);   

    if(SUCCESS != SetFriendSemName())
    {
        CleanUp();
        WriteToLog(MSG3);
        return WD_INIT_FAIL;
    }

    if(SUCCESS != pthread_create(&worker_id, NULL, WatchDog, argv))
    {
        CleanUp();
        WriteToLog(MSG4);
        return WD_INIT_FAIL;
    }

    if((SUCCESS != sem_wait(&g_good_to_go_unnamed_sem)))
    {
        CleanUp();
        WriteToLog(MSG5);
        return WD_INIT_FAIL;
    }

    pthread_mutex_lock(&g_lock); 

        if(SUCCESS != g_connection_status)
        {
            CleanUp();
            WriteToLog(MSG15);
            return WD_INIT_FAIL;
        }

    pthread_mutex_unlock(&g_lock);

    CleanUp();    
    WriteToLog(MSG6);

    return WD_SUCCESS;
}




