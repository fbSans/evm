# EVM: Experimenting Virtual Machine (Incomplete)

## **Console** 
### To run examples
```
    $ make
    $ build/easm example/<example>.easm
```
### Basic test to the virtual machine (must print the fibonacci sequence)
```
    $ make
    $ build/evm
```

## **Parts**
### **evm - the virtual machine**
#### **Composed by:**
* **Instruction memory**: where the program is stored
* **Data memory**: where data is stored
* **Data stack**: analogous to registers
* **Call stack**: where return addresses are pushed
* **Data memory**: where data is read and written too  including the static strings
* **Heap Base**: address in the <u>data memory</u> from which you can start to write your custum data

## Labels:
    Labels are names which refer to location in memories relative to where they are placed in the code.
    Easm supports to kinds of labels, because it uses different memories for code and for data (code != data).
    There kinds of labels are:
*       Instruction labels: whose names do not start with dot (.).
        These reference locations in the instruction memory.
        Useful for jumps and call instructions
*       Data labels: whose names start with dot (.). These reference locations in the data memory.
        Useful for data section access
    
*NOTE*: Labels hold 64 bit addresses for 64bit data. So to access `bytes` with them its important to multiply them by 8.

## Instruction behaviors
### Memory Handling
    For now there is no memory allocation mechanisms, this will be treated as higher level mechanisms to be dealt with the programs itself.
    The memory is (maybe a bad decision) viewed by the virtual machine and aligned as a collection of 64 bit words.
    But in order to make easy to handle strings there are for now 8 bit instructions and 64 bit ones.

* puts: prints string from data memory to the standard output. Treats the whole memory has if where bytes. The user must make sure data location is in bytes

The instructions bellow move data between *data memory* and the *data stack*. 
All of them expect address at the top of data stack. remember this because write instructions have two arguments (data and address)
* *write8*: writes in data memory as if where bytes array 
* *read8*: reads in data memory as if where byte array

* *read64*: reads from data memory as if where a 64 bit element array
* *write64*: writes in data memory as if where a 64 bit element array


Each of these memory components are completely separate, and special instructions must be used to transfer data between.

### easm - the assembler
For now it is working as an AOT interpreter, but the goal is to make it generate the bytecode that will be ran by evm. 