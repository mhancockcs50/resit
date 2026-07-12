# Resit ADC Logger

A C99 command-line tool that reads binary ADC sensor logs, performs analysis, and generates a detailed report.

## Why VSCode (instead of CLion)

I chose **Visual Studio Code** over CLion for this project because:
- It's completely free and lightweight.
- Excellent CMake and C/C++ support via official extensions.
- Fast startup and low resource usage on my machine.
- Easy integration with MSYS2/GCC toolchain.
- I already had a good setup with it after the laptop reset.

CLion is great but heavier and requires a license for full features.

## How to Run in VSCode

### 1. Prerequisites
- **MSYS2** installed with GCC and CMake
- Add MSYS2/mingw64/bin to your system PATH

### 2. Open the Project
1. Open VSCode
2. `File → Open Folder` → select this project folder
3. Install recommended extensions if prompted:
   - C/C++ (Microsoft)
   - CMake Tools
   - CMake Language Support

### 3. Build the Project
- Run gcc -std=c99 -Wall -Wextra -pedantic main.c adc.c io.c stats.c -o resit_adc -lm

### 4. Run the Program
1. Run:
   .\resit_adc.exe .\adc_sensor_log.bin