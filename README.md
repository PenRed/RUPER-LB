
# RUPER-LB

RUPER-LB (Runtime Unpredictable Performance Load Balancer) is a load balance library for loosely coupled applications running on heterogeneous environments with unpredictable performance variability. It is capable to balance at both, multi-thread and multi-process levels.

# Restrictions

To be able to use RUPER-LB to balance an application, this one should satisfy some restrictions.

- Firstly, the application must be split in independent tasks, each of wich respresent a chunck of the whole process and will be processed by several workers. Usually, these workers are constitued by one thread. During the execution of each task, the application should not require any communication or syncronisation point among the executing workers. Nevertheless, if communications are required, their overhead on the task performance should be negligible. Notice that the capability of split the process in independent tasks is a soft requirement, because the whole process could be included in a single task.

- Secondly, the application must be capable of measure its speed at runtime. RUPER-LB assumes that the application behaves like an iterative process, whose speed is measured in iterations per second. How that *iterations* are interpreted depends on the application. For example, in a radiation transport Monte-Carlo simulation code, each iteration could be interpreted as a primary particle or history, in a file processing application, each iteration could be a chunk of files, etc.

- Finally, the number of iterations to compute by each thread or process should be allowed to be changed at runtime. However, RUPER-LB neither requires an homogeneous computational cost for the iterations nor a previous balanced distribution.

# Usage

A basic example of how to use the RUPER-LB library for both, multi-threading and MPI balance, is shown in the *examples* folder. First of all, the library header must be included in the aplication source code,

```
#include "ruperLB.hh"
```

Then, to create a task and initialize it, we should specify a checkpoint time interval and a time threshold to stop the balance procedure.

```
//Assign a checkpoint interval in seconds
long long int LBinterval = 30;

//Create the load balance task
LB::task task;

//Set the time between checkpoints
task.setCheckTime(LBinterval);

//Set time threshold
task.minTime(LBinterval/2)
```

Then, the task initialization depends on the balance type, i.e. if MPI is enabled or disabled. To be able to detect if the code has been compiled with MPI capabilities, we define the *\_USE\_MPI\_* value in the provided cmake file.

```
//Initialize the task
#if(defined _USE_MPI_)
    task.setCheckTimeMPI(LBinterval);
    task.init(nthreads,niter,MPI_COMM_WORLD,1,2,"LB-log",3);
#else
    task.init(nthread,niter,"LB-log",3);
#endif
```

In the MPI enabled case, the rank 0 will request reports to the other ranks. The function *setCheckTimeMPI* specify the time interval, in seconds, between these requests. Then, the *init* function initializes the task. This function takes as arguments the number of workers (*nthreads*), in the current MPI process, the number of iterations to do *niter*, which will be reasigned during the balance operations, the *MPI_Comm*, which, in this case, has been obtained from a *MPI_Comm_rank* call, two MPI tags, one for the rank 0 requests and another for the responses, a filename where the log messages will be written and, finally, a verbose level. Notice that, in this example, the number of workers in each process is equal to the number of threads. However, in another case, each worker could use more than a single thread. The MPI init signature can be found in the header file and is as follows,

```
int init(const size_t nw,
         const unsigned long long nIter,
         const MPI_Comm commIn,
         const int tagRequests,
         const int tagProcess,
         const char* logFileName,
         const unsigned verbose = 1);
```

In the other case, only multi-threading, the *init* function only takes the number of threads, the number of iterations, the log file name and the verbose level as arguments.

Once initialized, each worker must inform to the task when it starts to perform the execution using the function *workerStart*,

```
task.workerStart(ith,verbose);
```

where *ith* is the worker number and *verbose* the log verbose level. Then, during the execution, each worker must perform periodic reports to the task class using the *report* method. These reports include the worker number and the number of iterations done. Also, the report method returns a suggested time interval, in seconds, to perform the next report.

```
std::chrono::seconds::rep nextReport = task.report(ith,done,&err,verbose);
```
the value of *err* will be set to the *LB\_SUCCESS* value of the enumeration *LB\_ERRCODE* on success, or to another value in the case of error. The signature is as follows,

```
std::chrono::seconds::rep report(const size_t iw,
                                 const unsigned long long nIter,
                                 int* errRet,
                                 const unsigned verbose = 3);
```

Also, the worker should obtain the updated number of iterations to do from the task. This is done via the function *assigned*, which only takes the worker number as argument and return the assigned iterations,

```
unsigned long long newAssign = task.assigned(ith);
```

this function could be called, for example, after the report. Notice that both operations are thread safe, i.e. all threads can perform reports and read the assigned iterations concurrently. Furthermore, these functions do not use syncronisation points, thus have a negligible impact in the application performance.

In addition to reports, some worker must call the *checkPoint* function periodically. This function will block the reports to recalculate the iteration assignation for each worker according to the reported speeds. The *checkPoint* function only takes a verbose level as argument and returns an error code, which success value is *LB\_SUCCESS*,

```
int err = task.checkPoint(verbose);
```

Notice that the other workers will be blocked during the checkpoint computation only if they try to perform a report. Finally, if a worker considers that it finished their assigned iterations, the *workerFinish* function should be called. This one will return *true* if the finish petition is accepted or *false* if is rejected. As arguments, the function takes the worker number, an integer to store the cause of a rejected petition, and the verbose level,

```
int why;
if(task.workerFinish(ith,why,verbose)){
    //Finished
}
else{
    //Finish petition rejected
    switch(why){
        cause 0:{} break; //Worker requires to update the assigned iterations
        cause 1:{} break; //A checkpoint is required
        cause 2:{} break; //Waiting response from rank 0
    }
}
```

The causes to reject a finish petition are summarised in the *switch* statement. First, a *0* value means that the worker has an outdated value of its assigned iterations and must update it via the *assigned* function. Secondly, a value of *1* means that the task requires a new *checkpoint* because the last balance is outdated. Finally, the *2* value appears only if MPI is enabled. This means that the rank 0 does not granted permission to the other ranks to finish the task. Notice that the report and checkpoint procedure is the same for both, with and without MPI balance. In the MPI case, the first call to *workerStart* function in each MPI process creates an internal thread to handle all the balances and reports transparently. As the MPI balance could change the number of iterations to do by each MPI process, a MPI process can't accept finish petitions of its local workers without permission of the MPI balance.
