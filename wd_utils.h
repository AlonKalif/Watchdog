#ifndef __WD_UTILS_H__
#define __WD_UTILS_H__

#define WD_EXE "wd.out"
#define WD_LOG "wd_log.txt"
#define MAX2(a, b) ((a) * ((a) >= (b)) + (b) * ((b) > (a)))
#define MIN_INTERVALS_PER_CHECK 2

#define UNUSED(x) ((void)(x))
#define MAX_STR 50

#define SHM_PERMISIONS 0644 | IPC_CREAT
#define IPC_SEM 0666

#define SHARED_SEM 0
#define TOP_PRIORITY 0
#define CHILD 0

#define SUCCESS 0
#define FAIL 1
#define ERROR -1

#define ALIVE 0
#define DEAD 1

#define FALSE 0
#define TRUE 1

#define OUT_OF_QUEUE 0
#define BACK_TO_QUEUE 1

#define MSG1 "error - from one of the following functions: InitSignalHandlers, InitSemaphores, atexit"
#define MSG2 "error - failed to create new process"
#define MSG3 "error - sprintf failed to set semaphore's name"
#define MSG4 "error - failed to create new thread"
#define MSG5 "error - inter-process communication failed"
#define MSG6 "Inter-process communication established"
#define MSG7 "error - failed to send stopping signal"
#define MSG8 "stopping sigal sent"
#define MSG9 "Watch-Dog stopped successfully"
#define MSG10 "Recived life sign from friend"
#define MSG11 "No life sign from friend. reviving friend"
#define MSG12 "Friend is alive"
#define MSG13 "Sending life sign to friend"
#define MSG14 "Joined worker thread"
#define MSG15 "error - failed to establish connection"

#endif /* __WD_UTILS_H__ */