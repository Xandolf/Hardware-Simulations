/* Created 11-24-2020 By Xander Winans
 * -- Program Description --
 * This program is a simulation of the directory-based cache coherence protocol used in the cc-NUMA (DASH) machine.
 * The system contains 4 MIPS-based SMP Nodes, a more detailed description of the Node can be found further below.
 * .... In brief each node has:
 * ........ 2 scalar processors with two registers and a local cache each
 * ........ 1 memory module
 * ........ 1 directory
 * The system has 64 words of globally addressed memory which evenly distributed across all 4 nodes (16 words/Node)
 * The cache system is direct mapped and uses WB when write hit and no-write allocate on write miss
 *
 * This program will take an input file containing machine code ("machine_code.txt") with load and store instructions.
 * After performing the instructions (8 in the test case) it will output the contents of each Node's contents
 * .... (registers, cache, memory, & directory)
 *
 *
 * -- Running the program --
 * To run the program make sure that both this file and machine_code.txt are in the same directory.
 * Compile this program using a c++ compiler. The following steps are for a linux machine, some steps may be different depending on your system/
 * Execute the following instructions in while inside the correct directory
 *      $> g++ main.cpp -o XanderIsCool
 *      $> ./XanderIsCool
 *
 */
//TODO Format output
// TODO Count clocks
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
 fstream inFile ("machine_code.txt"); //global fstream for file input
struct Node;
Node* systemNodes [4];
int clockCount;

// Function Declarations
void fetch();
void decode(string line);
void execute(int nodeIndex, int cpuIndex, string opCode, int rs, int rt, int byteOffset);
void memoryAccess(int nodeIndex, int cpuIndex, const string& opCode, int memoryAddress, int reg);
void writeToMem(int nodeIndex, int cpuIndex, const  string& opCode, int memoryAddress, int reg);
void writeBack(int nodeIndex, int cpuIndex, int cacheIndex);

int binaryToDecimal(string binaryNumber)
{
    int size = binaryNumber.size();
    int result = 0;
    for(int i = 0; i<size; i++)
    {
        result*=2;
        if(binaryNumber[i]=='1')
            result++;
    }
    return result;
}

//this function takes in an integer value and returns a 32 bit binary number as a boolean array
bool * decimalTo32BitBinary(int decimalValue)
{
    static bool binaryNumber [32];
    for(int i = 31; i>=0; i--)
    {
        binaryNumber[i] = decimalValue % 2 == 1;
        decimalValue/=2;
    }
    return binaryNumber;
}

/*
 * -- Detailed description of a Node --
 * This is a Node class that's purpose is to simulate a MIPS based SMP Node
 * The system contains 4 of these nodes each of which consists of the following components:
 * 2 scalar processors each with a local cache (4 lines/cache, 1 word/line, 32 bits/word + valid bit + tag field)
 * .....each processor also has 2 registers (1 words/reg) each
 * .....Cache is direct-mapped and uses WB when write hit and no-write-allocate when write miss;
 *
 * 1 memory module (16 words),
 * .....Memory is globally addressed and the total memory size in the system is 64 words (16 words/node);
 * .....Physical address is 6 bits (2 bits for index, 4 bits for tag),
 * .....and the memory address is word address and we ignore byte level addressing.
 *
 *  1 directory (6 bits/entry)
 * .... Each directory also consists of 16 entries, one for each line (1 word) in the node memory.
 * .... Bits [0,1] are for the status of the respective memory location
 * ........ 00 - uncached
 * ........ 01 - shared
 * ........ 11 - dirty
 * .... Bits [2,3,4,5] are to indicate which nodes ([0,1,2,3] respectively) have the memory in their cache
 */
struct Node{
public:
    bool registers [2][2][32];  // Each node also has 2 processors
                                // Each processor has 2 registers (word size each)
                                // Each register has 32 bits

    bool caches [2][4][37];     // Each processor has a cache, there are 2 processors
                                // each cache has 4 lines
                                // each line has (1 valid bit + 4 bits tag field + 32 bit word for data)

    bool memory [16][32];       // Each node has 16 words of memory
    bool directory [16][6];     // Each memory location has 6 bits of status information
                                // .... bits 0-1 are for state, bits 2-6 are to indicate which nodes contain the data
};


/*
 * The fetch function access the input file (simulated instruction memory) and retrieves the next instruction.
 * If there are no more instructions it terminates
 * If an instruction is successfully fetched then it will move to decode the instruction
*/
 void fetch()
{
    string line;
    if(getline( inFile, line ))
    {
        decode(line);
    }
}

/* The decode function takes a machine instruction as input and decodes it in preparation for execution.
 * This includes separating the parts of the instruction into the correct fields
 * Decode also converts the parts of the instruction from binary to decimal to allow easier computation
 * .... In an actual system all arithmetic would be done in binary.
 * All instructions have the same length and follow this pattern
 * .... [N,N,C, :,(space), OP,OP,OP,OP,OP,OP, rs,rs,rs,rs,rs, rt,rt,rt,rt,rt, b,b,b,b,b,b,b,b,b,b,b,b,b,b,b,b]
 * .... [0,1,2, 3,  4    ,  5, 6, 7, 8, 9,10, 11,12,13,14,15, 16,17,18,19,20, 21,22,23,..............34,35,36]
 * .... Where N = Node Index
 * .........  C = CPU Index
 * .........  OP = Op Code
 * .........  rs = rs field
 * .........  rt = rt field
 * .........  b = Byte Offset
*/
void decode(string line)
{
    //The first two bits indicate which node number the instruction is for
    int nodeIndex = 0;
    if (line[0] == '1')
        nodeIndex += 2;
    if (line[1] == '1')
        nodeIndex += 1;

    //The third bit indicates what cpu the instruction is for
   int cpuIndex = 0;
   if(line[2] == '1')
       cpuIndex = 1;

   //There are two spaces in our input string that need to be ignored. These are at indexes 3 & 4

   // There are 6 bits for the opcode located at index 5
   string opCode = line.substr(5,6);

   // The rs field has 5 bits starting at index 11
   int rs = binaryToDecimal(line.substr(11,5));
   // The rt field has 5 bits starting at index 16
   int rt = binaryToDecimal(line.substr(16,5));

   //The byte offset is 16 bits long starting at index 21
   int byteOffset = binaryToDecimal(line.substr(21,16));

   execute(nodeIndex,cpuIndex,opCode,rs,rt,byteOffset);
}

// The execute function performs the ALU arithmetic needed to properly execute the instruction.
// For load/store instructions this includes adding the word offset to the base address
void execute(int nodeIndex, int cpuIndex, string opCode, int rs, int rt, int byteOffset)
{
    // shift right 2 bits to get word offset........ (I wasn't sure if shifting should be in decode or execute stage)
    int wordOffset = byteOffset/4;
    int memoryAddress = rs + wordOffset;
    // each instruction we will be loading to or storing from a register
    // if the rt field is odd then it is $s1 register
    // if the rt field is even then it is $s2 register
    int reg = (rt+1) % 2;
    memoryAccess(nodeIndex, cpuIndex, opCode, memoryAddress, reg);
}
/*
 * The memoryAccess function is responsible for retrieving the correct value from memory
 * First: The function will check the local processors cache in search of the value in the memory address
 * .... making sure that the valid bit is 1. If found load it into register and done (one clock)
 * .... if not found go to next step
 * Second: Check other cache in local node, if found load it into local cache & reg then done (30 clocks)
 * .... if not found go to next step
 * Third: Search home node's memory/directory. If 'uncached' or 'shared' then load it into local cache/reg (100 clocks)
 * .... if not found go to next step
 * Fourth: Search all caches in the "dirty" node. (use MOD 4 for cache address)
 * .... When found perform necessary operations (135 clocks)
 * .... go to dirty node and find valid value in cache
 * .... share write-back to home
 * .... dirty -> shared
 * .... load into local cache/reg
 *** In all the above steps manage the directories and cache invalid/valid bits correctly ***
*/
void memoryAccess(int nodeIndex, int cpuIndex, const  string& opCode, int memoryAddress, int reg)
{
    // 10011 is the op code for load word
    if (opCode == "100011")
    {
        //compute the cache index and the tag
        // I convert the tag to a bool array for easy comparison
        int cacheIndex = memoryAddress % 4;
        int iTag = memoryAddress / 4;
        bool tag [4];
        tag [3] = iTag % 2 == 1;
        iTag /= 2;
        tag [2] = iTag % 2 == 1;
        iTag /= 2;
        tag [1] = iTag % 2 == 1;
        iTag /= 2;
        tag [0] = iTag % 2 == 1;

        if  (systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][0]            &&   // valid bit is true
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][1] == tag[0]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][2] == tag[1]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][3] == tag[2]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][4] == tag[3]       //check tag field is the same
            )
        { // Case 1: Valid copy found in local cache
            //Copy cached value into register
            clockCount++;
            for(int i =0; i<32; i++)
                systemNodes[nodeIndex]->registers[cpuIndex][reg][i] = systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i+4];
        }//end if case 1
        else //else not case 1
        {
            //since not found in local cache write back current cache contents before loading new value
            writeBack(nodeIndex,cpuIndex,cacheIndex);

            //Check sister processors cache
            int otherCPU = (cpuIndex + 1) % 2;    //There are two cpus so add one mod 2 will get the other one
            if  (systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][0]            &&   // valid bit is true
                 systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][1] == tag[0]  &&   //check tag field is the same
                 systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][2] == tag[1]  &&   //check tag field is the same
                 systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][3] == tag[2]  &&   //check tag field is the same
                 systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][4] == tag[3]       //check tag field is the same
                )
            { //Case 2 valid copy found in sister cache
                clockCount+=30;
                for(int i =0; i<32; i++)
                {
                    systemNodes[nodeIndex]->registers[cpuIndex][reg][i] =
                            systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][i + 5]; //Copy value into local register

                    systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i + 5] =       //Copy value into local cache
                            systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][i + 5];
                }


                for( int i =0; i<4; i++) //this loop copies the valid bit and tag field into local cache from sister cache
                {
                    systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i] =
                            systemNodes[nodeIndex]->caches[otherCPU][cacheIndex][i];
                }
            } //end if case 2
            else //else not case 2
            {
                int homeNode = memoryAddress / 16;
                int localMemIndex = memoryAddress % 16;
                if ( //uncached or shared
                     ( systemNodes[homeNode]->directory[localMemIndex][0] == 0 &&
                       systemNodes[homeNode]->directory[localMemIndex][1] == 0
                     )  // 00 is uncached
                  || ( systemNodes[homeNode]->directory[localMemIndex][0] == 0 &&
                       systemNodes[homeNode]->directory[localMemIndex][1] == 1
                     ) // 01 is shared
                  )
                {// if case 3, copy from home node (uncached or shared)
                    clockCount+=100;
                    for(int i = 0; i<32; i++)
                    {
                        systemNodes[nodeIndex]->registers[cpuIndex][reg][i] =
                                systemNodes[homeNode]->memory[localMemIndex][i];            //Copy value into local register

                        systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i + 5] =       //Copy value into local cache
                                systemNodes[homeNode]->memory[localMemIndex][i];
                    }
                    systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][0] = true;         //set local cache to valid

                    for(int i = 0; i<4; i++)
                        systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i+1] = tag[i];       //Copy tag field

                    //set directory to shared
                    systemNodes[homeNode]->directory[localMemIndex][0] = false;
                    systemNodes[homeNode]->directory[localMemIndex][1] = true;
                    systemNodes[homeNode]->directory[localMemIndex][2+nodeIndex] = true;

                }//end if case 3

                else //case 4
                {
                    clockCount+=135;
                    //Find which node contains the dirty data
                    int dirtyNode = -1;
                    for(int i = 0; i<4 && dirtyNode<0; i++)
                    {
                        if(systemNodes[homeNode]->directory[localMemIndex][2+i])
                            dirtyNode = i;
                    }

                    //Find what CPU in the dirty node contains the data
                    int dirtyCPU = -1;
                    if ( systemNodes[dirtyNode]->caches[0][cacheIndex][0] &&
                        systemNodes[dirtyNode]->caches[0][cacheIndex][1] == tag[0]  &&   //check tag field is the same
                        systemNodes[dirtyNode]->caches[0][cacheIndex][2] == tag[1]  &&   //check tag field is the same
                        systemNodes[dirtyNode]->caches[0][cacheIndex][3] == tag[2]  &&   //check tag field is the same
                        systemNodes[dirtyNode]->caches[0][cacheIndex][4] == tag[3]       //check tag field is the same
                       )
                    {
                        dirtyCPU = 0;
                    }
                    else if ( systemNodes[dirtyNode]->caches[1][cacheIndex][0] &&
                             systemNodes[dirtyNode]->caches[1][cacheIndex][1] == tag[0]  &&   //check tag field is the same
                             systemNodes[dirtyNode]->caches[1][cacheIndex][2] == tag[1]  &&   //check tag field is the same
                             systemNodes[dirtyNode]->caches[1][cacheIndex][3] == tag[2]  &&   //check tag field is the same
                             systemNodes[dirtyNode]->caches[1][cacheIndex][4] == tag[3]       //check tag field is the same
                            )
                    {
                        dirtyCPU = 1;
                    }

                    for(int i =0; i<32; i++)
                    {
                        systemNodes[homeNode]->memory[localMemIndex][i] =                   // WB to home
                                systemNodes[nodeIndex]->registers[cpuIndex][reg][i] =       // load value in local reg
                                systemNodes[homeNode]->caches[cpuIndex][cacheIndex][i+5] =  // load value into local cache
                                systemNodes[dirtyNode]->caches[dirtyCPU][cacheIndex][i+5];  //value to load
                    }

                    //set local cache valid and tag fields
                    systemNodes[homeNode]->caches[cpuIndex][cacheIndex][0] = true;      // set local cache to valid
                    systemNodes[homeNode] ->caches[cpuIndex][cacheIndex][1] = tag[0];   //tag field
                    systemNodes[homeNode] ->caches[cpuIndex][cacheIndex][2] = tag[1];
                    systemNodes[homeNode] ->caches[cpuIndex][cacheIndex][3] = tag[2];
                    systemNodes[homeNode] ->caches[cpuIndex][cacheIndex][4] = tag[3];

                    //Set to shared
                    systemNodes[homeNode]->directory[localMemIndex][0] = false;
                    systemNodes[homeNode]->directory[localMemIndex][1] = true;
                    //Indicate that current cache has the memory value
                    systemNodes[homeNode]->directory[localMemIndex][2+homeNode] = true;
                }//end else case 4
            } //end else not case 2
        } //end else not case 1
    }
    if(opCode=="101011")
    {
        writeToMem(nodeIndex, cpuIndex, opCode, memoryAddress, reg);
    }
}

/* writeToMem will write the value in a given register to a given memory position
 * 1: Search local cache (local to each processor) (check valid bit and tag)
 * .... if found get exclusive access to it:
 * ........ using home directory invalidate all others that are shared
 * ........ update value in (local cache?) with contents of register (1 clock)
 * ........ home directory is dirty
 * .... else go to step 2
 * 2: Update home node memory with the content of the register (100 clocks)
 * .... if directory indicates "uncached" -> "uncached"
 * ........................... "shared"   -> "shared", but invalidate all shared cached copies (valid bit = 0)
 * ........................... "dirty"    -> "shared", but invalidate all shared cached copies
 * */
void writeToMem(int nodeIndex, int cpuIndex, const  string& opCode, int memoryAddress, int reg)
{
    if(opCode == "101011") //opcode for store instruction
    {
        //compute the cache index and the tag
        // I convert the tag to a bool array for easy comparison
        int cacheIndex = memoryAddress % 4;
        int iTag = memoryAddress / 4;
        bool tag [4];
        tag [3] = iTag % 2 == 1;
        iTag /= 2;
        tag [2] = iTag % 2 == 1;
        iTag /= 2;
        tag [1] = iTag % 2 == 1;
        iTag /= 2;
        tag [0] = iTag % 2 == 1;

        int otherCPU = (cpuIndex + 1) % 2;
        int homeNode = memoryAddress / 16;
        int localMemIndex = memoryAddress % 16;

        //search local cache
        if  (systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][0]            &&   // valid bit is true
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][1] == tag[0]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][2] == tag[1]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][3] == tag[2]  &&   //check tag field is the same
             systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][4] == tag[3]       //check tag field is the same
            )
        {   //Case 1: write hit
            //Found in local cache
            clockCount++;
            //set home dir to dirty 11
            systemNodes[homeNode]->directory[localMemIndex][0] = true;
            systemNodes[homeNode]->directory[localMemIndex][1] = true;

            //invalidate all cached values of this
            for(int i = 0; i<4; i++)
            {
                if(systemNodes[homeNode]->directory[localMemIndex][i+2])
                {
                    for (int j = 0; j<2; j++)
                    {
                        //if the value in the cache is the one being updated
                        if( systemNodes[i]->caches[j][cacheIndex][0] &&             //check if valid
                            systemNodes[i]->caches[j][cacheIndex][1] == tag[0] &&   //check tag field
                            systemNodes[i]->caches[j][cacheIndex][2] == tag[1] &&
                            systemNodes[i]->caches[j][cacheIndex][3] == tag[2] &&
                            systemNodes[i]->caches[j][cacheIndex][4] == tag[3]
                        )
                            systemNodes[i]->caches[j][cacheIndex][0] = false;       //invalidate all cached values
                    }
                }
            } //end cache invalidation

            //mark local cache as valid
            systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][0] = true;

            //update tag field
            for( int i =0; i<4; i++)
            {
                systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i+1] = tag[i];
            }

            //update register
            for(int i =0; i< 32; i++)
            {
                systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i+5] =     //store in local cache
                    systemNodes[nodeIndex]->registers[cpuIndex][reg][i];    //the value to be used is in the reg
            }
        } //end case 1:  (write-hit)
        else
        { //case 2: write-miss
            //update home memory
            clockCount+=100;
            for(int i =0; i<32; i++)
            {
                systemNodes[homeNode]->memory[localMemIndex][i] = systemNodes[nodeIndex]->registers[cpuIndex][reg][i];
            }

            //invalidate all cached values of this
            for(int i = 0; i<4; i++)
            {
                if(systemNodes[homeNode]->directory[localMemIndex][i+2])
                {
                    for (int j = 0; j<2; j++)
                    {
                        //if the value in the cache is the one being updated
                        if( systemNodes[i]->caches[j][cacheIndex][0] &&             //check if valid
                            systemNodes[i]->caches[j][cacheIndex][1] == tag[0] &&   //check tag field
                            systemNodes[i]->caches[j][cacheIndex][2] == tag[1] &&
                            systemNodes[i]->caches[j][cacheIndex][3] == tag[2] &&
                            systemNodes[i]->caches[j][cacheIndex][4] == tag[3]
                                )
                            systemNodes[i]->caches[j][cacheIndex][0] = false;       //valid bit = 0
                    }
                }
            } //end cache invalidation

            // if the status is "shared" or "uncached" we do nothing BUT...
            // if the status is dirty "11" then we switch it to shared "01"
            if(systemNodes[homeNode]->directory[localMemIndex][0] &&
                systemNodes[homeNode]->directory[localMemIndex][1])
            {
                systemNodes[homeNode]->directory[localMemIndex][0] = false;
                systemNodes[homeNode]->directory[localMemIndex][1] = true;
            }
        } //end case 2 write-miss
    }
}

/* writeBack is used whan a cache block is being replaced
 * if the cached value is valid it will go to the correct memory location and update it
*/
void writeBack(int nodeIndex, int cpuIndex, int cacheIndex)
{
    //check if block to be replaced is valid
    if(systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][0])
    {
        int memoryAddress = 0;
        for(int i =1; i<5; i++)
        {
            //convert tag field into decimal number
            if(systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i])
            {
                memoryAddress *= 2;
                memoryAddress += 1;
            }
        }
        memoryAddress *= 4;
        memoryAddress += cacheIndex;

        int homeNode = memoryAddress/16;
        int localMemAddress = memoryAddress % 16;

        //write it back to memory if the valid block is dirty
        if(systemNodes[homeNode]->directory[localMemAddress][0] && systemNodes[homeNode]->directory[localMemAddress][1])
        for(int i =0; i<32; i++)
        {
            systemNodes[homeNode]->memory[localMemAddress][i] = systemNodes[nodeIndex]->caches[cpuIndex][cacheIndex][i+5];
        }
    }
}



//printAll takes the array of 4 nodes as input displays all the values within the nodes
// this includes registers, caches, memory, directories
void printAll(Node *nodes[])
{
    //loop through all 4 nodes
    for(int i =0; i<4; i++)
    {
        cout<<"\n----------------------------------------";
        cout<<"\nNode #"<<i<<endl;

        //There are two processors
        for(int j =0 ; j<2; j++) {
            cout << "\n-- Processor #" << j <<" --"<<endl;
            for (int k = 0; k < 2; k++)                //each processor has 2 registers
            {
                cout << "$s" << k+1 << ": ";
                //each register has 32 bits
                for (int l = 0; l < 32; l++)
                    cout << nodes[i]->registers[j][k][l];
                cout<<endl;
            }

            cout<<"Cache #: V : Tag  : Data Contents"<<endl;
            for (int k = 0; k < 4; k++)                 //each processor has 4 cache sets
            {
                cout << "Cache " << k << ": ";
                //each cache has 37 bits (1 valid bit: 4 tag field: 32 Data)
                for (int l = 0; l < 37; l++)
                {
                    cout << nodes[i]->caches[j][k][l];

                    if(l==0 || l==4)
                    {
                        cout<<" : ";
                    }

                }cout<<endl;
            }
        }
        cout<<"\n-- Memory --"<<endl;
        for (int j = i * 16; j < (i+1) * 16; ++j)
        {

            cout<<setw(3)<<left<<j<<": ";

            for(int k = 0; k<32; k++)
            {
                cout<<nodes[i]->memory[j%16][k];
            }
            cout<<endl;
        }

        cout<<"\n-- Directory --"<<endl;
        for (int j = 0; j < 16; j++)
        {
            cout<<setw(3)<<left<<j+(i*16)<<": ";
            for(int k = 0; k < 6; k++)
            {
                if(k>=2)
                {
                    cout<<" : ";
                }
                cout<<nodes[i]->directory[j][k];
            }
            cout<<endl;
        }
   } //End Node Loop
}//end printAll


/* initializeSystem will reset the contents of the systems 4 nodes to be all 0s except for in memory
 * the value at each memory location will be the memory address + 5
 *  .... For example Mem[0] = 5, Mem[1] = 6, ... , Mem[62] = 67, Mem[63]= = 68
 * */
void initializeSystem()
{
    clockCount=0;
   for (int i =0; i<4; i++)
   {
        systemNodes[i] = new Node;
        for(int j =0; j<16; j++)
        {
            int memoryValue = i*16 + j + 5;
            bool *binaryMemValue = decimalTo32BitBinary(memoryValue);

            for(int k =0; k<32; k++)
            {
                systemNodes[i]->memory[j][k] = binaryMemValue[k];
            }
        }
   }
}


int main() {
    initializeSystem();
    // This is the main loop to fetch instructions until the end of the file
    int i =1;
    while(!inFile.eof()) {
        fetch();
    }
    printAll(systemNodes);
    cout<<"\n --------------- \nTotal Clock Count: "<<clockCount<<endl;
    return 0;
}
