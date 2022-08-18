//---------------------------------------PRE PROCESSS-------------------------------------------------
//Including necessary libraries
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <semaphore.h>

#define false 0
#define true 1

//Typedefinition of boolean variable
typedef int bool;
//---------------------------------------END OF PRE PROCESSS-------------------------------------------

//---------------------------------------DECLARE STRUCTS-----------------------------------------------
struct atom{
    int atomID;
    char atomTYPE; //C, H, O or N
    };

struct Information {
    int moleculeTYPE ;    //which molecule is generated 1:H2O 2:CO2 3:NO2 4:NH3
    };
//---------------------------------------END OF STRUCTS------------------------------------------------

//---------------------------------------GLOBAL VARIABLES----------------------------------------------

//semaphores for each atom
//As it is generated this semaphore will increment by one. Their initial values are zero
sem_t semC,semH,semN,semO;
//Semaphore to check if one of thread reads/write atom semaphores
sem_t semAtomAmount;
//Semaphores for counting each molecule
//As it is generated, this semaphore will incremenet by one. Their initial values are zero
sem_t CO2,NH3,NO2,H2O;
//Semaphore that will count how many corresponding atom is used. (That is my solution to TS problem)
sem_t semC_usage,semO_usage,semN_usage,semH_usage;
//Semaphore that will tell whether there is a generated molecule or not
sem_t semInfo;
//Threads for each molecule
pthread_t CO2_t;
pthread_t NO2_t;
pthread_t H2O_t;
pthread_t NH3_t;
//G parameter that will decide how much tine will generation be sleeping
int numberG=100;
//Information object that will hold the information to be printed
struct Information info;    
//---------------------------------------END OF GLOBAL VARIABLES----------------------------------------




//------------------------------COMPOUND THREAD--------------------------------------
//This thread will be called when there is a corresponding molecule.

void * composer_H2O(void * arg){
	//Since we are going to update semaphores, lock it
	sem_wait(&semAtomAmount);
	//Decrease the semaphore for each atom
	//Increase the semaphore usage
	sem_wait(&semH);
	sem_post(&semH_usage);
	sem_wait(&semH);
	sem_post(&semH_usage);
	sem_wait(&semO);
	sem_post(&semO_usage);
	//Increase the molecule semaphore
	sem_post(&H2O);
	sem_post(&semAtomAmount);
	//Set the molecule type so that message will be printed accurate
	info.moleculeTYPE=1;
	//Set the message semaphore so main thread will understand there is a new molecule
	sem_post(&semInfo);
}
//All of the molecule is generated with a same methodology
void * composer_CO2(void * arg){

	sem_wait(&semAtomAmount);
	sem_wait(&semC);
	sem_post(&semC_usage);
	sem_wait(&semO);
	sem_post(&semO_usage);
	sem_wait(&semO);
	sem_post(&semO_usage);
	sem_post(&CO2);
	sem_post(&semAtomAmount);
	info.moleculeTYPE=2;
	sem_post(&semInfo);
}
void * composer_NH3(void * arg){

	sem_wait(&semAtomAmount);
	sem_wait(&semN);
	sem_post(&semN_usage);
	sem_wait(&semH);
	sem_post(&semH_usage);
	sem_wait(&semH);
	sem_post(&semH_usage);
	sem_wait(&semH);
	sem_post(&semH_usage);
	sem_post(&NH3);
	sem_post(&semAtomAmount);
	info.moleculeTYPE=4;
	sem_post(&semInfo);

}
void * composer_NO2(void * arg){
	sem_wait(&semAtomAmount);
	sem_wait(&semN);
	sem_post(&semN_usage);
	sem_wait(&semO);
	sem_post(&semO_usage);
	sem_wait(&semO);
	sem_post(&semO_usage);
	sem_post(&NO2);
	sem_post(&semAtomAmount);
	info.moleculeTYPE=3;
	sem_post(&semInfo);
}

//------------------------------COMPOUND THREAD-----------------------------------

//----------------------------------------CHECKER FUNCTION FOR MOLECULE-----------------------------------------------------
//After each generation, this function will be called
//And this function will create a thread for compounds
void molecule_finder(int * prioC, int * prioH,int * prioO,int * prioN){
	//Since we will achieve number of atoms by semaphores, we will lock it by semAtomAmount
	int semValH,semValO,semValN,semValC;//variables that will hold the number of atoms in queue
	sem_wait(&semAtomAmount);
	sem_getvalue(&semH,&semValH);
	sem_getvalue(&semN,&semValN);
	sem_getvalue(&semO,&semValO);
	sem_getvalue(&semC,&semValC);
	sem_post(&semAtomAmount);
	//Assume that there is no possible molecule in the queue
	bool CO2_b=false;
	bool NO2_b=false;
	bool H2O_b=false;
	bool NH3_b=false;
	//If there is enough amount, then turn it's variable to true
	if((semValC>=1)&&(semValO>=2)){CO2_b=true;}
	if((semValN>=1)&&(semValO>=2)){NO2_b=true;}
	if((semValN>=1)&&(semValH>=3)){H2O_b=true;}
	if((semValH>=2)&&(semValO>=1)){NH3_b=true;}
	//count variable will hold the number of possible molecules inside the queue
	//How many possible molecule can be made	
	int count=CO2_b+NO2_b+H2O_b+NH3_b;
	if(count==1){//if there is only one possible option then create it
		if((semValC>=1)&&(semValO>=2)){pthread_create(&CO2_t,NULL,&composer_CO2,NULL);pthread_join(CO2_t,NULL);}
		else if((semValN>=1)&&(semValO>=2)){pthread_create(&NO2_t,NULL,&composer_NO2,NULL);pthread_join(NO2_t,NULL);}
		else if((semValN>=1)&&(semValH>=3)){pthread_create(&NH3_t,NULL,&composer_NH3,NULL);pthread_join(NH3_t,NULL);}
		else if((semValH>=2)&&(semValO>=1)){pthread_create(&H2O_t,NULL,&composer_H2O,NULL);pthread_join(H2O_t,NULL);}
	} 
	else if(count>1) {//If there is more than one option, then we should look TS values 
		int indexC,indexH,indexO,indexN;
		sem_wait(&semAtomAmount);//Atom usage will be read, therefore lock the semaphore
		sem_getvalue(&semH_usage,&indexH);
		sem_getvalue(&semN_usage,&indexN);
		sem_getvalue(&semO_usage,&indexO);
		sem_getvalue(&semC_usage,&indexC);
		sem_post(&semAtomAmount);//Unlock it 
		//There are two possible senario,
		// 1) CO2-NO2-last O is coming
		// 2) NH3-NO2-last N is coming
		if((CO2_b==true)&&(NO2_b==true) ){//If this is the case, then look C and N TS values 
			if(prioC[indexC]<prioN[indexN])	{//if C has come first,(TS values)
				pthread_create(&CO2_t,NULL,&composer_CO2,NULL);
				pthread_join(CO2_t,NULL);			
			}
			else{//If N has come first
				pthread_create(&NO2_t,NULL,&composer_NO2,NULL);
				pthread_join(NO2_t,NULL);
			}	
		}
		else if((NH3_b==true)&&(NO2_b==true) ){//If this is the case, then look H and O TS values 
			if(prioH[indexH]<prioO[indexO])	{//if the first H has come first,(TS values)
				pthread_create(&NH3_t,NULL,&composer_CO2,NULL);
				pthread_join(NH3_t,NULL);			
			}
			else{//if the first O has come first,(TS values)
				pthread_create(&NO2_t,NULL,&composer_NO2,NULL);
				pthread_join(NO2_t,NULL);
			}
		}

		else{printf("ERRORRRRRRR!!!!!!!!!!!!!!-------------------------\n");}//If there is another senario that will print error message :((
	}
	else{}//If there is no possible molecule than do nothing
}


//----------------------------------------END OF CHECKER FUNCTION-----------------------------------------------


//------------------------------ATOM THREAD--------------------------------------
//Producing atoms
//Note *: Each generation of the atom will cause to create this thread and kill it
//Passing arguemnt method is taken from CodeVault
void * Produce_C(void * arg ){
	struct atom * atomPtr = (struct atom *) arg;//	Take argument from main thread
	double waitTime;
	sem_wait(&semAtomAmount);		//Since we are increasing # of C by one, lock the semaphore
	printf("%c with ID: %d is created\n",atomPtr->atomTYPE,atomPtr->atomID);
	sem_post(&semC);			//	Increment semaphore C
        waitTime=((double)rand() / (RAND_MAX)); // create random numberbetween 0 and 1
        waitTime= -log(1-waitTime) / numberG;   // exp dist function tocalculate wait time
	sleep(waitTime);			//	Waiting time 
	sem_post(&semAtomAmount);		//	Unlock it
	}
void * Produce_H(void * arg ){
	double waitTime;
	struct atom * atomPtr = (struct atom *) arg;			//Take argument from main thread
	sem_wait(&semAtomAmount);					//Since we are increasing # of C by one, lock the semaphore
	printf("%c with ID: %d is created\n",atomPtr->atomTYPE,atomPtr->atomID);
	sem_post(&semH);						//	Increment semaphore C	
        waitTime=((double)rand() / (RAND_MAX)); 			// 	create random numberbetween 0 and 1
        waitTime= -log(1-waitTime) / numberG;   			// 	exp dist function tocalculate wait time
	sleep(waitTime);						//	Waiting time 
	sem_post(&semAtomAmount);					//	Unlock it
	}
//Other cases are made within same idea of Carbon or Hydrogen
void * Produce_N(void * arg ){
	struct atom * atomPtr = (struct atom *) arg;
	double waitTime;	
	sem_wait(&semAtomAmount);
	printf("%c with ID: %d is created\n",atomPtr->atomTYPE,atomPtr->atomID);
	sem_post(&semN);
        waitTime=((double)rand() / (RAND_MAX)); 
        waitTime= -log(1-waitTime) / numberG;   
	sleep(waitTime);
	sem_post(&semAtomAmount);
	}
void * Produce_O(void * arg ){
    	double waitTime;
	struct atom * atomPtr = (struct atom *) arg;
	sem_wait(&semAtomAmount);
	printf("%c with ID: %d is created\n",atomPtr->atomTYPE,atomPtr->atomID);
	sem_post(&semO);
        waitTime=((double)rand() / (RAND_MAX)); 
        waitTime= -log(1-waitTime) / numberG;   
	sleep(waitTime);
	sem_post(&semAtomAmount);

	}

//------------------------------MAIN THREAD--------------------------------------
int main(int argc, char * argv[]){
	//  Taking input from terminal
	// Inspired from gnu.org
	int opt;
	int numberM=40;

	while((opt=getopt(argc,argv,":m:g:"))!=-1){
		switch(opt){
			case 'm':
			    numberM=atoi(optarg);
			    break;
			case 'g':
			    numberG=atoi(optarg);
			    break;
			default :
			    break;
		}
	}

	int numberTotalAtoms = numberM;//Total # of atoms
	int numberC=numberM/4;
	int numberH=numberM/4;
	int numberO=numberM/4;
	int numberN=numberM/4;
	//Necessary initialization
	bool conditionGenerate=true;//This will check if generation number is exceed or not
	double waitTime;            //After each generation it will sleep that much time
	int selectionAtom;          // 0: create C ; 1: create H ; 2: create O ; 3: create N
	srand(time(NULL));          // Random generator seed is set to NULL, so that each calling rand function will result in different sequence
	int mAtomID=0;              // atomID

	//The list for priority problems
	//They will hold the atomID's of each atom that will be generated
	//When an atom X is used, we are going to increase the semaphore, semX_usage.
	//Thus, priorityListOfX[semaX_usage] will hold the TS for each atom X.  
	int priortyListOfC[numberC],priortyListOfH[numberH],priortyListOfN[numberN],priortyListOfO[numberO];

	//Necessary initialization for PTHREADS
	pthread_t berkay[numberTotalAtoms];                               //ATOM ASSIGN THREAD
	//Semaphore initializations
	//There is no atom at the beggining therefore they are initialized as zero
	sem_init(&semC,0,0);
	sem_init(&semH,0,0);
	sem_init(&semN,0,0);
	sem_init(&semO,0,0);
	//There will be generated atom at the beggining therefore it is initialized as one
	sem_init(&semAtomAmount,0,1);
	//There is no molecule at the beggining therefore they are initialized as zero
	sem_init(&CO2,0,0);
	sem_init(&NH3,0,0);
	sem_init(&NO2,0,0);
	sem_init(&H2O,0,0);
	//There is no used atom at the beggining therefore they are initialized as zero
	sem_init(&semC_usage,0,0);
	sem_init(&semO_usage,0,0);
	sem_init(&semN_usage,0,0);
	sem_init(&semH_usage,0,0);
	//There is no created molecule  at the beggining therefore they are initialized as zero
	sem_init(&semInfo,0,0);
	//Index location that will help me to find TS
	int indexC=0;
	int indexO=0;
	int indexH=0;
	int indexN=0;
	//GENERATE ATOM AND CALL OTHER THREADS WHEN NECESSARY
	for(int i=0; i<numberTotalAtoms;i++){
		conditionGenerate=true;                 // Reset the flag after each generation
		struct atom nextAtom;                   // atom that will be generated
		mAtomID++;                              // Increment atom ID
		nextAtom.atomID=mAtomID;                //Assign atom nubmer
		while(conditionGenerate){
			selectionAtom=(rand() % 4);         // create random number between 0 and 3
			switch(selectionAtom){
				case 0:                         //If it is C,
					if(numberC==0){break;}      //If C is no longer need in total, do not set the flag
					else{
						conditionGenerate=false;//Set the flag as false since it will be generated
						nextAtom.atomTYPE='C';
						numberC--;		//Decrease the # of C atoms 
						pthread_create(&berkay[i],NULL,&Produce_C,&nextAtom);//Create the thread
						priortyListOfC[indexC]=mAtomID;//Set the atomID as TS at the location indexC
						indexC++;		//Increase since a new C atom is generated
					}
					break;
				//Other cases are made within same idea of Carbon
				case 1:
					if(numberH==0){break;}
					else{
						conditionGenerate=false;
						nextAtom.atomTYPE='H';
						numberH--;
						pthread_create(&berkay[i],NULL,&Produce_H,&nextAtom);
						priortyListOfH[indexH]=mAtomID;
					indexH++;
					}
					break;
				case 2:
					if(numberO==0){break;}
					else{
						conditionGenerate=false;
						nextAtom.atomTYPE='O';
						numberO--;
						pthread_create(&berkay[i],NULL,&Produce_O,&nextAtom);
						priortyListOfO[indexO]=mAtomID;
						indexO++;
					}
					break;
				case 3:
					if(numberN==0){break;}
					else{
						conditionGenerate=false;
						nextAtom.atomTYPE='N';
						numberN--;
						pthread_create(&berkay[i],NULL,&Produce_N,&nextAtom);
						priortyListOfN[indexN]=mAtomID;
						indexN++;
					}
					break;
			}//end switch 
		}//end while

		//waits until assigning an atom is finished
		pthread_join(berkay[i],NULL);//Wait finish
		//Check a molecule can be created or not 
		molecule_finder(priortyListOfC,priortyListOfH,priortyListOfO,priortyListOfN);
		int res;
		//Check the value of semInfo so that it can understand if there is a molecule or not
		sem_getvalue(&semInfo,&res);
		if(res==1){
			switch(info.moleculeTYPE){
				case 1:
					printf("H2O is generated\n");
					break;
				case 2:
					printf("CO2 is generated\n");
					break;
				case 3:
					printf("NO2 is generated\n");
					break;
				case 4:
					printf("NH3 is generated\n");
					break;
				default:
					printf("THERE IS A WRONG GENERATED MOLECULE\n");
					break;
			}//end switch
			sem_wait(&semInfo);//Decrement the value of semInfo since it is printed
		}//end if 
	
	}//end for loop (generation of atoms)
	//DESTROYING PTHREAD OBJECTS
	sem_destroy(&semC);
	sem_destroy(&semH);
	sem_destroy(&semN);
	sem_destroy(&semO);
	sem_destroy(&semAtomAmount);
	sem_destroy(&CO2);
	sem_destroy(&NH3);
	sem_destroy(&NO2);
	sem_destroy(&H2O);
	sem_destroy(&semInfo);
	sem_destroy(&semC_usage);
	sem_destroy(&semO_usage);
	sem_destroy(&semN_usage);
	sem_destroy(&semH_usage);

	return 0;
}
//------------------------------END OF MAIN THREAD-----------------------------------
