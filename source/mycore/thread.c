/*
 Copyright (C) 2015-2017 Alexander Borisov
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 
 Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "mycore/thread.h"

#ifndef MyCORE_BUILD_WITHOUT_THREADS

#if defined(IS_OS_WINDOWS)
/***********************************************************************************
 *
 * For Windows
 *
 ***********************************************************************************/
mystatus_t mycore_thread_create(mythread_t *mythread, mythread_list_t *thr, void *work_func)
{
    thr->pth = CreateThread(NULL,                   // default security attributes
                            0,                      // use default stack size
                            work_func,              // thread function name
                            &thr->data,             // argument to thread function
                            0,                      // use default creation flags
                            NULL);                  // returns the thread identifier

    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_join(mythread_t *mythread, mythread_list_t *thr)
{
    WaitForSingleObject(thr->pth, INFINITE);
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_cancel(mythread_t *mythread, mythread_list_t *thr)
{
    TerminateThread(thr->pth, 0);
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_init(mythread_t *mythread)
{
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_clean(mythread_t *mythread)
{
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_destroy(mythread_t *mythread)
{
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_create(mythread_t *mythread, mythread_context_t *ctx, size_t prefix_id)
{
    ctx->mutex = CreateSemaphore(NULL, 0, 1, NULL);
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_post(mythread_t *mythread, mythread_context_t *ctx)
{
    ReleaseSemaphore(ctx->mutex, 1, NULL);
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_wait(mythread_t *mythread, mythread_context_t *ctx)
{
    WaitForSingleObject(ctx->mutex, INFINITE);
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_try_wait(mythread_t *mythread, mythread_context_t *ctx)
{
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_close(mythread_t *mythread, mythread_context_t *ctx)
{
    CloseHandle(ctx->mutex);
    
    return MyCORE_STATUS_OK;
}

void mycore_thread_nanosleep(const struct timespec *tomeout)
{
    Sleep(0);
}

#else /* defined(IS_OS_WINDOWS) */
/***********************************************************************************
 *
 * For all unix system. POSIX pthread
 *
 ***********************************************************************************/

mystatus_t mycore_thread_create(mythread_t *mythread, mythread_list_t *thr, void *work_func)
{
    pthread_create(&thr->pth, mythread->attr,
                   work_func,
                   (void*)(&thr->data));
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_join(mythread_t *mythread, mythread_list_t *thr)
{
    pthread_join(thr->pth, NULL);
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_cancel(mythread_t *mythread, mythread_list_t *thr)
{
    pthread_cancel(thr->pth);
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_init(mythread_t *mythread)
{
    mythread->attr = (pthread_attr_t*)mycore_calloc(1, sizeof(pthread_attr_t));
    
    if(mythread->attr == NULL)
        return MyCORE_STATUS_THREAD_ERROR_ATTR_MALLOC;
    
    mythread->sys_last_error = pthread_attr_init(mythread->attr);
    if(mythread->sys_last_error)
        return MyCORE_STATUS_THREAD_ERROR_ATTR_INIT;
    
    mythread->sys_last_error = pthread_attr_setdetachstate(mythread->attr, PTHREAD_CREATE_JOINABLE);
    if(mythread->sys_last_error)
        return MyCORE_STATUS_THREAD_ERROR_ATTR_SET;
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_clean(mythread_t *mythread)
{
    mythread->attr = NULL;
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_thread_attr_destroy(mythread_t *mythread)
{
    if(mythread->attr) {
        mythread->sys_last_error = pthread_attr_destroy(mythread->attr);
        
        mycore_free(mythread->attr);
        mythread->attr = NULL;
        
        if(mythread->sys_last_error)
            return MyCORE_STATUS_THREAD_ERROR_ATTR_DESTROY;
    }
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_create(mythread_t *mythread, mythread_context_t *ctx, size_t prefix_id)
{
    ctx->mutex = (pthread_mutex_t*)mycore_calloc(1, sizeof(pthread_mutex_t));
    
    if(ctx->mutex == NULL)
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_MALLOC;
    
    if(pthread_mutex_init(ctx->mutex, NULL)) {
        mythread->sys_last_error = errno;
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_INIT;
    }
    
    if(pthread_mutex_lock(ctx->mutex)) {
        mythread->sys_last_error = errno;
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_LOCK;
    }
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_post(mythread_t *mythread, mythread_context_t *ctx)
{
    if(pthread_mutex_unlock(ctx->mutex)) {
        mythread->sys_last_error = errno;
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_UNLOCK;
    }
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_wait(mythread_t *mythread, mythread_context_t *ctx)
{
    if(pthread_mutex_lock(ctx->mutex)) {
        mythread->sys_last_error = errno;
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_LOCK;
    }
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_try_wait(mythread_t *mythread, mythread_context_t *ctx)
{
    if(pthread_mutex_trylock(ctx->mutex)) {
        mythread->sys_last_error = errno;
        return MyCORE_STATUS_THREAD_ERROR_MUTEX_LOCK;
    }
    
    return MyCORE_STATUS_OK;
}

mystatus_t mycore_hread_mutex_close(mythread_t *mythread, mythread_context_t *ctx)
{
    if(ctx->mutex) {
        pthread_mutex_destroy(ctx->mutex);
        mycore_free(ctx->mutex);
        
        ctx->mutex = NULL;
    }
    
    return MyCORE_STATUS_OK;
}

void mycore_thread_nanosleep(const struct timespec *tomeout)
{
    nanosleep(tomeout, NULL);
}

#endif /* !defined(IS_OS_WINDOWS) */
#endif /* MyCORE_BUILD_WITHOUT_THREADS */

/*
 *
 * MyTHREAD logic
 *
 */

mythread_t * mythread_create(void)
{
    return mycore_calloc(1, sizeof(mythread_t));
}

#ifdef MyCORE_BUILD_WITHOUT_THREADS

mystatus_t mythread_init(mythread_t *mythread, const char *sem_prefix, size_t thread_count)
{
    return MyCORE_STATUS_OK;
}

#else /* MyCORE_BUILD_WITHOUT_THREADS */

mystatus_t mythread_init(mythread_t *mythread, const char *sem_prefix, size_t thread_count)
{
    mythread->batch_count    = 0;
    mythread->batch_first_id = 0;
    mythread->stream_opt     = MyTHREAD_OPT_STOP;
    mythread->batch_opt      = MyTHREAD_OPT_STOP;
    
    if(thread_count)
    {
        mystatus_t status = mycore_thread_attr_init(mythread);
        if(status)
            return status;
        
        mythread->pth_list_root   = 1;
        mythread->pth_list_length = 1;
        mythread->pth_list_size   = thread_count + 1;
        mythread->pth_list        = (mythread_list_t*)mycore_calloc(mythread->pth_list_size, sizeof(mythread_list_t));
        
        if(mythread->pth_list == NULL)
            return MyCORE_STATUS_THREAD_ERROR_LIST_INIT;
    }
    else {
        mycore_thread_attr_clean(mythread);
        
        mythread->sys_last_error  = 0;
        mythread->pth_list_root   = 1;
        mythread->pth_list_length = 1;
        mythread->pth_list_size   = 0;
        mythread->pth_list        = NULL;
    }
    
    if(sem_prefix)
    {
        mythread->sem_prefix_length = strlen(sem_prefix);
        
        if(mythread->sem_prefix_length) {
            mythread->sem_prefix = mycore_calloc((mythread->sem_prefix_length + 1), sizeof(char));
            
            if(mythread->sem_prefix == NULL) {
                mythread->sem_prefix_length = 0;
                return MyCORE_STATUS_THREAD_ERROR_SEM_PREFIX_MALLOC;
            }
            
            mycore_string_raw_copy(mythread->sem_prefix, sem_prefix, mythread->sem_prefix_length);
        }
    }
    
    return MyCORE_STATUS_OK;
}

#endif /* MyCORE_BUILD_WITHOUT_THREADS */

void mythread_clean(mythread_t *mythread)
{
    mythread->sys_last_error = 0;
}

mythread_t * mythread_destroy(mythread_t *mythread, mythread_callback_before_join_f before_join, bool self_destroy)
{
    if(mythread == NULL)
        return NULL;
    
#ifndef MyCORE_BUILD_WITHOUT_THREADS
    
    mycore_thread_attr_destroy(mythread);
    
    if(mythread->pth_list) {
        mythread_resume_all(mythread);
        mythread_stream_quit_all(mythread);
        mythread_batch_quit_all(mythread);
        
        if(before_join)
            before_join(mythread);
        
        for (size_t i = mythread->pth_list_root; i < mythread->pth_list_length; i++)
        {
            mycore_thread_join(mythread, &mythread->pth_list[i]);
        }
        
        mycore_free(mythread->pth_list);
        mythread->pth_list = NULL;
    }
    
    if(mythread->sem_prefix) {
        mycore_free(mythread->sem_prefix);
        
        mythread->sem_prefix = NULL;
        mythread->sem_prefix_length = 0;
    }
    
#endif /* MyCORE_BUILD_WITHOUT_THREADS */
    
    if(self_destroy) {
        mycore_free(mythread);
        return NULL;
    }
    
    return mythread;
}

#ifndef MyCORE_BUILD_WITHOUT_THREADS

mythread_id_t _myhread_create_stream_raw(mythread_t *mythread, mythread_work_f work_func, void *process_func, mythread_thread_opt_t opt, mystatus_t *status, size_t total_count)
{
    mythread->sys_last_error = 0;
    
    if(status)
        *status = MyCORE_STATUS_OK;
    
    if(mythread->pth_list_length >= mythread->pth_list_size) {
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_NO_SLOTS;
        
        return 0;
    }
    
    mythread_list_t *thr = &mythread->pth_list[mythread->pth_list_length];
    
    thr->data.mythread = mythread;
    thr->data.func     = work_func;
    thr->data.id       = mythread->pth_list_length;
    thr->data.t_count  = total_count;
    thr->data.opt      = opt;
    thr->data.status   = 0;
    
    mystatus_t m_status = mycore_hread_mutex_create(mythread, &thr->data, 0);
    
    if(m_status != MyCORE_STATUS_OK && status) {
        *status = m_status;
        return 0;
    }
    
    m_status = mycore_thread_create(mythread, thr, process_func);
    if(m_status != MyCORE_STATUS_OK)
        return 0;
    
    mythread->pth_list_length++;
    return thr->data.id;
}

mythread_id_t myhread_create_stream(mythread_t *mythread, mythread_process_f process_func, mythread_work_f work_func, mythread_thread_opt_t opt, mystatus_t *status)
{
    return _myhread_create_stream_raw(mythread, work_func, process_func, opt, status, 0);
}

mythread_id_t myhread_create_batch(mythread_t *mythread, mythread_process_f process_func, mythread_work_f work_func, mythread_thread_opt_t opt, mystatus_t *status, size_t count)
{
    if(mythread->batch_count) {
        *status = MyCORE_STATUS_THREAD_ERROR_BATCH_INIT;
        return 0;
    }
    else if((mythread->pth_list_length + count) > mythread->pth_list_size) {
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_NO_SLOTS;
        
        return 0;
    }
    
    if(count == 0)
        count = 1;
    
    mythread->batch_first_id = 0;
    mythread->batch_count    = count;
    
    size_t start = mythread->pth_list_length;
    *status = MyCORE_STATUS_OK;
    
    bool init_first = false;
    
    for (size_t i = 0; i < count; i++)
    {
        mythread_id_t curr_id = _myhread_create_stream_raw(mythread, work_func, process_func, opt, status, i);
        
        if(init_first == false) {
            mythread->batch_first_id = curr_id;
            init_first = true;
        }
        
        if(*status)
        {
            for (size_t n = start; n < (start + i); n++)
            {
                mythread_list_t *thr = &mythread->pth_list[n];
                
                mycore_thread_cancel(mythread, thr);
                
                mycore_hread_mutex_post(mythread, &thr->data);
                mycore_hread_mutex_close(mythread, &thr->data);
            }
            
            mythread->batch_first_id = 0;
            mythread->batch_count    = 0;
            
            break;
        }
    }
    
    return mythread->batch_first_id;
}

#endif /* MyCORE_BUILD_WITHOUT_THREADS */

// mythread queue functions
#ifndef MyCORE_BUILD_WITHOUT_THREADS
mythread_queue_list_t * mythread_queue_list_create(mystatus_t *status)
{
    if(status)
        *status = MyCORE_STATUS_OK;
    
    mythread_queue_list_t* queue_list = (mythread_queue_list_t*)mycore_calloc(1, sizeof(mythread_queue_list_t));
    
    if(queue_list == NULL) {
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_MALLOC;
        return NULL;
    }
    
    return queue_list;
}

void mythread_queue_list_destroy(mythread_queue_list_t* queue_list)
{
    if(queue_list == NULL)
        return;
    
    mycore_free(queue_list);
}

size_t mythread_queue_list_get_count(mythread_queue_list_t* queue_list)
{
    return queue_list->count;
}

mythread_queue_list_entry_t * mythread_queue_list_entry_push(mythread_t *mythread, mythread_queue_t *queue, mystatus_t *status)
{
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    
    if(status)
        *status = MyCORE_STATUS_OK;
    
    mythread_queue_list_entry_t* entry = (mythread_queue_list_entry_t*)mycore_calloc(1, sizeof(mythread_queue_list_entry_t));
    
    if(entry == NULL) {
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_MALLOC;
        return NULL;
    }
    
    entry->thread_param = (mythread_queue_thread_param_t*)mycore_calloc(mythread->pth_list_size, sizeof(mythread_queue_thread_param_t));
    
    if(entry->thread_param == NULL) {
        mycore_free(entry);
        
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_MALLOC;
        return NULL;
    }
    
    size_t idx;
    for (idx = mythread->batch_first_id; idx < (mythread->batch_first_id + mythread->batch_count); idx++) {
        entry->thread_param[idx].use = mythread->pth_list[idx].data.t_count;
    }
    
    entry->queue = queue;
    
    if(mythread->stream_opt == MyTHREAD_OPT_UNDEF) {
        mythread_suspend_all(mythread);
    }
    else if(mythread->stream_opt == MyTHREAD_OPT_STOP) {
        mythread_stop_all(mythread);
    }
    
    if(queue_list->first) {
        queue_list->last->next = entry;
        entry->prev = queue_list->last;
        
        queue_list->last = entry;
    }
    else {
        queue_list->first = entry;
        queue_list->last = entry;
    }
    
    queue_list->count++;
    
    if(mythread->stream_opt != MyTHREAD_OPT_STOP)
        mythread_resume_all(mythread);
    
    return entry;
}

mythread_queue_list_entry_t * mythread_queue_list_entry_delete(mythread_t *mythread, mythread_queue_list_entry_t *entry, bool destroy_queue)
{
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    
    mythread_queue_list_entry_t *next = entry->next;
    mythread_queue_list_entry_t *prev = entry->prev;
    
    if(mythread->stream_opt == MyTHREAD_OPT_UNDEF) {
        mythread_suspend_all(mythread);
    }
    else if(mythread->stream_opt == MyTHREAD_OPT_STOP) {
        mythread_stop_all(mythread);
    }
    
    if(prev)
        prev->next = next;
    
    if(next)
        next->prev = prev;
    
    if(queue_list->first == entry)
        queue_list->first = next;
    
    if(queue_list->last == entry)
        queue_list->last = prev;
    
    if(mythread->stream_opt != MyTHREAD_OPT_STOP)
        mythread_resume_all(mythread);
    
    if(destroy_queue && entry->queue)
        mythread_queue_destroy(entry->queue);
    
    if(entry->thread_param)
        mycore_free(entry->thread_param);
    
    mycore_free(entry);
    
    queue_list->count--;
    
    return NULL;
}

void mythread_queue_list_entry_clean(mythread_t *mythread, mythread_queue_list_entry_t *entry)
{
    if(entry == NULL)
        return;
    
    mythread_queue_clean(entry->queue);
    
    size_t idx;
    for (idx = mythread->pth_list_root; idx < mythread->batch_first_id; idx++) {
        entry->thread_param[idx].use = 0;
    }
    
    for (idx = mythread->batch_first_id; idx < (mythread->batch_first_id + mythread->batch_count); idx++) {
        entry->thread_param[idx].use = mythread->pth_list[idx].data.t_count;
    }
}

void mythread_queue_list_entry_wait_for_done(mythread_t *mythread, mythread_queue_list_entry_t *entry)
{
    if(entry == NULL)
        return;
    
    size_t idx;
    const struct timespec tomeout = {0, 0};
    
    for (idx = mythread->pth_list_root; idx < mythread->pth_list_size; idx++) {
        mythread_queue_thread_param_t *thread_param = &entry->thread_param[ idx ];
        while(thread_param->use < entry->queue->nodes_uses) {
            mycore_thread_nanosleep(&tomeout);
        }
    }
}

#endif /* MyCORE_BUILD_WITHOUT_THREADS */

mythread_queue_t * mythread_queue_create(size_t size, mystatus_t *status)
{
    if(status)
        *status = MyCORE_STATUS_OK;
    
    if(size < 4096)
        size = 4096;
    
    mythread_queue_t* queue = (mythread_queue_t*)mycore_malloc(sizeof(mythread_queue_t));
    
    if(queue == NULL) {
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_MALLOC;
        return NULL;
    }
    
    queue->nodes_pos_size = 512;
    queue->nodes_size     = size;
    queue->nodes          = (mythread_queue_node_t**)mycore_calloc(queue->nodes_pos_size, sizeof(mythread_queue_node_t*));
    
    if(queue->nodes == NULL) {
        mycore_free(queue);
        
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODES_MALLOC;
        return NULL;
    }
    
    mythread_queue_clean(queue);
    
    queue->nodes[queue->nodes_pos] = (mythread_queue_node_t*)mycore_malloc(sizeof(mythread_queue_node_t) * queue->nodes_size);
    
    if(queue->nodes[queue->nodes_pos] == NULL) {
        mycore_free(queue->nodes);
        mycore_free(queue);
        
        if(status)
            *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODE_MALLOC;
        return NULL;
    }
    
    return queue;
}

void mythread_queue_clean(mythread_queue_t* queue)
{
    queue->nodes_length = 0;
    queue->nodes_pos    = 0;
    queue->nodes_root   = 0;
    queue->nodes_uses   = 0;
    
    if(queue->nodes[queue->nodes_pos])
        mythread_queue_node_clean(&queue->nodes[queue->nodes_pos][queue->nodes_length]);
}

mythread_queue_t * mythread_queue_destroy(mythread_queue_t* queue)
{
    if(queue == NULL)
        return NULL;
    
    if(queue->nodes) {
        for (size_t i = 0; i <= queue->nodes_pos; i++) {
            mycore_free(queue->nodes[i]);
        }
        
        mycore_free(queue->nodes);
    }
    
    mycore_free(queue);
    
    return NULL;
}

void mythread_queue_node_clean(mythread_queue_node_t* qnode)
{
    memset(qnode, 0, sizeof(mythread_queue_node_t));
}

mythread_queue_node_t * mythread_queue_get_prev_node(mythread_queue_node_t* qnode)
{
    return qnode->prev;
}

mythread_queue_node_t * mythread_queue_get_current_node(mythread_queue_t* queue)
{
    return &queue->nodes[queue->nodes_pos][queue->nodes_length];
}

mythread_queue_node_t * mythread_queue_get_first_node(mythread_queue_t* queue)
{
    return &queue->nodes[0][0];
}

size_t mythread_queue_count_used_node(mythread_queue_t* queue)
{
    return queue->nodes_uses;
}

mythread_queue_node_t * mythread_queue_node_malloc(mythread_t *mythread, mythread_queue_t* queue, mystatus_t *status)
{
    queue->nodes_length++;
    
    if(queue->nodes_length >= queue->nodes_size)
    {
        queue->nodes_pos++;
        
        if(queue->nodes_pos >= queue->nodes_pos_size)
        {
            mythread_queue_wait_all_for_done(mythread);
            
            queue->nodes_pos_size <<= 1;
            mythread_queue_node_t** tmp = mycore_realloc(queue->nodes, sizeof(mythread_queue_node_t*) * queue->nodes_pos_size);
            
            if(tmp) {
                memset(&tmp[queue->nodes_pos], 0, sizeof(mythread_queue_node_t*) * (queue->nodes_pos_size - queue->nodes_pos));
                
                queue->nodes = tmp;
            }
            else {
                if(status)
                    *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODES_MALLOC;
                
                return NULL;
            }
        }
        
        if(queue->nodes[queue->nodes_pos] == NULL) {
            queue->nodes[queue->nodes_pos] = (mythread_queue_node_t*)mycore_malloc(sizeof(mythread_queue_node_t) * queue->nodes_size);
            
            if(queue->nodes[queue->nodes_pos] == NULL) {
                if(status)
                    *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODE_MALLOC;
                
                return NULL;
            }
        }
        
        queue->nodes_length = 0;
    }
    
    queue->nodes_uses++;
    
    return &queue->nodes[queue->nodes_pos][queue->nodes_length];
}

mythread_queue_node_t * mythread_queue_node_malloc_limit(mythread_t *mythread, mythread_queue_t* queue, size_t limit, mystatus_t *status)
{
    queue->nodes_length++;
    
    if(queue->nodes_uses >= limit) {
        queue->nodes_uses++;
        mythread_queue_wait_all_for_done(mythread);
        
        queue->nodes_length = 0;
        queue->nodes_pos    = 0;
        queue->nodes_root   = 0;
        queue->nodes_uses   = 0;
    }
    else if(queue->nodes_length >= queue->nodes_size)
    {
        queue->nodes_pos++;
        
        if(queue->nodes_pos >= queue->nodes_pos_size)
        {
            mythread_queue_wait_all_for_done(mythread);
            
            queue->nodes_pos_size <<= 1;
            mythread_queue_node_t** tmp = mycore_realloc(queue->nodes, sizeof(mythread_queue_node_t*) * queue->nodes_pos_size);
            
            if(tmp) {
                memset(&tmp[queue->nodes_pos], 0, sizeof(mythread_queue_node_t*) * (queue->nodes_pos_size - queue->nodes_pos));
                
                queue->nodes = tmp;
            }
            else {
                if(status)
                    *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODES_MALLOC;
                
                return NULL;
            }
        }
        
        if(queue->nodes[queue->nodes_pos] == NULL) {
            queue->nodes[queue->nodes_pos] = (mythread_queue_node_t*)mycore_malloc(sizeof(mythread_queue_node_t) * queue->nodes_size);
            
            if(queue->nodes[queue->nodes_pos] == NULL) {
                if(status)
                    *status = MyCORE_STATUS_THREAD_ERROR_QUEUE_NODE_MALLOC;
                
                return NULL;
            }
        }
        
        queue->nodes_length = 0;
    }
    
    queue->nodes_uses++;
    
    return &queue->nodes[queue->nodes_pos][queue->nodes_length];
}

#ifndef MyCORE_BUILD_WITHOUT_THREADS

mythread_queue_node_t * mythread_queue_node_malloc_round(mythread_t *mythread, mythread_queue_list_entry_t *entry, mystatus_t *status)
{
    mythread_queue_t* queue = entry->queue;
    
    queue->nodes_length++;
    
    if(queue->nodes_length >= queue->nodes_size) {
        queue->nodes_uses++;
        
        mythread_queue_list_entry_wait_for_done(mythread, entry);
        mythread_queue_list_entry_clean(mythread, entry);
    }
    else
        queue->nodes_uses++;
    
    return &queue->nodes[queue->nodes_pos][queue->nodes_length];
}

void mythread_queue_wait_all_for_done(mythread_t *mythread)
{
    const struct timespec tomeout = {0, 0};
    
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    mythread_queue_list_entry_t *entry = queue_list->first;
    
    while(entry)
    {
        for (size_t idx = mythread->pth_list_root; idx < mythread->pth_list_size; idx++) {
            while(entry->thread_param[idx].use < entry->queue->nodes_uses) {
                mycore_thread_nanosleep(&tomeout);
            }
        }
        
        entry = entry->next;
    }
}

#else

void mythread_queue_wait_all_for_done(mythread_t *mythread) {}

#endif /* MyCORE_BUILD_WITHOUT_THREADS */

#ifdef MyCORE_BUILD_WITHOUT_THREADS

void mythread_stream_quit_all(mythread_t *mythread) {}
void mythread_batch_quit_all(mythread_t *mythread) {}
void mythread_stream_stop_all(mythread_t *mythread) {}
void mythread_batch_stop_all(mythread_t *mythread) {}
void mythread_stop_all(mythread_t *mythread) {}
void mythread_resume_all(mythread_t *mythread) {}
void mythread_suspend_all(mythread_t *mythread) {}

#else /* MyCORE_BUILD_WITHOUT_THREADS */

// mythread functions
void mythread_stream_quit_all(mythread_t *mythread)
{
    mythread->stream_opt = MyTHREAD_OPT_QUIT;
}

void mythread_batch_quit_all(mythread_t *mythread)
{
    mythread->batch_opt = MyTHREAD_OPT_QUIT;
}

void mythread_stream_stop_all(mythread_t *mythread)
{
    if(mythread->stream_opt != MyTHREAD_OPT_STOP)
        mythread->stream_opt = MyTHREAD_OPT_STOP;
    
    size_t idx;
    const struct timespec tomeout = {0, 0};
    
    for (idx = mythread->pth_list_root; idx < mythread->batch_first_id; idx++) {
        while(mythread->pth_list[idx].data.opt != MyTHREAD_OPT_STOP) {
            mycore_thread_nanosleep(&tomeout);
        }
    }
}

void mythread_batch_stop_all(mythread_t *mythread)
{
    if(mythread->batch_opt != MyTHREAD_OPT_STOP)
        mythread->batch_opt = MyTHREAD_OPT_STOP;
    
    size_t idx;
    const struct timespec tomeout = {0, 0};
    
    for (idx = mythread->batch_first_id; idx < (mythread->batch_first_id + mythread->batch_count); idx++) {
        while(mythread->pth_list[idx].data.opt != MyTHREAD_OPT_STOP) {
            mycore_thread_nanosleep(&tomeout);
        }
    }
}

void mythread_stop_all(mythread_t *mythread)
{
    mythread_stream_stop_all(mythread);
    mythread_batch_stop_all(mythread);
}

void mythread_resume_all(mythread_t *mythread)
{
    if(mythread->stream_opt == MyTHREAD_OPT_UNDEF &&
       mythread->batch_opt  == MyTHREAD_OPT_UNDEF)
        return;
    
    if(mythread->stream_opt == MyTHREAD_OPT_WAIT ||
       mythread->batch_opt == MyTHREAD_OPT_WAIT)
    {
        mythread->stream_opt = MyTHREAD_OPT_UNDEF;
        mythread->batch_opt  = MyTHREAD_OPT_UNDEF;
    }
    else {
        mythread->stream_opt = MyTHREAD_OPT_UNDEF;
        mythread->batch_opt  = MyTHREAD_OPT_UNDEF;
        
        for (size_t idx = mythread->pth_list_root; idx < mythread->pth_list_size; idx++) {
            mycore_hread_mutex_post(mythread, &mythread->pth_list[idx].data);
        }
    }
}

void mythread_suspend_all(mythread_t *mythread)
{
    if(mythread->stream_opt != MyTHREAD_OPT_WAIT)
        mythread->stream_opt = MyTHREAD_OPT_WAIT;
    
    if(mythread->batch_opt != MyTHREAD_OPT_WAIT)
        mythread->batch_opt  = MyTHREAD_OPT_WAIT;
    
    const struct timespec tomeout = {0, 0};
    
    for (size_t idx = mythread->pth_list_root; idx < mythread->pth_list_size; idx++) {
        mycore_hread_mutex_try_wait(mythread, &mythread->pth_list[idx].data);
        
        while(mythread->pth_list[idx].data.opt != MyTHREAD_OPT_WAIT) {
            mycore_thread_nanosleep(&tomeout);
        }
    }
}

unsigned int mythread_check_status(mythread_t *mythread)
{
    for (size_t idx = mythread->pth_list_root; idx < mythread->pth_list_size; idx++) {
        if(mythread->pth_list[idx].data.status) {
            return mythread->pth_list[idx].data.status;
        }
    }
    
    return MyCORE_STATUS_OK;
}

bool mythread_function_see_for_all_done(mythread_queue_list_t *queue_list, size_t thread_id)
{
    size_t done_count = 0;
    
    mythread_queue_list_entry_t *entry = queue_list->first;
    while(entry)
    {
        if(entry->thread_param[ thread_id ].use >= entry->queue->nodes_uses) {
            done_count++;
            entry = entry->next;
        }
        else
            break;
    }
    
    return done_count == queue_list->count;
}

bool mythread_function_see_opt(mythread_context_t *ctx, volatile mythread_thread_opt_t opt, size_t done_count, const struct timespec *timeout)
{
    mythread_t * mythread = ctx->mythread;
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    
    if(done_count != queue_list->count)
        return false;
    
    if(opt & MyTHREAD_OPT_STOP)
    {
        if(mythread_function_see_for_all_done(queue_list, ctx->id))
        {
            ctx->opt = MyTHREAD_OPT_STOP;
            mycore_hread_mutex_wait(mythread, ctx);
            ctx->opt = MyTHREAD_OPT_UNDEF;
            
            return false;
        }
    }
    else if(opt & MyTHREAD_OPT_QUIT)
    {
        if(mythread_function_see_for_all_done(queue_list, ctx->id))
        {
            mycore_hread_mutex_close(mythread, ctx);
            ctx->opt = MyTHREAD_OPT_QUIT;
            return true;
        }
    }
    
    mycore_thread_nanosleep(timeout);
    
    return false;
}

void mythread_function_queue_batch(void *arg)
{
    mythread_context_t *ctx = (mythread_context_t*)arg;
    mythread_t * mythread = ctx->mythread;
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    
    const struct timespec timeout = {0, 0};
    mycore_hread_mutex_wait(mythread, ctx);
    
    do {
        if(mythread->batch_opt & MyTHREAD_OPT_WAIT) {
            ctx->opt = MyTHREAD_OPT_WAIT;
            
            while (mythread->batch_opt & MyTHREAD_OPT_WAIT) {
                mycore_thread_nanosleep(&timeout);
            }
            
            ctx->opt = MyTHREAD_OPT_UNDEF;
        }
        
        mythread_queue_list_entry_t *entry = queue_list->first;
        size_t done_count = 0;
        
        while(entry)
        {
            mythread_queue_thread_param_t *thread_param = &entry->thread_param[ ctx->id ];
            
            if(thread_param->use < entry->queue->nodes_uses)
            {
                size_t pos = thread_param->use / entry->queue->nodes_size;
                size_t len = thread_param->use % entry->queue->nodes_size;
                
                mythread_queue_node_t *qnode = &entry->queue->nodes[pos][len];
                
                //if((qnode->tree->flags & MyCORE_TREE_FLAGS_SINGLE_MODE) == 0)
                ctx->func(ctx->id, (void*)qnode);
                
                thread_param->use += mythread->batch_count;
            }
            else
                done_count++;
            
            entry = entry->next;
        }
        
        if(done_count == queue_list->count &&
           mythread_function_see_opt(ctx, mythread->batch_opt, done_count, &timeout))
            break;
    }
    while (1);
}

void mythread_function_queue_stream(void *arg)
{
    mythread_context_t *ctx = (mythread_context_t*)arg;
    mythread_t * mythread = ctx->mythread;
    mythread_queue_list_t *queue_list = (mythread_queue_list_t*)mythread->context;
    
    const struct timespec timeout = {0, 0};
    mycore_hread_mutex_wait(mythread, ctx);
    
    do {
        if(mythread->stream_opt & MyTHREAD_OPT_WAIT) {
            ctx->opt = MyTHREAD_OPT_WAIT;
            
            while (mythread->stream_opt & MyTHREAD_OPT_WAIT) {
                mycore_thread_nanosleep(&timeout);
            }
            
            ctx->opt = MyTHREAD_OPT_UNDEF;
        }
        
        mythread_queue_list_entry_t *entry = queue_list->first;
        size_t done_count = 0;
        
        while(entry)
        {
            mythread_queue_thread_param_t *thread_param = &entry->thread_param[ ctx->id ];
            
            if(thread_param->use < entry->queue->nodes_uses)
            {
                size_t pos = thread_param->use / entry->queue->nodes_size;
                size_t len = thread_param->use % entry->queue->nodes_size;
                
                mythread_queue_node_t *qnode = &entry->queue->nodes[pos][len];
                
                //if((qnode->tree->flags & MyCORE_TREE_FLAGS_SINGLE_MODE) == 0)
                ctx->func(ctx->id, (void*)qnode);
                
                thread_param->use++;
            }
            else
                done_count++;
            
            entry = entry->next;
        }
        
        if(done_count == queue_list->count &&
           mythread_function_see_opt(ctx, mythread->stream_opt, done_count, &timeout))
            break;
    }
    while (1);
}

void mythread_function(void *arg)
{
    mythread_context_t *ctx = (mythread_context_t*)arg;
    mythread_t * mythread = ctx->mythread;
    
    mycore_hread_mutex_wait(mythread, ctx);
    
    do {
        if(mythread->stream_opt & MyTHREAD_OPT_STOP || ctx->opt & MyTHREAD_OPT_STOP)
        {
            ctx->opt |= MyTHREAD_OPT_DONE;
            mycore_hread_mutex_wait(mythread, ctx);
            
            if(mythread->stream_opt & MyTHREAD_OPT_QUIT || ctx->opt & MyTHREAD_OPT_QUIT)
            {
                mycore_hread_mutex_close(mythread, ctx);
                ctx->opt = MyTHREAD_OPT_QUIT;
                break;
            }
            
            ctx->opt = MyTHREAD_OPT_UNDEF;
        }
        else if(mythread->stream_opt & MyTHREAD_OPT_QUIT || ctx->opt & MyTHREAD_OPT_QUIT)
        {
            mycore_hread_mutex_close(mythread, ctx);
            ctx->opt = MyTHREAD_OPT_QUIT;
            break;
        }
        
        ctx->func(ctx->id, ctx);
    }
    while (1);
}

#endif /* MyCORE_BUILD_WITHOUT_THREADS */

