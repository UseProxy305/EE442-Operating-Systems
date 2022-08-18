//Berkay İPEK 2304814
//EE442 HW2

//To be able to run the program,
//Create out file
//Run the out file with as following parameters
// (out file) (1st Thread 1st CPU) (2nd Thread 1st CPU) (3rd Thread 1st CPU) (4th Thread 1st CPU) (5th Thread 1st CPU) (1st Thread 1st IO) (2nd Thread 1st IO) (3rd Thread 1st IO) (4th Thread 1st IO) (5th Thread 1st IO) (1st Thread 2nd CPU) (2nd Thread 2nd CPU) (3rd Thread 2nd CPU) (4th Thread 2nd CPU) (5th Thread 2nd CPU)...

//For example, we want to get a threads like
//	1ST		2ND		....
//------CPU-----IO------CPU-----IO	....
//T1 	1	20	6	25	....
//T2 	2	21	7	26	....
//T3 	3	22	8	27	....
//T4 	4	23	9	28	....
//T5 	5	24	10	29	....

//Run the following ones
// ./a.out 1 2 3 4 5 20 21 22 23 24 6 7 8 9 10 25 26 27 28 29 ....


//I choose scheduler algorithm as shortest remaining time

//Necessary Libraries
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

//Defining words
#define STACK_SIZE 8196
#define NUM_THREAD 5
#define NUM_BURST 3

//-------------------------General Structure of Thread Info
struct ThreadInfo {
	ucontext_t context;
	int state;		//0:ready 1:running 2:finished 3:I/O 4:empty
	int cpuAmount1;		//CPU First Burst Time
	int cpuAmount2;		//CPU Second Burst Time
	int cpuAmount3;		//CPU Third Burst Time
	int IOAmount1;		//IO First Burst Time
	int IOAmount2;		//IO Second Burst Time
	int IOAmount3;		//IO Third Burst Time
	int count;		//Spent Burst Time
}ThreadInfo;
//------------------------Just declaration of Functions
void exitThread(int id);
void printInfo();
int isFinished();

//-------------------------Global variables
struct ThreadInfo * berkay[6];
// berkay [0] = > Running Thread To Swap Context
// berkay [i] = > Thread Info for i th Thread 
int * argument;
//Argument that will be passed to swap context function
//Also this arguemnt will hold the value of the thread id of running one

//--------------------------FUNCTIONS
//printCount Function is to print the current burst time on the screen
void printCount(int id){
	int counter=1;//How many tab should I do will be decided by this variable
	if(id==-1){return;}//If it is called with -1, It means there is no current running
	while(1){
		if(id==counter) break;//If we hit the column break it
		else{
			printf("\t");//Put tabs until there is right column
			counter++;	
		}	
	}
	printf("%d\n",berkay[id]->count);//Print the current burst time on the screen
}

//IO Control Funcition is to check IO Operations in the system
//It returns if current thread with given id is exceed the boundary
int IOControl(int id){

	int total1= berkay[id]->cpuAmount1+berkay[id]->IOAmount1;//First Boundary
	int total2= berkay[id]->cpuAmount2+berkay[id]->IOAmount2;//Second Boundary
	int total3= berkay[id]->cpuAmount3+berkay[id]->IOAmount3;//Third Boundary
	
	if(berkay[id]->count >= total1+ total2+total3){//All of IO & CPU are finished
		berkay[id]->state=2;		//Change its state to Finished
		exitThread(id);			//Exit the thread free the memory
		return 0;
	}
	else if(berkay[id]->count >= total1+ total2){
		//If it finishes its 1st and 2nd CPU and IO
		//It can take 3rd CPU, so it goes into ready state		
		berkay[id]->state=0;		//Change its state to Ready
		return 0;		
	}
	else if(berkay[id]->count >= total1){
		//If it finishes its 1st CPU and IO
		//It can take 2nd CPU, so it goes into ready state
		berkay[id]->state=0;		//Change its state to Ready	
		return 0;	
	}
	else {
		//If there is no exceed for boundary
		//State them as in the IO state
		return 1;	
	}
}
//Swap Context Function
//It will take the next running thread id
//It will call the printing function
//It will update ThreadInfo members
void func1(int * x){
	//If *x is equal to -1, that means there is no ready thread in the queue 
	if (*x != -1){//If it is not minus one
		berkay[*x]->state=1;//Change its State to RUNNING
	}
	printInfo();//Printing information about states
	//ready> running> .. etc.
	if(*x!=-1){
			//For the RUNNING Thread
		int total1= berkay[*x]->cpuAmount1+berkay[*x]->IOAmount1;//1st CPU + 1st IO
		int total2= berkay[*x]->cpuAmount2+berkay[*x]->IOAmount2;//2nd CPU + 2nd IO
		int total3= berkay[*x]->cpuAmount3+berkay[*x]->IOAmount3;//3rd CPU + 3rd IO
	
		//Increment the total bust time by one
		berkay[*x]->count++;
		//Print the total burst time for the running case	
		printCount(*x);
		//Sleep 1 second after each print
		
	
		//Check there is an odd number issue
		//This issue occurs when boundary points have odd value
		//Since we are taking input for the 2 seconds
		//We can exceed a boundary in the first increment
	
		//Solution:
		//Since there is no restriction in the homework
		//I assumed program will wait until a new interrupt comes
		if(berkay[*x]->count == total1+ total2+berkay[*x]->cpuAmount3){
			//If it is third boundary, do not increment burst time 
			printf("\n");//wait
		}
		else if(berkay[*x]->count == total1+ berkay[*x]->cpuAmount2){
			//If it is second boundary, do not increment burst time 
			printf("\n");//wait
		}
		else if(berkay[*x]->count == berkay[*x]->cpuAmount1){
			//If it is first boundary, do not increment burst time
			printf("\n");//wait

		}
		else {
			//If it is not on a boundary
			berkay[*x]->count++;//Directly increment the total burst time
			printCount(*x);//Call printer function
		}
	
	
		

		//Update the state if it is on a boundary
		if(berkay[*x]->count >= total1+ total2+berkay[*x]->cpuAmount3){
			berkay[*x]->state=3;	//	I/O 3
		}
		else if(berkay[*x]->count >= total1+ berkay[*x]->cpuAmount2){
			berkay[*x]->state=3;	//	I/O 2	
		}
		else if(berkay[*x]->count >= berkay[*x]->cpuAmount1){
			berkay[*x]->state=3;	//	I/O 1	
		}
		else {
			//If it is not on a boundary
			//Its state will be ready
			berkay[*x]->state=0;	//	Ready State
		}

	}
	
	//Checking threads within IO state
	//We will increment the total burst time and check boundaries
	//and also decide the next state
	int checker=0;
	for(int i=0;i<NUM_THREAD;i++){
		if(*x != i+1){//If it is just passed to IO state, do not modify it
			if(berkay[i+1]->state == 3){//If it is in IO
				//Increment the total burst time
				berkay[i+1]->count++;
				//Check the boundary
				checker=IOControl(i+1);	
				if(checker==0){
				//If it is 0, that means it was on the boundary
				//In the IOControl function, updating members was done 
				}
				else{
					//Else, increment the total burst time
					//Control it again
					berkay[i+1]->count++;
					checker=IOControl(i+1);
									
				}
				

						
			}		
		}
	}
	

}

//Initializer 
void initializeThread(int id,int cpu1,int cpu2,
			int cpu3, int IO1, int IO2, int IO3){
	berkay[id]->state=4;		//empty state
	//Assiging members with argument that is taken from terminal
	berkay[id]->cpuAmount1=cpu1;	
	berkay[id]->cpuAmount2=cpu2;
	berkay[id]->cpuAmount3=cpu3;
	berkay[id]->IOAmount1=IO1;
	berkay[id]->IOAmount2=IO2;
	berkay[id]->IOAmount3=IO3;
	//Assign total burst time as zero initially
	berkay[id]->count=0;
	
}

//Creater
int createThread(int id,void * sm){
	int countFull=0;//Counter for the thread number
	for(int i=0;i<NUM_THREAD;i++){
		if(berkay[i+1]->state != 4){//If it is not empty increment counter
			countFull+=1;
		}
	}
	if(countFull==5){//If there are already 5 thread then return -1 as stated in the hw
		return -1;
	}
	//Context Creation (Ref. : Homework)
	getcontext(&berkay[id]->context);
	berkay[id]->context.uc_link=&berkay[0]->context;
	berkay[id]->context.uc_stack.ss_sp=malloc(STACK_SIZE);
	berkay[id]->context.uc_stack.ss_size=STACK_SIZE;
	makecontext(&berkay[id]->context, (void (*)(void))func1,1,argument);
	berkay[id]->state=0;//Assign it as ready
	return 0;//Return 0 if there is no error 
}
//Selection Algorithm
//SRTF
int selectionSRTF(void){
	//Initially id is -1
	int id=-1;
	int SRTF=99999999;//shortest RT value
	int total=0;//Total burst time for iterated thread
	int currentRT=0;//RT value for iterated thread
	for(int i=0;i<NUM_THREAD;i++){
		if(berkay[i+1]->state==0){//If it is in the ready queue
			//Calculate the total burst time
			total = berkay[i+1]->cpuAmount1+berkay[i+1]->IOAmount1+
				berkay[i+1]->cpuAmount2+berkay[i+1]->IOAmount2+
				berkay[i+1]->cpuAmount3+berkay[i+1]->IOAmount3;
			//Calculate RT value by total burst time - current burst time
			currentRT = total - berkay[i+1]->count;
			if(currentRT <SRTF){//If it is shorter than SRT, then change..
				SRTF=currentRT;
				id=i+1;			
			}		
		}
	}	
	return id;//Return the thread id which has SRT 
}

//Thread Runner
void runThread(int signal){
	//Select the thread which will be in running state	
	int id=-1;
	id=selectionSRTF();

	//Get Context 
	getcontext(&berkay[0]->context);
	//Allocate dynamic memory for the arguemnt of func1 (swap function)
	int * argument=(int*)malloc(sizeof(int));
	*argument=id;//Assign it
	//If there is no running thread then select the first one non-dead thread to settle context
	//Otherwise it will be called as berkay[-1]->context
	if(id==-1){
		for(int i=0;i<NUM_THREAD;i++){
			if(berkay[i+1]->state!=2){id=i;}		
		}
	}
	if(id==-1){id=1;}
	//Context (ref: HW)
	makecontext(&berkay[id]->context, (void (*)(void))func1,1,argument);
	swapcontext(&berkay[0]->context,&berkay[id]->context);
	//Deallocate dynamic memory
	free(argument);
}
//Exiter
void exitThread(int id){
	free(berkay[id]->context.uc_stack.ss_sp);
	
	//Deallocate dynamic memory
	//berkay[id]=NULL; It creates errors since I am iterating lots of times in the threads
	//It will be pointed to NULL at the end of operation
	//Also that means there is no need for FINISH state, which contradicts our HW description
}


//isFinished function is to return 1 when all threads are finished
//0 ow
int isFinished(){
	int counter=0;
	for(int i=0;i<NUM_THREAD;i++){
		if(berkay[i+1]->state==2){//If iterated thread is finished
			counter++;
		}
	}
	if(counter==5)
		return 1;//If there are five threads that are in finished
	return 0;//If not
}

//Init printer
void printInit(){
    printf("Threads :\nT1\tT2\tT3\tT4\tT5\n");
}

//printList Function will be printing the list given state number
//For example stateX = 0 Ready // stateX=2 Finished etc.
void printList(int stateX){
    int commaFlag=0;//To be able to put comma when necessary
    int counterFinished=16;//Character number to be printed when there is finished list printing
    int counterReady=15;//Character number to be printed when there is ready list printing
    for(int i=1;i<6;i++){
        if(berkay[i]->state==stateX){//If desired state is found
                if(commaFlag==1){
                        printf(",");//Print a comma
                        counterFinished--;//Decrement one char
                        counterReady--;//Decrement one char
                }
                printf("T%d",i);
                counterFinished--;//Decrement two char
                counterFinished--;//Decrement two char
                counterReady--;//Decrement two char
                counterReady--;//Decrement two char
                commaFlag=1;//Now you can put comma when there is a new thread
                }
    }

    if(stateX==2){
            while(counterFinished!=0){printf(" ");counterFinished--;}
		//Put an empty char to fill the row
        }
    if(stateX==0){
            while(counterReady!=0){printf(" ");counterReady--;}
		//Put an empty char to fill the row    
	}
}

//printInfo will print the list with the helper function above
void printInfo(){
    printf("running>");//Running State printing
    int id=-1;
   
    for(int i=1;i<6;i++){
        if(berkay[i]->state==1){//If it is in the running state, print it
            printf("T%d",i);
        }
    }
    
    printf("\t");// just print a tab
    
    //Printing ready states
    printf("ready>");
    printList(0);

    //Printing finished states
    printf("finished>");
    printList(2);
    //Printing IO State
    printf("IO>");
    printList(3);
    //Print a new line
    printf("\n");
}



//---------------------------------------------------------MAIN FUNCTION
int main(int argc, char ** argv){
	//Assign Memory for pointers of array
	for(int i=0;i<6;i++)
	berkay[i]=(struct ThreadInfo *) malloc(sizeof(struct ThreadInfo));

	//  Taking input from terminal
	// geeksforgeeks.org/command-line-arguemnts-in-cpp
	argument = (int*)malloc(sizeof(int));
	//Create an alarm for runThread function
	signal(SIGALRM,runThread);
	//error value to hold the error information coming from creating thread 	
	int err;
	*argument=0;
	initializeThread(0,0,0,0,0,0,0);

	for(int i=0;i<NUM_THREAD;i++){
	//Init Create each thread
	initializeThread(i+1,atoi(argv[1+i]),atoi(argv[11+i]),atoi(argv[21+i]),atoi(argv[6+i]),atoi(argv[16+i]),atoi(argv[26+i]));
	err=createThread(i+1,func1);
	//If it returns -1, print an error messahe
	if(err==-1){
		printf("ERRRRRORRRRR---------while creating thread %d\n",i+1);		
		}	
	}
	//Init printing
	printInit();
	//Initial printing info states
	printInfo();
	//Operation while
	while(1){
		sleep(2);//Raise the alarm with each 2 seconds
		raise(SIGALRM);//Call runThread Function
		if(isFinished()==1){//If all threads are finished
			printInfo();//Call the printer info for the last time
			break;
			}
			
	}
	for(int i=0;i<6;i++)
	free(berkay[i]);//Free the alocated pointers of array
	while(1);//Wait in an infinite loop
	return 0;//Return main function
}

