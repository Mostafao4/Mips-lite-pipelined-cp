# MIPS-Lite 🚀: A 5-Stage Pipelined CPU Simulator
🚀 Pipelined Processor Simulator in C

Welcome to MIPS-Lite: A Pipelined CPU Simulator! This project is a cycle-accurate simulation of a custom 32-bit RISC processor with a 5-stage pipeline (IF, ID, EX, MEM, WB). Designed for educational purposes, it emulates a Von Neumann architecture with a unified memory system and supports 12 unique instructions, including arithmetic operations, branches, jumps, and memory access.

🔧 Key Features:

✅ 5-Stage Pipeline with hazard handling (control hazards only)

📜 12 Supported Instructions (ADD, SUB, MUL, MOVI, JEQ, AND, XORI, JMP, LSL, LSR, MOVR, MOVM)

⚡ Branch/Jump Handling with instruction flushing

📊 Detailed Debugging Outputs (register/memory updates per cycle)

📂 Assembly Program Loading from text files

🛠 Technical Highlights:

2048-word memory (1024 for instructions, 1024 for data)

33 Registers (R0 hardwired to 0, R1-R31 general-purpose, PC)

Pipeline Stalling when MEM and IF conflict

Signed/Unsigned Immediate Support (2's complement, except shifts)

(NOT SOLVING DATA HAZARD)

🎯 Perfect for:

Computer Architecture students

Pipeline design enthusiasts

Anyone curious about CPU simulation!

