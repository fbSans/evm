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
* **Execution stack**: analogous to registers
* **Call stack**: where return addresses are pushed
* **Data memory**: where data is read and written too  including the static strings
* **Heap Base**: address in the <u>data memory</u> from which you can start to write your custum data

Each of these memory components are completely separate, and special instructions must be used to transfer data between.

### easm - the assembler
For now it is working as an AOT interpreter, but the goal is to make it generate the bytecode that will be ran by evm. 