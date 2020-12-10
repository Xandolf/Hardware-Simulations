// Booths Alg Simulation
// Created By Xander Winans on 2020/10/10
/*
    This program is designed to simulate booths algorithm to multiply two 16-bit numbers. It does this at the hardware level.
    The prorgam asks the user to input these numbers through a terminal. 
    Here is how you can run this program on a linux based computer. You may need to adjust the steps based on your system setup.
        Navigate to the directory that contains Booths_16bit.cpp 
        Compile it with $> g++ Booths_16bit.cpp -o boothser
        Run it with     $> ./boothser
        Follow the prompt and enter two 16bit binary numbers. 
            This program is a simulation and doesn't contain input validation, please ensure you enter the desired number correctly.
        To learn more about Booths algorithm visit https://en.wikipedia.org/wiki/Booth%27s_multiplication_algorithm
 */


#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

//Since numbers are stores with the lowest bit in the 0's place 
//I need to print starting at the higher order bit.
string binaryToString(static bool* binaryNumber)
{
    string result = "";
    for (int i = 0; i < 16; i++)
    {
        if (binaryNumber[15 - i] == true) result.append(1, '1');
        else result.append(1,'0');
    }
    return result;
}


//Simulates a 1-bit full adder
//The output is an array of 2 bools in form [sum, carryOut]
bool * OneBitFullAdder(bool a, bool b, bool carryIn)
{
    bool sum, carryOut;

    sum = (a != b) != carryIn;
    carryOut = (a && b) || ((a != b) && carryIn);

    static bool result [2];
    result[0] = sum;
    result[1] = carryOut;
    return result;
}

// Simulates a 1-bit ALU
// returns a bool array of size 2
// Format of return [result, carryOut]
// Less function has not been implemented
bool* ALUOneBit (static bool a,static bool b, bool aInvert, bool bInvert, bool carryIn, bool* operation)
{
    static bool result[2];
    bool op0, op1, op2, carryOut;
    bool* adder;
    //check inversion
    if (aInvert) a = !a;
    if (bInvert) b = !b;
    //perform all operations
    op0 = a && b;       

    op1 = a || b;       
    
    adder = OneBitFullAdder(a, b, carryIn);
    op2 = adder[0];
    result[1] = adder[1];                   //carry Out
   
    //Selected with 4 way MUX
    if (!operation[0] && !operation[1])     // AND case 00
        result[0] = op0;
    else if (!operation[0] && operation[1]) // OR case 01
        result[0] = op1;
    else if (operation[0] && !operation[1])  //ADD case 10
        result[0] = op2;

    return result;
}


// Simulates a 1-bit ALU with overflow
// returns a bool array of size 3
// Format of return [result, carryOut, overFlow]
// Less function has not been implemented
// This is created because the last ALU is structurally different than the others and includes overflow 
bool* ALUOneBitWithOF(bool a, bool b, bool aInvert, bool bInvert, bool carryIn, bool* operation)
{
   static bool result[3];
   //use basic alu to get 
   bool* tempResults = ALUOneBit(a, b, aInvert, bInvert, carryIn, operation);
   result[0] = tempResults[0];
   result[1] = tempResults[1];

   //calculate overflow
   result[2] = !(a != b) && (result[0] != b);
   return result;
}

//Currently bits 0-15 represent binary number with bit 0 being the lowest order bit
// bit at result[16] is the overflow bit
bool* ALU16Bit(bool* a, bool* b, bool aInv, bool bInv, bool* operation)
{
    static bool result[17]; //16 bit result plus overflow
    bool overflow;
    bool* tempResult;

    //first carryIn is bInverse
    bool carryIn = bInv;

    //first 15 ALUs dont need overflow
    for (int i = 0; i <15 ; i++)
    {
        tempResult = ALUOneBit(a[i], b[i], aInv, bInv, carryIn, operation);
        result[i] = tempResult[0];
        carryIn = tempResult[1];    
    }

    tempResult = ALUOneBitWithOF(a[15], b[15], aInv, bInv, carryIn, operation);
    result[15] = tempResult[0];
    result[16] = tempResult[2];
    return result;
}

//Currently bits 0-3 represent binary number with bit 0 being lower order bit
//This is used to simulate the CPU Cycle Counter
bool* ALU4Bit(bool* a, bool* b, bool aInv, bool bInv, bool* operation)
{
    bool result[4]; //16 bit result plus overflow
    bool* tempResult;

    //first carryIn is bInverse
    bool carryIn = bInv;

    
    for (int i = 0; i < 4; i++)
    {
        tempResult = ALUOneBit(a[i], b[i], aInv, bInv, carryIn, operation);
        result[i] = tempResult[0];
        carryIn = tempResult[1];
    }

    return result;
}

//Simulates Booths algorithm multiplying MD*MQ
//stores the result in AC MQ
void booths(static bool* opA,static bool* opB)
{
   
    bool* MD = opA;
    bool* MQ = opB;
   
    bool mqv = false;
    bool cycleCounter[4] = { true,true,true,true};


    bool ac[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    bool* AC = ac;
    
    //Output header 
    std::cout <<setw(14)<< "cycle-counter"<<setw(3)<<" | "<<setw(18)<< "MD"<<setw(3)<<" | "
        << setw(18) << "AC" <<setw(3)<<" | "
        << setw(18) << "MQ" <<setw(3)<<" | " 
        << setw(5) << "MQ-1"<<setw(3)<<" | "<< "Comment" << endl;


    //output initialization
    std::cout << setw(11) << cycleCounter[3] << cycleCounter[2] << cycleCounter[1] << cycleCounter[0] << setw(3) << " | "
        << setw(18)<<binaryToString(MD) << setw(3)<<" | " 
        << setw(18)<<binaryToString(AC) << setw(3)<<" | " 
        << setw(18)<<binaryToString(MQ) << setw(3)<<" | " 
        << setw(5)<<mqv << setw(3)<<" | "<< "Initialize"<<endl;
    int i = 0;
    bool enteredLoop= false;
    //16 iterations
    while (!(enteredLoop && cycleCounter[0] && cycleCounter[1] && cycleCounter[2] && cycleCounter [3]))
    {
        i++;
        enteredLoop = true;
        //Add Case
        if (!MQ[0] && mqv)                      
        {
            static bool op[2] = { true,false };
            AC = ALU16Bit(AC, MD, 0, 0, op);    //add (AC <- AC + MD)
            std::cout << setw(11) << cycleCounter[3] << cycleCounter[2] << cycleCounter[1] << cycleCounter[0] << setw(3) << " | "
                << setw(18) << binaryToString(MD) << setw(3) << " | "
                << setw(18) << binaryToString(AC) << setw(3) << " | "
                << setw(18) << binaryToString(MQ) << setw(3) << " | "
                << setw(5) << mqv << setw(3) << " | " << "AC <- AC + MD    ";

        }
        //Sub Case
        else if (MQ[0] && !mqv)                 // Sub case
        {
            static bool op[2] = { true,false };
            AC = ALU16Bit(AC, MD, 0, 1, op);    //sub (AC <- AC - MD
            std::cout << setw(11) << cycleCounter[3] << cycleCounter[2] << cycleCounter[1] << cycleCounter[0] << setw(3) << " | "
                << setw(18) << binaryToString(MD) << setw(3) << " | "
                << setw(18) << binaryToString(AC) << setw(3) << " | "
                << setw(18) << binaryToString(MQ) << setw(3) << " | "
                << setw(5) << mqv << setw(3) << " | " << "AC <- AC - MD    ";
        }
        // Do Nothing Case
        else
        {
            std::cout << setw(11) << cycleCounter[3] << cycleCounter[2] << cycleCounter[1] << cycleCounter[0] << setw(3) << " | "
                << setw(18) << binaryToString(MD) << setw(3) << " | "
                << setw(18) << binaryToString(AC) << setw(3) << " | "
                << setw(18) << binaryToString(MQ) << setw(3) << " | "
                << setw(5) << mqv << setw(3) << " | " << "Do Nothing       ";
        }
        cout << "Step: 1 | Iteration: " << i << endl;

        //shift bits (signed)
        mqv = MQ[0];
        for (int i = 1; i < 16; i++)
        {
            MQ[i - 1] = MQ[i];    
        }
        MQ[15] = AC[0];
        for (int i = 1; i < 15; i++)
        {
            AC[i - 1] = AC[i];
        }
        //no need to sign extend from AC[14] since AC[14] == AC[15]

        //Output after shifting
        std::cout << setw(11) << cycleCounter[3] << cycleCounter[2] << cycleCounter[1] << cycleCounter[0] << setw(3) << " | "
            << setw(18) << binaryToString(MD) << setw(3) << " | "
            << setw(18) << binaryToString(AC) << setw(3) << " | "
            << setw(18) << binaryToString(MQ) << setw(3) << " | "
            << setw(5) << mqv << setw(3) << " | " << "Shift 1 Bit >>   Step: 2 | Iteration: "<< i << endl;
        
        //decrement cycle counter
        bool one[4] = { true,false,false,false };
        static bool op[2] = { true,false };
        bool* temp = ALU4Bit(cycleCounter, one, false, true, op);
        cycleCounter[0] = temp[0];
        cycleCounter[1] = temp[1];
        cycleCounter[2] = temp[2];
        cycleCounter[3] = temp[3];
    }//end of booths, The answer is in 

    //Output final results
    std::cout << "--------------------------------------------------------------------------------------" << endl;
    std::cout << setw(14) << "DONE" << setw(3) << " | "
        << setw(18) << binaryToString(MD) << setw(3) << " | "
        << setw(18) << binaryToString(AC) << setw(3) << " | "
        << setw(18) << binaryToString(MQ) << setw(3) << " | "
        << setw(5) << mqv << setw(3) << " | " << "Final Result" << endl;
}


//Converts a string to boolean array of size 16
//has to flip the order so that result[0] has the lowest order bit
bool* stringTo16Bits(string inputString)
{
    bool result[16];
    for (int i = 0; i < 16; i++)
    {
        result[i] = (inputString.at(15-i) != '0');
        
    }
    return result;
}

//This drives the program. It asks for two 16 bit numbers and then performs Booth's on them
int main()
{
    string inputString;
    std::cout << "Enter 16bit MD: ";
    cin >> inputString;
    bool * tempArray = stringTo16Bits(inputString);
    static bool opA[16];
    for (int i = 0; i < 16; i++)
        opA[i] = tempArray[i];
    
    std::cout << "Enter 16bit MQ: ";
    cin >> inputString;
    tempArray = stringTo16Bits(inputString);
    static bool opB[16];
    for (int i = 0; i < 16; i++)
        opB[i] = tempArray[i];

    booths(opA,  opB);
}


