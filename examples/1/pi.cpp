
//
//
//    Copyright (C) 2020 Universitat Politècnica de València - UPV
//
//    This file is part of RUPER-LB: Runtime Unpredictable Performance Load Balancer.
//
//    RUPER-LB is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    RUPER-LB is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with RUPER-LB.  If not, see <https://www.gnu.org/licenses/>. 
//
//
//    contact emails:
//
//        vicent.gimenez.alventosa@gmail.com
// 

#include <random>
#include <chrono>
#if (defined _USE_MPI_)
#include <mpi.h>
#endif
#include "ruperLB.hh"

inline void sample(std::mt19937& gen,
		   const unsigned long long niter,
		   unsigned long long& in){
  long long unsigned inAux;
  std::uniform_real_distribution<float> dis(0.0,1.0);
  for(unsigned long long i = 0; i < niter; ++i){
    float x = dis(gen);
    float y = dis(gen);
    float r2 = x*x + y*y;
    if(r2 <= 1.0)
      ++in;
  }
}

void loop(const size_t ith,
	  LB::task* task,
	  const long long int LBinterval,
	  const unsigned verbose,
	  unsigned long long* total,
	  unsigned long long* totalIn){

  printf("Starting thread %lu\n",static_cast<unsigned long>(ith));
  //Init random generator
  std::random_device rd;
  std::mt19937 gen(rd());
  
  //Start worker
  task->workerStart(ith,verbose);

  //Get actual time and report time
  std::chrono::steady_clock::time_point treport =
    std::chrono::steady_clock::now() + std::chrono::seconds(LBinterval/2);
  printf("Thread %lu: Time until first report: %lds\n",
	 std::chrono::duration_cast<std::chrono::seconds>
	 (treport-std::chrono::steady_clock::now()).count());

  std::chrono::steady_clock::time_point tcheckpoint =
    std::chrono::steady_clock::now() + std::chrono::seconds(LBinterval);
  if(ith == 0)
    printf("Thread 0: Time until first checkpoint: %lds\n",
	   std::chrono::duration_cast<std::chrono::seconds>
	   (tcheckpoint-std::chrono::steady_clock::now()).count());
      
  //Get number of iterations to do
  unsigned long long toDo = task->assigned(ith);
  printf("Thread %lu: Points to sample: %llu\n",
	 static_cast<unsigned long>(ith),toDo);
  //Initialize counters
  unsigned long long done = 0;
  const unsigned long long chunkSize = 100;
  unsigned long long in = 0;
  for(;;){
    //printf("Hola %ul!\n",static_cast<unsigned long>(ith));
    sample(gen,chunkSize,in);
    done += chunkSize;

    //Check if is time to report
    std::chrono::steady_clock::time_point tnow = std::chrono::steady_clock::now();
    if(treport < tnow){
      printf("Thread %lu: Time to report.\n",
	     static_cast<unsigned long>(ith));
      int err;
      std::chrono::seconds::rep nextReport = task->report(ith,done,&err,verbose);
      treport = tnow + std::chrono::seconds(nextReport);

      //Update hists to do
      unsigned long long newAssign = task->assigned(ith);
      if(toDo != newAssign){
	printf("Thread %u: Iterations updated from %llu to %llu\n",
	       static_cast<unsigned>(ith),toDo,newAssign);
	toDo = newAssign;
      }
    }
    if(ith == 0 && tcheckpoint < tnow){
      printf("Thread %lu: Perform a checkpoint.\n",
	     static_cast<unsigned long>(ith));
      task->checkPoint(verbose);
      tcheckpoint = tnow + std::chrono::seconds(LBinterval); 
    }

    //Check if we reached the iteration goal
    if(toDo <= done){
      //Sleep few seconds to ensure minimum time between reports
      std::this_thread::sleep_for (std::chrono::seconds(2));
      //Perform a report
      int err;
      task->report(ith,done,&err,verbose);
      int why = 0;
      if(task->workerFinish(ith,why,verbose)){
	printf("Thread %u: Task finished\n",
	       static_cast<unsigned>(ith));
	*total = done;
	*totalIn = in;
	return; //The worker can finish the task
      }
      printf("Thread %u: Unable to finish the task. Reason ID: %d\n",
	     static_cast<unsigned>(ith),why);
      switch(why){
      case 0: //Update assigned histories
	{
	  const unsigned long long newAssign = task->assigned(ith);
	  printf("Thread %u: Assigned histories "
		 "updated from %llu to %llu\n\n",
		 static_cast<unsigned>(ith),toDo,newAssign);
	  toDo = newAssign;
	}break;
      case 1: //Checkpoint required
	{
	  task->checkPoint(verbose);
	  //Update histories
	  const unsigned long long newAssign = task->assigned(ith);
	  if(newAssign != toDo){
	    printf("Thread %u: Assigned histories "
		   "updated from %llu to %llu\n\n",
		   static_cast<unsigned>(ith),toDo,newAssign);
	    toDo = newAssign;
	  }
	}break;
      case 2: //Waiting response from rank 0
	{
	  printf("Thread %u: Waiting a response from rank 0 to finish.\n\n",
		 static_cast<unsigned>(ith));
	  toDo += 3*chunkSize;
	}break;
      default: //Unexpected response
	{
	  printf("Thread %u: Unable to finish, unexpected reason.\n\n",
		 static_cast<unsigned>(ith));
	  toDo += 3*chunkSize;
	}
      }
	
    }    
  }
  
}

int main(int argc, char** argv){
  if(argc != 4){
    printf("usage:%s nthreads niterations balanceInterval(s)\n",argv[0]);
    return 0;
  }

  int workers = atoi(argv[1]);
  long long int niter = std::max(atoll(argv[2]),1ll);
  long long int LBinterval = std::max(atoll(argv[3]),1ll);

  if(workers <= 0){
    printf("Requires, as least, one thread.\n");
    return 1;
  }

#if (defined _USE_MPI_)

  //Init MPI
  int MThProvided;
  int MPIinitErr = MPI_Init_thread(nullptr, nullptr, MPI_THREAD_SERIALIZED,&MThProvided);
  if(MPIinitErr != MPI_SUCCESS){
    printf("Unable to initialize MPI. Error code: %d\n",MPIinitErr);
    return -1;
  }
  if(MThProvided != MPI_THREAD_SERIALIZED){
    printf("Warning: The MPI implementation used doesn't provide"
	   "support for serialized thread communication.\n"
	   "This could produce unexpected behaviours or performance issues.\n");
  }  
  
  //Get MPI rank
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //Redirect stdout to a log file
  char stdoutFilename[100];
  snprintf(stdoutFilename,100,"rank-%03d.log",rank);
  printf("Rank %d: Redirect 'stdout' to file '%s'\n",rank,stdoutFilename);
  if(freopen(stdoutFilename, "w", stdout) == NULL){
    printf("Rank %d: Can't redirect stdout to '%s'\n",rank,stdoutFilename);
    return 2;
  }
  else{
    printf("\n**** Rank %d Log ****\n\n",rank);
  }
#endif  
  
  printf("Iterations to do: %lld\n",niter);
  printf("Checkpoints interval: %lld\n",LBinterval);
  
  size_t nthreads = size_t(workers);

  //Create the load balance task
  LB::task task;

  //Set time between checkpoints
  task.setCheckTime(LBinterval);

  //Set time threshold
  task.minTime(LBinterval/2);
  
  //Initialize the task
#if (defined _USE_MPI_)
  task.setCheckTimeMPI(LBinterval);
  task.init(nthreads,niter,MPI_COMM_WORLD,1,2,"LB-log",3);
#else
  task.init(nthreads,niter,"LB-log",3);
#endif
  
  std::vector<std::thread> threads;
  std::vector<unsigned long long> ins(nthreads,0);
  std::vector<unsigned long long> dones(nthreads,0);
  for(int ith = 0; ith < nthreads; ++ith){
    threads.push_back(std::thread(loop,ith,&task,LBinterval,3,&dones[ith],&ins[ith]));
  }

  for(auto& thread : threads){
    thread.join();
  }

  unsigned long long totalIn = 0;
  printf("Inner points on each thread:\n");
  for(const auto in : ins){
    totalIn += in;
    printf("%llu\n",in);
  }

  unsigned long long sampled = 0;
  printf("Sampled points by each thread:\n");
  for(const auto done : dones){
    sampled += done;
    printf("%llu\n",done);
  }

  printf("Total sampled points: %llu\n",sampled);
  printf("Total inner points  : %llu\n",totalIn);
  printf("Pi estimation: %.10f\n",4.0*static_cast<double>(totalIn)/static_cast<double>(sampled));

  FILE* fout = fopen("localSpeeds.rep","w");
  task.printReport(fout);
  fclose(fout);
#if (defined _USE_MPI_)
  fout = fopen("MPISpeeds.rep","w");
  task.printReportMPI(fout);
  fclose(fout);
  MPI_Barrier(MPI_COMM_WORLD); //Sync all MPI processes
  // Finalize the MPI environment.
  MPI_Finalize();  
#endif  
  return 0;
}
