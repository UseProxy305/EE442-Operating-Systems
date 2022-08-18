//Necessary libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//Defining preprocess definitions
#define LENGTH_OF_FAT 4096
#define LENGTH_OF_FILE_LIST 128
#define LENGTH_OF_DATA 512
#define LENGTH_OF_DATA_LIST 4096

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------STRUCTURES(ALREADY DEFINED IN THE HW)
typedef struct{
	unsigned int value;
}FATRow;
typedef struct{
	FATRow list[LENGTH_OF_FAT];
}FAT;

//----------------------------------------------------------------------
typedef struct{
	char fileName[248];
	unsigned int fileBlock;
	unsigned int fileSize;
}FileListRow;
typedef struct{
	FileListRow list[LENGTH_OF_FILE_LIST];
}fileList;

//----------------------------------------------------------------------
typedef struct{
	char data[LENGTH_OF_DATA];
}dataRow;
typedef struct{
	dataRow list[LENGTH_OF_DATA_LIST];
}dataList;

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------FUNCTIONS(ALREADY DECLEARED IN THE HW)
void Format();
void Write(char * srcPath, char * destFileName);
void Read(char * srcFileName, char *destPath);
void Delete(char * fileName);
void PrintFat();
void PrintFileList();
void List();
void Defragment();

//--------------------------------------------------------------------------------HELPER FUNCTIONS (I DEFINED)

int FindEmptySpaceFAT(FAT * fatptr,int starting);//EMPTY SLOT FINDER FOR FAT (point to start iteration can be arranged)
int FindEmptySpaceFileList(fileList * fileListptr);//EMPTY SLOT FINDER FOR FILE LIST 
int BigToLittle(int input);//Little endian from(to) big endian converter
 
//------------------------------------------------------------------------------------------GLOBAL VARIABLES
FILE *fileptr;//To open disk, file pointer
char * folderLocation;//in the main function, we will assign it with second argument

//---------------------------------------------------------------------------------------------MAIN FUNCTION
int main(int argc, char *argv[]){
	folderLocation=argv[1];//Assign the disk formation
	if(strcmp(argv[2],"-format")==0){//Format()
		Format();
	}
	else if (strcmp(argv[2],"-write")==0){//Write()
		Write(argv[3],argv[4]);
}

	else if (strcmp(argv[2],"-read")==0){//Read()
		Read(argv[3],argv[4]);
}

	else if (strcmp(argv[2],"-delete")==0){//Delete()
		Delete(argv[3]);
}

	else if (strcmp(argv[2],"-list")==0){//List()
		List();
}

	else if (strcmp(argv[2],"-printfilelist")==0){//PrintFileList()
		PrintFileList();		
}

	else if (strcmp(argv[2],"-printfat")==0){//PrintFat()
		PrintFat();
}

	else if (strcmp(argv[2],"-defragment")==0){//Defragment()
		Defragment();
}
	else
		printf("UNKOWN COMMENT..\n");//Unkown comment is entered
	return 0;

}

//---------------------------------------------------------------------------------------------FORMAT
void Format(){
	
	fileptr=fopen(folderLocation,"w+");//Open the folder that is stored in the global variable
	FAT biFAT;//Create an initial FAT struct
	biFAT.list[0].value=0xFFFFFFFF;//Initial value is 0xFFFFFFFF defined in the HW
	memset(&biFAT,0,sizeof(FAT));//Memory Allocation (fill with zeros)
	biFAT.list[0].value=0xFFFFFFFF;//Initial value is 0xFFFFFFFF defined in the HW	
	for(int i=0;i<LENGTH_OF_FAT;i++)
		fwrite(&biFAT.list[i],sizeof(FATRow),1,fileptr);//Write it through temporary FAT

	fileList biFileList;//Create an initial File List struct
	memset(&biFileList.list[0].fileName[0],0,sizeof(biFileList.list[0].fileName));//Memory Allocation File Name(fill with zeros)
	biFileList.list[0].fileSize=0x00000000;//Initial value is 0 defined in the HW
	biFileList.list[0].fileBlock=0x00000000;//Initial value is 0 defined in the HW
	biFileList.list[0].fileName[0]='\0';//Initial value is \0 defined in the HW
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++)
		fwrite(&biFileList.list[0],sizeof(FileListRow),1,fileptr);//Write it through temporary File List
	

	dataList biDataList;//Create an initial File List struct
	memset(&biDataList,0,sizeof(biDataList));//Memory Allocation (fill with zeros)
	fwrite(&biDataList,sizeof(dataList),1,fileptr);//Write it through temporary Data List

	//Close the file
	fclose(fileptr);
}


//---------------------------------------------------------------------------------------------WRITE
void Write(char * srcPath, char * destFileName){

	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	dataList * tempDataList=malloc(sizeof(dataList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	fread(tempDataList,sizeof(dataList),1,fileptr);


	//Find the empty spot with the lowest having address (From the starting point)
	int emptySpaceFAT = FindEmptySpaceFAT(tempFAT,0);
	int firstSpace=emptySpaceFAT;//Record the first block location for File List
	if(emptySpaceFAT == 0){printf("[ERROR-1] There is no possible location in the FAT disk...\n"); return;}
	int emptySpaceFileList= FindEmptySpaceFileList(tempFileList);//Try to find an empty space in the File List
	if(emptySpaceFileList == -1){printf("[ERROR-2] There is no possible location in the file list disk...\n"); return;}

	FILE * fileptrR =fopen(srcPath,"r");//For Direct Usage
	FILE * fileptrL= fopen(srcPath,"r");//To find the size of file 
	
	size_t lenTotal,len;
	fseek(fileptrL,0L,SEEK_END);//Go to end of file
	lenTotal = ftell(fileptrL);//Tell where it is and found out the size of file
	long int lenBerkay = lenTotal;//Record the file size for File List
	fclose(fileptrL);//Close the temporary file open
	if(fileptrR==NULL){printf("[ERROR-3] There is no such file name...\n"); return;}
	
	dataRow berkay;//Temporary data buffer 


	//---------UPDATE FAT
	int nextPlace;//Calculate the next possible empty position in the FAT
	while(1){
		if(lenTotal<LENGTH_OF_DATA){
			len=fread(&berkay,1,lenTotal,fileptrR);
			//This will be called when length of data to be written is lower than 512
			//I am reading with the remaining lenTotal 
		}
		else{
			len=fread(&berkay,1,LENGTH_OF_DATA,fileptrR);
			//Directly Read 512 bytes
		}
		lenTotal-=len;//Update the remaning data to be written
		if(len<LENGTH_OF_DATA){//If it is the final part
			tempDataList->list[emptySpaceFAT]=berkay;//Write it through temp data list
			tempFAT->list[emptySpaceFAT].value=0xFFFFFFFF;//Last position so put 0xFFFFFFFF
			break;//Break the loop

		}
		tempDataList->list[emptySpaceFAT]=berkay;//Write it through temp data list
		nextPlace= FindEmptySpaceFAT(tempFAT,emptySpaceFAT+1);//Find an empty space for the coming data
		//Write the next place to FAT with converting it to little-endian
		tempFAT->list[emptySpaceFAT].value=BigToLittle(nextPlace);

		emptySpaceFAT=nextPlace;//Go to next iteration
		
	}



	//---------UPDATE FILE LIST
	int i=0;
	for(;i<248;i++){
		if(destFileName[i]=='\0'){break;}//If file name is finished
		tempFileList->list[emptySpaceFileList].fileName[i]=destFileName[i];//Assign it one char by one char
	}
	tempFileList->list[emptySpaceFileList].fileName[i]='\0';//Put the finisher character
	tempFileList->list[emptySpaceFileList].fileSize=lenBerkay;//Put the size of file
	tempFileList->list[emptySpaceFileList].fileBlock=firstSpace;//Put the first block


	//FINISHING STATE
	//WRITE THE UPDATED TEMP STRUCTURES TO DISK IMAGE
	FILE * fileptrW=fopen(folderLocation,"w+");
	fseek(fileptrW,0,SEEK_SET);
	fwrite(tempFAT,sizeof(FAT),1,fileptrW);

	fwrite(tempFileList,sizeof(fileList),1,fileptrW);
	fwrite(tempDataList,sizeof(dataList),1,fileptrW);
	//CLOSE THE FOLDERS
	fclose(fileptrW);
	fclose(fileptr);
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFAT);
	free(tempFileList);
	free(tempDataList);

}
//---------------------------------------------------------------------------------------------READ

void Read(char * srcFileName, char *destPath){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	dataList * tempDataList=malloc(sizeof(dataList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	fread(tempDataList,sizeof(dataList),1,fileptr);
	
	//FIND THE FILE IN THE FILE LIST
	int isFind=-1;
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++){
		if(strcmp(tempFileList->list[i].fileName,srcFileName)==0){//COMPARE NAMES
			isFind=i;//Found it
			break;
		}
	}
	if(isFind==-1){printf("file not found\n");return;}
	
	FILE * fileptrW= fopen(destPath,"w+");//Destination path
	size_t len;//Holds the how many bytes are written succesfully	
	int indexBlock;//next location in the fat

	size_t aroma;//Current Size of Data to be written (for remainder part) 
	aroma=tempFileList->list[isFind].fileSize;//Assign it as File List
	indexBlock=tempFileList->list[isFind].fileBlock;//Assign it as first location of the file
	while(1){
		if(aroma<LENGTH_OF_DATA){
			len = fwrite(&tempDataList->list[indexBlock],aroma,1,fileptrW);//Write the just reamining part
			break;//Break the loop 
		}
		else{
			len = fwrite(&tempDataList->list[indexBlock],LENGTH_OF_DATA,1,fileptrW);//Directly write it 
		}
		

		indexBlock=BigToLittle(tempFAT->list[indexBlock].value);//Update the current index
		aroma-=LENGTH_OF_DATA;//Current Size of Data to be written (for remainder part)  
		if(tempFAT->list[indexBlock].value == 0xFFFFFFFF){//Find the final spot
			fwrite(&tempDataList->list[indexBlock],aroma,1,fileptrW);//Last print and break
			break;			
		}
	}

	

	//FINISHING STATE
	//CLOSE THE FOLDERS
	fclose(fileptr);
	fclose(fileptrW);
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFAT);
	free(tempFileList);
	free(tempDataList);
}
//---------------------------------------------------------------------------------------------DELETE
void Delete(char * fileName){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	dataList * tempDataList=malloc(sizeof(dataList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	fread(tempDataList,sizeof(dataList),1,fileptr);
	//FIND THE FILE IN THE FILE LIST
	int isFind=-1;
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++){
		if(strcmp(tempFileList->list[i].fileName,fileName)==0){
			isFind=i;
			break;
		}
	}
	if(isFind==-1){printf("file not found\n");return;}
	int indexBlock;
	indexBlock=tempFileList->list[isFind].fileBlock;
	//Clear the file list
	tempFileList->list[isFind].fileName[0]='\0';
	tempFileList->list[isFind].fileSize=0;
	tempFileList->list[isFind].fileBlock=0;
	//Clear the FAT
	int temp;
	while(tempFAT->list[indexBlock].value != 0xFFFFFFFF){//Do it until you see final block
		temp=indexBlock;//Hold the current address
		indexBlock=BigToLittle(tempFAT->list[indexBlock].value);//Update the current address
			//Called converter since table is filled with little endian 
		tempFAT->list[temp].value = 0x00000000;//Empty Location
	}
	tempFAT->list[indexBlock].value=0x00000000;//Clear the 0xFFFFFFF part also

	//FINISHING STATE
	//WRITE THE UPDATED TEMP STRUCTURES TO DISK IMAGE
	FILE * fileptrW=fopen(folderLocation,"w+");
	fseek(fileptrW,0,SEEK_SET);
	fwrite(tempFAT,sizeof(FAT),1,fileptrW);
	fwrite(tempFileList,sizeof(fileList),1,fileptrW);
	fwrite(tempDataList,sizeof(dataList),1,fileptrW);
	
	//CLOSE THE FOLDERS	
	fclose(fileptrW);
	fclose(fileptr);
	
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFAT);
	free(tempFileList);
	free(tempDataList);
	
}
//---------------------------------------------------------------------------------------------LIST
void List(){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	//PRINTING FORMAT IS NOT CORRECT I KNOW THAT BUT IT IS UNDERSTABLE	
	//Initial printing
	printf("file name\tfile size\n");
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++){
		if(tempFileList->list[i].fileBlock !=0){//If first block is not equal to zero,there is a file
			printf("%s\t\t%d\n",tempFileList->list[i].fileName,tempFileList->list[i].fileSize);
			//Print the spotted file
		}
		else{

		}


	}
	//FINISHING STATE
	//CLOSE THE FOLDERS
	fclose(fileptr);	
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFileList);
	free(tempFAT);
}
//---------------------------------------------------------------------------------------------PRINT THE FILE LIST
void PrintFileList(){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	//Open filelist.txt
	FILE * fileptrW=fopen("filelist.txt","w+");
	//Initial printing
	fprintf(fileptrW,"Item\tFile Name\tFirst Block\tFile Size (Bytes)\n");
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++){
		if(tempFileList->list[i].fileBlock==0){//If it is empty, then name is no more needed
			fprintf(fileptrW,"%03d\t\"\"\t\t%d\t\t%d\n",i,tempFileList->list[i].fileBlock,tempFileList->list[i].fileSize);
		}
		else{//If there is a file, print the filename also.
			fprintf(fileptrW,"%03d\t\"%s\"\t\t%d\t\t%d\n",i,tempFileList->list[i].fileName,tempFileList->list[i].fileBlock,tempFileList->list[i].fileSize);
		}


	}
	//FINISHING STATE
	//CLOSE THE FOLDERS
	fclose(fileptr);
	fclose(fileptrW);
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFileList);
	free(tempFAT);
}
//---------------------------------------------------------------------------------------------PRINT FAT
void PrintFat(){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	//Open FAT.txt
	FILE * fileptrW=fopen("fat.txt","w+");
	//Initial printing
	fprintf(fileptrW,"Entry\tValue\t\tEntry\tValue\t\tEntry\tValue\t\tEntry\tValue\n");
	
	for(int i=0;i<LENGTH_OF_FAT;i++){
		fprintf(fileptrW,"%04d\t%08X\t",i,tempFAT->list[i].value);//Print the value for each entry
		if(i%4==3){fprintf(fileptrW,"\n");}//Each 4 block put a new enter
	}
	//FINISHING STATE
	//CLOSE THE FOLDERS
	fclose(fileptr);
	fclose(fileptrW);
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempFAT);
} 


//-------------------------------------------------------------------------------------------------------------
void Defragment(){
	//READ ALL STRUCTURES FROM DISK 
	FAT * tempFAT=malloc(sizeof(FAT));
	fileList * tempFileList=malloc(sizeof(fileList));
	dataList * tempDataList=malloc(sizeof(dataList));
	fileptr=fopen(folderLocation,"r");
	fread(tempFAT,sizeof(FAT),1,fileptr);
	fread(tempFileList,sizeof(fileList),1,fileptr);
	fread(tempDataList,sizeof(dataList),1,fileptr);
	//Open a temporary file to hold the data
	FILE * fileptrTemp=fopen("temp.txt","w+");
	char * temp;
	for(int i=0;i<LENGTH_OF_FILE_LIST;i++){
		if(tempFileList->list[i].fileSize !=0){
			//printf("Reading %s to temp.txt file\n",tempFileList->list[i].fileName);
			Read(tempFileList->list[i].fileName,"temp.txt");
			//printf("Deleting %s",tempFileList->list[i].fileName);
			temp=tempFileList->list[i].fileName;
			Delete(tempFileList->list[i].fileName);
			Write("temp.txt",temp);
		}
		else{

		}


	}
	//FINISHING STATE
	//WRITE THE UPDATED TEMP STRUCTURES TO DISK IMAGE
	FILE * fileptrW=fopen(folderLocation,"w+");
	fseek(fileptrW,0,SEEK_SET);
	fwrite(tempFAT,sizeof(FAT),1,fileptrW);
	fwrite(tempFileList,sizeof(fileList),1,fileptrW);
	fwrite(tempDataList,sizeof(dataList),1,fileptrW);
	//FREE THE DYNAMICALLY ALLOCATED MEMORY
	free(tempDataList);
	free(tempFileList);
	free(tempFAT);

}
//-------------------------------------------------------------------------------------------------------------
int FindEmptySpaceFAT(FAT * fatptr,int starting){//From starting point, iterate and find the first empty location in the FAT
	int i=starting;
	int emptySpace=0;
	for(;i<LENGTH_OF_FAT;i++){
		if(fatptr->list[i].value== 0x00000000){//Empty Location
			emptySpace=i;//Found it
			break;//Break it so that we can guarentee it as the first one
		}
	}
	return emptySpace;//It will be return -1 when there is no location
}

int FindEmptySpaceFileList(fileList * fileListptr){//Iterate and find the first empty location in the file list
	int i=0;
	int emptySpace=-1;
	for(;i<LENGTH_OF_FAT;i++){
		if(fileListptr->list[i].fileBlock==0){//Empty Location
			emptySpace=i;//Found it
			break;//Break it so that we can guarentee it as the first one
		}
	}
	return emptySpace;//It will be return -1 when there is no location
}
//https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
int BigToLittle(int input){
	int swapped = ((input>>24)&0xff) | // move byte 3 to byte 0
		            ((input<<8)&0xff0000) | // move byte 1 to byte 2
		            ((input>>8)&0xff00) | // move byte 2 to byte 1
		            ((input<<24)&0xff000000); // byte 0 to byte 3
	return swapped;
}
