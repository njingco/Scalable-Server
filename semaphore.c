#include "semaphore.h"

/* Initialize Semphore*/
int initsem(key_t key)
{
    int sid, status = 0;

    if ((sid = semget((key_t)key, 1, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        if (errno == EEXIST)
            sid = semget((key_t)key, 1, 0);
    }
    else /* if created */
        status = semctl(sid, 0, SETVAL, 0);
    if ((sid == -1) || status == -1)
    {
        perror("initsem failed\n");
        return (-1);
    }
    else
        return (sid);
}

/* acquire semophore */
void P(int sid)
{
    struct sembuf *sembuf_ptr;

    sembuf_ptr = (struct sembuf *)malloc(sizeof(struct sembuf *));
    sembuf_ptr->sem_num = 0;
    sembuf_ptr->sem_op = -1;
    sembuf_ptr->sem_flg = SEM_UNDO;

    if ((semop(sid, sembuf_ptr, 1)) == -1)
        printf("semop error\n");
}

/* release semaphore */
void V(int sid)
{
    struct sembuf *sembuf_ptr;

    sembuf_ptr = (struct sembuf *)malloc(sizeof(struct sembuf *));
    sembuf_ptr->sem_num = 0;
    sembuf_ptr->sem_op = 1;
    sembuf_ptr->sem_flg = SEM_UNDO;

    if ((semop(sid, sembuf_ptr, 1)) == -1)
        printf("semop error\n");
}
