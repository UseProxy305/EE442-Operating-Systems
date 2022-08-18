//---------------------------------------PREPROCESSS-------------------------------------------------
//Including necessary libraries
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

//Defining some words so that code can be more readable
#define NUM_ATOMS 4
//assume num_molecules=num_atoms
#define NUM_TUBES 3
#define false 0
#define true 1

//Typedefinition of boolean variable
typedef int bool;
//---------------------------------------END OF PREPROCESSS-------------------------------------------

//---------------------------------------DECLARESTRUCTS-----------------------------------------------
struct atom{
    int atomID;
    char atomTYPE; //C, H, O or N
    };
struct tube{
    int tubeID;
    int tubeTS; // time stamp (ID of the atom spilled first)
    int moleculeTYPE; // 1:H2O, 2:CO2, 3: NO2, 4: NH3 ,0: Not Yet
    bool atomsCanBeAdded[NUM_ATOMS];// => [C,H,N,O] //[true,true,true,true] means you can add all atoms //[true,true,false,false] You can add C and H not N or O
    int atomNumber;
    int possibleMolecules[NUM_ATOMS];// => {H2O,CO2,NO2,NH3}        same methodology with atomsCanBeAdded
    char atoms[NUM_ATOMS]; // => Holds atoms inside in a corresponding tube
// IF THERE IS NO ATOM THEN IT WILL BE PLACED AS 'X' notation
    };
struct Information {
    int tubeID; //where does molecule create
    struct tube mytube ;    //which molecule is generated
    };
//---------------------------------------END OF STRUCTS------------------------------------------------

//---------------------------------------GLOBAL VARIABLES----------------------------------------------
bool END_OF_EVERYTHING=false; // This variable holds for the situation ifthe program is finished or not. So that threads can be destroyed

//Global pthread objects
pthread_mutex_t mutex_tubes;    //Mutex corresponds to accessing global tubes variable
pthread_mutex_t mutexInfo;      //Mutex corresponds to accessing global information variable
pthread_cond_t printInfo;       //Condition variable corresponds to starts to print Information
pthread_t tubesUpdate[NUM_TUBES]; //Three threads will update different tube
pthread_cond_t atomNew[NUM_TUBES];  //Condition variable corresponds to update tube (in which atom will be replaced) information
pthread_cond_t updateTube_c;
pthread_t printInfo_t;  //Thread will print molecule information
struct Information info;    //Information object that will hold the information to be printed
struct tube tubes[NUM_TUBES];   // Creating tubes
//---------------------------------------END OF GLOBAL VARIABLES----------------------------------------

//----------------------------------------FUNCTIONS-----------------------------------------------------

//Restarting tube in the given index
void restartTube(int index){
    tubes[index].tubeTS=0;
    tubes[index].moleculeTYPE=0;
    tubes[index].atomNumber=0;
    for(int j=0; j<NUM_ATOMS;j++){
            tubes[index].atomsCanBeAdded[j] =true;
            tubes[index].atoms[j] ='X';
    }

    //possible molecule update does not need since I am checking only atomsCanBeAdded field when a new atom comes
}

//Initialize all tubes (also assign ID's)
void initializeTubes(){
    for(int i =0; i<NUM_TUBES;i++){
        tubes[i].tubeID=i+1;
        tubes[i].tubeTS=0;
        tubes[i].moleculeTYPE=0;
        tubes[i].atomNumber=0;
        for(int j=0; j<NUM_ATOMS;j++){
			tubes[i].atomsCanBeAdded[j] =true;
			tubes[i].atoms[j] ='X';}
        }
    }
//Placing atom into a specified tube (given index)
void placeAtomToTube(int index, char atomType, int atomId){
    for(int i =0;i<NUM_ATOMS;i++){
        if(tubes[index].atoms[i]=='X'){//Finding first empty location in the list
            tubes[index].atoms[i]=atomType;//locate atom into that location
            tubes[index].atomNumber++;//increment atom number
            if(tubes[index].atomNumber==1){//If it is the first location, then assign the tubeTS as atomID
                    tubes[index].tubeTS=atomId;
            }
            break;//If an empty location hits, then directly break it
        }
    }

}

//This function returns how many tube (given) atom can be inserted into.
int possibleLocations(char atomTYPE){
    //This count value counts locations
    int count=0;
    int type=-1;//This will hold the index for the atom type
    switch(atomTYPE){
        case 'C':
            type=0;
            break;
        case 'H':
            type=1;
            break;
        case 'N':
            type=2;
            break;
        case 'O':
            type=3;
            break;
        default:
            break;
    }
    for (int j =0; j<NUM_TUBES;j++){//Iterating all tubes
        if(tubes[j].atomsCanBeAdded[type]==true){//If atom can be inserted
                count++;//Increment count
        }
    }
    return count;
}


// Checks if all tubes are empty or not
bool allTubesEmpty(){
    for(int i =0; i<NUM_TUBES;i++){//Iterating all tubes
            if(tubes[i].atomNumber!=0)
                return false;//If there is a tube with non-zero # of atoms
    }
    //If iteration is finished, then it means all tubes are empty
    return true;
}

// Returns # of (atomType) atom is in the tube (index)
int howManyAtomInTube(int index, char atomType){
    int count=0;
    for (int j =0; j<NUM_ATOMS;j++){//Iterating all the atoms inside a tube
        if(tubes[index].atoms[j]==atomType){//If it hits
                count++;//increment count
        }
    }
    return count;
}
//----------------------------------------END OF FUNCTIONS-----------------------------------------------


//----------------------------------------THREAD FUNCTIONS-----------------------------------------------

//------------------------------PRINT THREAD--------------------------------------
//This thread corresponds to printing message to the terminal when a molecule appears
void * printInfo_f(void * arg){
    //Since I will use the global variable,info,
    //Lock the mutex, mutexInfo.
    pthread_mutex_lock(&mutexInfo);

    while(true){//printing loop
        //If print signal is achieved, then go
        //If not, then wait
        pthread_cond_wait(&printInfo,&mutexInfo);
        //If all terminal output occurs, then break the loop so thread can finish
        if(END_OF_EVERYTHING){break;}
        //According to moleculeType, printing message will differ
        switch(info.mytube.moleculeTYPE){
            case 1:
                printf("H20 is created in tube %d\n",info.tubeID);
                break;
            case 2:
                printf("CO2 is created in tube %d\n",info.tubeID);
                break;
            case 3:
                printf("NO2 is created in tube %d\n",info.tubeID);
                break;
            case 4:
                printf("NH3 is created in tube %d\n",info.tubeID);
                break;
            default:
                printf("SOMETHING IS GONE WRONG\n");
                break;
        }
    //Finish while loop then unlock the mutex
    pthread_mutex_unlock(&mutexInfo);
    }
}
//------------------------------END OF PRINT THREAD-----------------------------------

//------------------------------TUBE THREAD--------------------------------------
//This thread corresponds to update the tube fields(*), if there is a new atom in that tube
//Note *: tubeTS, atoms and atomNumber is updated in assignAtom thread function.
//Note **: There are one thread for each tubes.
void * updateTubes(void * arg){
    //Take argument
    int index=*(int*)arg;
    //Since I will use the global variable,tubes,
    //Lock the mutex, mutex_tubes.
    pthread_mutex_lock(&mutex_tubes);
    while(true){

        //If update tube signal(atomNew) is achieved, then go
        //If not, then wait
        pthread_cond_wait(&atomNew[index],&mutex_tubes);
        //If all terminal output occurs, then break the loop so thread can finish
        if(END_OF_EVERYTHING){
                break;
        }
        //------------------------------------------------------------------------------UPDATING TUBE VARIABLES
        //Each variable corresponds to a specified molecule
        //Assume each molecule can be appeared in that tube initially
        bool possibleH20=true;
        bool possibleC02=true;
        bool possibleNO2=true;
        bool possibleNH3=true;
        //Check if there is a conflicting situation
        for(int i=0;i<tubes[index].atomNumber;i++){//Iterating all atoms inside the tube (until see an empty location)
            switch(tubes[index].atoms[i]){
                case 'C'://If C is in the tube,then H2O NO2 and NH3 can not be appeared
                    possibleH20=false;
                    possibleNO2=false;
                    possibleNH3=false;
                    break;
                case 'H'://If H is in the tube,then CO2 and NO2 can not be appeared
                    possibleC02=false;
                    possibleNO2=false;
                    break;
                case 'N'://If N is in the tube, then CO2 and H2O can not be appeared
                    possibleH20=false;
                    possibleC02=false;
                    break;
                case 'O'://If N is in the tube,then NH3 can not be appeared
                    possibleNH3=false;
                    break;
                default:
                    printf("There is an error in tube update\n");
                    break;
            }//end of switch block
        }//end of for loop
        //Update the tube field, possible molecules
        tubes[index].possibleMolecules[0]=possibleH20;
        tubes[index].possibleMolecules[1]=possibleC02;
        tubes[index].possibleMolecules[2]=possibleNO2;
        tubes[index].possibleMolecules[3]=possibleNH3;
        //Now, we know which molecule can be created in that tube

        //Let's check if given atom is needed for that molecule or not
        //amount variable holds # of atoms in that specified tube
        int amount=0;

        //Initially, assume that none of these atoms can not be inserted
        bool possibleH=false;
        bool possibleC=false;
        bool possibleN=false;
        bool possibleO=false;

        //-------------IS CARBON ADDABLE?-----------
        //Calculate how many C is in that tube
        amount=howManyAtomInTube(index,'C');
        if(possibleC02){//If CO2 can be occured
            if(amount < 1){//Since CO2 requires 1 C atom, if amount is lower than 1, then C can be inserted that tube
                possibleC=true;
            }
        }
        //-------------END OF CARBON CHECK-----------

        //-------------IS HYDROGEN ADDABLE?-----------
        //Calculate how many H is in that tube
        amount=howManyAtomInTube(index,'H');
        if(possibleH20 ){//If H20 can be occured
            if(amount < 2){possibleH=true;}//Since H20 requires 2 H atom, if amount is lower than 2, then H can be inserted that tube
        }
        if(possibleNH3 ){//If NH3 can be occured
            if(amount < 3){possibleH=true;}//Since NH3 requires 3 H atom, if amount is lower than 3, then H can be inserted that tube
        }
        //-------------END OF HYDROGEN CHECK-----------


        //-------------IS NITROGEN ADDABLE?-----------
        //Calculate how many N is in that tube
        amount=howManyAtomInTube(index,'N');
        if(possibleNH3 ){//If NH3 can be occured
            if(amount < 1){possibleN=true;}//Since NH3 requires 1 N atom, if amount is lower than 1, then N can be inserted that tube
        }
        if(possibleNO2 ){//If NO2 can be occured
            if(amount < 1){possibleN=true;}//Since NO2 requires 1 N atom, if amount is lower than 1, then N can be inserted that tube
        }
        //-------------END OF HYDROGEN CHECK-----------


        //-------------IS OXYGEN ADDABLE?-----------
        //Calculate how many O is in that tube
        amount=howManyAtomInTube(index,'O');
        if(possibleNO2 ){//If NO2 can be occured
            if(amount < 2){possibleO=true;}//Since NO2 requires 2 O atom, if amount is lower than 2, then O can be inserted that tube
        }
        if(possibleH20 ){//If H2O can be occured
            if(amount < 1){possibleO=true;}//Since H20 requires 1 O atom, if amount is lower than 1, then O can be inserted that tube
        }
        if(possibleC02 ){//If CO2 can be occured
            if(amount < 2){possibleO=true;}//Since CO2 requires 2 O atom, if amount is lower than 2, then O can be inserted that tube
        }
        //-------------END OF OXYGEN CHECK-----------

        //Update the tube field, atomsCanBeAdded
        tubes[index].atomsCanBeAdded[0]=possibleC;
        tubes[index].atomsCanBeAdded[1]=possibleH;
        tubes[index].atomsCanBeAdded[2]=possibleN;
        tubes[index].atomsCanBeAdded[3]=possibleO;
        //Now, we updated all necessary parameters
        //Let's check if a molecule can be generated
        //------------------------------------------------------------------------------FINISH UPDATING TUBE VARIABLES

        //------------------------------------------------------------------------------CHECK MOLECULE
        //If none of atoms can be added to this tube, then it means we must create molecule (there is no possible space on that module)
        bool anAtomCanAdd=possibleC || possibleH || possibleN || possibleO;
        if(!(anAtomCanAdd) ){//If anAtomCanAdd is false, that means we can not add an atom to that tube == Molecule must be generated
            int molecule=-1;
            for (int j =0; j<NUM_ATOMS;j++){//Iterate all molecule type
                if(tubes[index].possibleMolecules[j]==true){//If that molecule can be created
                        molecule=j;//Hold that value
                        break;//Don't iterate anymore (not necessary, since there can be only one molecule)
                }
            }
            //Now molecule is determined. That means we found that molecule
            //Lock the mutexInfo since we will be update info variable
            pthread_mutex_lock(&mutexInfo);
            //change info
            info.tubeID=index+1;
            info.mytube.moleculeTYPE=molecule+1;
            //Unlock the mutexInfo since it is updated
            pthread_mutex_unlock(&mutexInfo);
            //Since molecule is created,we should clear the tube
            restartTube(index);
            //Now let's wake up the printer thread
            pthread_cond_signal(&printInfo);
	    
        }
        //------------------------------------------------------------------------------FINISH CHECK MOLECULE	
	
    }
    //unlock the mutex since we finished all tasks
    pthread_mutex_unlock(&mutex_tubes);
    //malloc is used so free that space
    free(arg);
}
//------------------------------END OF TUBE THREAD-----------------------------------


//------------------------------ATOM THREAD--------------------------------------
//Deciding which cube will be the generated atom into
//Note *: Each generation of the atom will cause to create this thread and kill it
void * assignAtom(void * arg ){
    //place variable holds the index of tube
    int place=-1;

    //Taking argument,atom, from main thread
    struct atom * atomPtr = (struct atom *) arg;
    //Since we are accessing global variables, cubes, we should lock this mutex
    pthread_mutex_lock(&mutex_tubes);
    //calculating number of possible location in all tubes
    int numberOfLocation=possibleLocations(atomPtr->atomTYPE);
    //If there is no possible location then it is wasted.
    if(numberOfLocation==0){
        pthread_mutex_unlock(&mutex_tubes);
        pthread_exit(NULL);//send NULL to main thread so that it can understand it is wasted
    }
    //If not,
    else{
        double smallestTS=999999999;//Assigning an arbitrary huge number
        //Finding index of atomsCanBeAdded for that atom
        int type=-1;
        switch(atomPtr->atomTYPE){
            case 'C':
                type=0;
                break;
            case 'H':
                type=1;
                break;
            case 'N':
                type=2;
                break;
            case 'O':
                type=3;
                break;
            default:
                break;
        }

        for(int i=0;i<NUM_TUBES;i++){//Iterate all tubes
            if(tubes[i].atomsCanBeAdded[type]==true){//If that atom can be inserted
                if((tubes[i].tubeTS < smallestTS)&&(tubes[i].tubeTS!=0)){//Find the non-empty and highest-prior tube in all tubes
                    smallestTS=tubes[i].tubeTS;
                    place=i;

                }
            }
        }
        //If there is no update from previous for loop
        //Then, it means there is no available non-empty tube
        //All available tubes are empty
        if(place==-1){
            for(int i=0;i<NUM_TUBES;i++){//Iterate all tubes
                if(tubes[i].atomsCanBeAdded[type]==true){//Through iteration, if you see an available tube,
                    place=i;//Take that value
                    break;//kill the iteration so that we can get FIRST EMPTY tube
                }
            }
        }//end if
	    //Update tube fields of tubeTS, atoms and atomNumber
	    
    }//end else 
    placeAtomToTube(place,atomPtr->atomTYPE,atomPtr->atomID);

    //There will be no longer tube accessing
    pthread_mutex_unlock(&mutex_tubes);

    //To give the result, assign a dynamic memmory
    int *result=malloc(sizeof(int));
    *result=place+1;
    pthread_exit((void*)result);


}
//------------------------------END OF ATOM THREAD-----------------------------------

//------------------------------MAIN THREAD--------------------------------------
int main(int argc, char * argv[]){
    //  Taking input from terminal
    // Inspired from gnu.org
    int opt;
    int numberC=20,numberH=20,numberO=20,numberN=20,numberG=100;

    while((opt=getopt(argc,argv,":c:h:o:n:g:"))!=-1)
    {
        switch(opt)
        {
        case 'c':
            numberC=atoi(optarg);
            break;
        case 'h':
            numberH=atoi(optarg);
            break;
        case 'o':
            numberO=atoi(optarg);
            break;
        case 'n':
            numberN=atoi(optarg);
            break;
        case 'g':
            numberG=atoi(optarg);
            break;
        default :
            break;
        }
    }

    int numberTotalAtoms = numberC+numberH+numberO+numberN;//Total # of atoms

    //Necessary initialization
    bool conditionGenerate=true;//This will check if generation number is exceed or not
    double waitTime;            //After each generation it will sleep that much time
    int selectionAtom;          // 0: create C ; 1: create H ; 2: create O ; 3: create N
    srand(time(NULL));          // Random generator seed is set to NULL, so that each calling rand function will result in different sequence
    int mAtomID=0;              // atomID
    int *res;                   // Taking Result

    //info initializer
    info.tubeID=0;
    info.mytube.tubeID=0;
    info.mytube.tubeTS=0;
    info.mytube.moleculeTYPE=0;
    info.mytube.atomNumber=0;

    //Necessary initialization for PTHREADS
    pthread_t berkay[numberTotalAtoms];                               //ATOM ASSIGN THREAD
    //TODO patlak olaiblir burası update gerekebilir eve geçince dene

    pthread_mutex_init(&mutex_tubes,NULL);          //init mutex

    pthread_cond_init(&atomNew[0],NULL);            //init cond
    pthread_cond_init(&atomNew[1],NULL);            //init cond
    pthread_cond_init(&atomNew[2],NULL);            //init cond
    pthread_cond_init(&printInfo,NULL);             //init cond
    //pthread_cond_init(&updateTube_c,NULL);             //init cond

    pthread_create(&printInfo_t,NULL,&printInfo_f,NULL);//Printer thread is created(it will wait)

    //To create a thread for each tube
    for(int i=0;i<NUM_TUBES;i++){
        // To assign the tube ID, we should pass argument
        int *a=malloc(sizeof(int));//create dynamically
        *a=i;
        pthread_create(&tubesUpdate[i],NULL,&updateTubes,a);    //Free it with free(arg) inside in the tube thread
    }

    //Initialize tubes
    initializeTubes();


    //GENERATE ATOM AND CALL OTHER THREADS WHEN NECESSARY
    for(int i=0; i<numberTotalAtoms;i++)//Iterate as
    {
        waitTime=((double)rand() / (RAND_MAX)); // create random number between 0 and 1
        waitTime= -log(1-waitTime) / numberG;   // exp dist function to calculate wait time
        conditionGenerate=true;                 // Reset the flag after each generation

        struct atom nextAtom;                   // atom that will be generated
        mAtomID++;                              // Increment atom ID
        while(conditionGenerate){
            selectionAtom=(rand() % 4);         // create random number between 0 and 3
            switch(selectionAtom){
                case 0:                         //If it is C,
                    if(numberC==0){break;}      //If C is no longer need in total, do not set the flag
                    else{
                        nextAtom.atomTYPE='C';  //generate atom
                        numberC--;              //decrement the number of C in total
                        }
                    conditionGenerate=false;     //set the flag so that break that loop then assign it to a tube
                    break;
                //Other cases are made within same idea of Carbon

                case 1:
                    if(numberH==0){break;}
                    else{
                        nextAtom.atomTYPE='H';
                        numberH--;
                        }
                    conditionGenerate=false;
                    break;
                case 2:
                    if(numberO==0){break;}
                    else{
                        nextAtom.atomTYPE='O';
                        numberO--;
                        }
                    conditionGenerate=false;
                    break;
                case 3:
                    if(numberN==0){break;}
                    else{
                        nextAtom.atomTYPE='N';
                        numberN--;

                        }
                    conditionGenerate=false;
                    break;
            }
            nextAtom.atomID=mAtomID;//Assign atom nubmer


        }
	
        pthread_create(&berkay[i],NULL,&assignAtom,&nextAtom);//Create that assigner atom thread
        
        //waits until assigning an atom is finished
        pthread_join(berkay[i],(void**)&res);//Wait finish
	sleep(waitTime);
        printf("%c with ID:%d is created.\n",nextAtom.atomTYPE,nextAtom.atomID);//atom generated message print
        if(res == NULL){
		printf("%c with ID:%d wasted.\n",nextAtom.atomTYPE,nextAtom.atomID);
	}//atom wasted message print
        else{
		pthread_cond_signal(&atomNew[(*res)-1]);		
	}
    }
    //If whole process is ended, change the terminater variable to true
    END_OF_EVERYTHING=true;
    // Call the threads for the last time so that they can finish their execution
    pthread_cond_signal(&atomNew[0]);
    pthread_cond_signal(&atomNew[1]);
    pthread_cond_signal(&atomNew[2]);
    pthread_cond_signal(&printInfo);
    //Be sure they are ended
    pthread_join(printInfo_t,NULL);
    for(int i=0;i<NUM_TUBES;i++){
        pthread_join(tubesUpdate[i],NULL);
    }
    //DESTROYING PTHREAD OBJECTS
    pthread_mutex_destroy(&mutex_tubes);
    pthread_cond_destroy(&atomNew[0]);
    pthread_cond_destroy(&atomNew[1]);
    pthread_cond_destroy(&atomNew[2]);
    //Free the allocated dynamic memory
    free(res);
    return 0;

}
//------------------------------END OF MAIN THREAD-----------------------------------
