# LC3-VM
this project is a virtual machine that emulates the LC-3 (Little Computer 3) architecture. This current implementation only supports I/O for Windows OS but I will make an update for Linux / macOS in the future. This virtual machine can support any Assembly written in the LC-3 Architecture such as the current 2048 game provided from Ryan Pendleton. 

##Setup
To run this project on Windows OS, clone this repository and compile LC3.c via gcc
`gcc src\LC3.c -o lc3-vm`

Once the executable is in our working directory you can play the 2048 game!
`./lc3-vm.exe asm\2048.obj`
