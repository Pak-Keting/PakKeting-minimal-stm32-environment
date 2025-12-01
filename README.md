# My personal note for minimal STM32 setup (without relying on CubeIDE).

> *Bro... I just wanted to make a quick note on my experiment with Black Pill about setting up the clock tree register, how am I ended up writing this whole documentation?*

This is my first time ever written, what can be considered, a documentation (like I said, I wrote this *accidentally*. I originally intended to wrote a note)


This "note" is *getting out of control*, it has grown so much that I need put a contents table here.

**TABLE OF CONTENTS**
1. [Setting things up](#1-setting-things-up)
    * [1.1. TL;DR](#11-tldr)
    * [1.2. Note for Windows](#12-note-for-windows)
    * [1.3. Flashing and debugging](#13-flashing-and-debugging)
        * [1.3.1. st-link tools](#131-st-link-tools)
        * [1.3.2. Debugging with GDB and OpenOCD](#132-debugging-with-gdb-and-openocd)
    * [1.4. Create a project and generate code on CubeMX](#14-create-a-project-and-generate-code-on-cubemx)
    * [1.5. *Yanking* the generated code](#15-yanking-the-generated-code)
    * [1.6. Modifing CubeMX auto-generated code](#16-modifing-cubemx-auto-generated-code)
        * [1.6.1. `Makefile`](#161-makefile)
        * [1.6.2. `stm32f4xx.h`](#162-stm32f4xxh)
        * [1.6.3. `stm32f411xe.h`](#163-stm32f411xeh)
    * [1.7. Setting up your main code](#17-setting-up-your-main-code)
2. [Setting up the clock for STM32](#2-setting-up-the-clock-for-stm32)
    * [2.1. TL;DR](#21-tldr)
    * [2.2. RCC Registers](#22-rcc-registers)
        * [2.2.1. `CR`](#221-cr)
        * [2.2.2. `CFGR`](#222-cfgr)
        * [2.2.3. `PLLCFGR`](#223-pllcfgr)



## Clarification
I need to clarify a few things if you ever encounter this note in a wild.

First of all, I'm not in any way an expert in embedded programming. Heck, I'm not even at amateur level, I am a complete beginner to this embedded stuff and software in general. So please take this more as a peer-to-peer note.

The second one is, this is based on my experiment with *Black Pill STM32F411CEU6*, so it is possible that this might not applied to other STM32 chip/board.

And last one is that, this aren't really targetted for absolute beginner, since I'll be using the most out of every *jargon* and technical words here and there. As I said earlier, this is originally a note for my future self, **and it still is**. So I'll be using terms and words that I've already know of.

We still be using CubeMX though, but only for visualizing the peripheral and clock tree. And also for "yanking" the generated code as followed:
* register definition header file
* startup.asm
* system.c
* linker script
* Makefile

More about about it [here](#15-yanking-the-generated-code).

And we'll also be using Visual Code for debugging with OpenOCD and GDB, more on that [here](#132-debugging-with-gdb-and-openocd)


## 1. Setting things up

### 1.1. *TL;DR*
0. Install [`msys2`](https://github.com/msys2/msys2-installer/releases) **(only if you're on Windows!)**
1. Download `st-link`, either download the [binaries](https://github.com/stlink-org/stlink/releases) yourself or from your package repo.
1. Install `libusb`, specifically `libusb-1.0` I think?
1. Download [arm gnu toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). If you're confused as to which one you need, most STM32 are AArch32. So, select `arm-none-eabi` and your pc operating system.
1. Download and install [CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html).
1. Create new project on CubeMX and generate the code, we don't care about the IDE. We just here to "yank" `stm32xxx.h`, `startup_stm32xxx.asm`, `system_stm32xx.c`, `STM32XXX.ld`, and `Makefile`. More details [here](#14-create-a-project-and-generate-code-on-cubemx).

Additionally, if you wanted to, you can download [CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) and use it. Honestly, jumping from 8-bit AVR MCU, debugging STM32 feel so surreal. You can literally access every bits of registers and memory, you can even manually setup and toggle the GPIO pin too!

### 1.2. Note for Windows
Install `msys2`, save you some headache later on, since this workflow are heavily depend on linux and gnu tools. You'll need to use something like `make`, `gdb`, `openocd`, `libusb (for st-link)`. Though except for `make`, you can get the Windows version of these binaries online, it just make a life easier to have all of these tools "centralized" inside `msys2`.


### 1.3. Flashing and debugging
#### 1.3.1. st-link tools
We'll be using open source `st-link` CLI tools to flash the MCU. Note that **it need libusb installed** in order for it to work.  
For some reason, it doesn't show `libusb are missing` error whatsoever when I'm running it on terminal as CLI, it just exit, on Windows at least. I only know this by double-click the st-flash.exe directly inside file explorer, only then the error window popping up and shown `libusb-1.0.dll are missing`. I don't remember if it were the same on linux.  
But the baseline is, if you run ` st-flash` on terminal and it doesn't output anything at all, and it just exit. There is a high chance that it is caused by libusb missing/not installed.

Anyway, here's the command for flashing the STM32 *(refer the datasheet for flash memory address! Although most of STM32 flash memory start address are 0x0800_0000, it doesn't hurt to have it confirmed)*
```
st-flash --reset write yourFile.bin 0x08000000
```

**Important Note:**
* **Make sure to use the .bin file!** I'm banging my head off the wall over why the blink doesn't work, turns out I'm using .hex file instead of .bin (due to my muscle memory using avrdude, which uses .hex file. Take note avrdude user!)
* The 4-pins programming header on Blue Pill and Black Pill doesn't include reset pin AFAIK, only `3v3`, `GND`, `SWDIO`, and `SWCLK`. So, don't forget to press the reset button everytime after you flashed the binary. This also gave me a headache as to why the blink doesn't work. Turns out I didn't press reset button.
* If pressing the reset button everytime you flashed the MCU feels cumbersome, you can just wired up the ST-LINK V2 reset pin to `NRST` on the board.
* Remove the `--reset` option if you didn't connect the reset pin to ST-LINK V2. I don't know if it a thing, this is just my gut. Ignore this if you know better. Though if the reset pin weren't connected, you'd need to press the reset button yourself.

#### 1.3.2. Debugging with GDB and OpenOCD

***...WORK IN PROGRESS...***


************************************

### 1.4. Create a project and generate code on CubeMX
Once you create the ST account, download the installer, and done installing CubeMX, go ahead and create a new project based on your chip. If you're using Nucleo or any other official board from ST, you can select your board. But if you're like me using Black or Blue Pill, you need to select the chip manually. In my case, it was STM32F411CEU6.

***Quick note:** Activate the SW (programming) peripheral if you're on Black/Blue Pill board, since this peripheral are disabled by default. It's on `System Core > SYS`, and change the `DEBUG` field to `Serial Wire`. This can avoid you from peripheral conflict later.*

Once you done that, you can `CTRL+S` to save it.  
Yes, you can save CubeMX project at this stage, even if you didn't setup the clock tree and generate the code yet. This is handful if you only wanted to use CubeMX as a visualizer and use other library like libopencm3, or even your own.

After setting up your peripheral and solved the clock tree, you can start generating the code. As for the "Toolchain / IDE" field in Project Manager, you can just select "Makefile". Then click `GENERATE CODE`.

That's all about CubeMX. I won't go into much details about CubeMX, there's plenty of resources out there. Though I might update this section if I see the need for it.


### 1.5. *Yanking* the generated code
Once the code has been generated, you'll have this project folder structure that are auto-generated by the CubeMX.
```
 CUBEMX_GENERATED
 ├── Core
 │   └── ...
 ├── Drivers
 │   └── ...
 ├── Makefile
 ├── STM32F411XX_FLASH.ld
 ├── startup_stm32f411xe.s
 └── CUBEMX_GENERATED.ioc
```
The code that we're going to *yank* are the folowing
```
./Makefile
./STM32F411XX_FLASH.ld
./startup_stm32f411xe.s
./Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h
./Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f411xe.h
./Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
```
`Makefile`: Self-explainatory, it's a Makefile.  
`STM32F411XX_FLASH.ld`: This is the linker script.  
`startup_stm32f411xe.s`: This is a startup code for the MCU, it's in assembly.  
`stm32f4xx.h`: This is a header file for `system_stm32f4xx.c`. It's part of CMSIS I assume?   
`stm32f411xe.h`: This is the code that you *absolutely need*, even if you're planning to make your own HAL, library, or even setting the register bit yourself. It contains all the register and memory map definition.  
`system_stm32f4xx.c`: This is part of the startup file alongside with `startup_stm32f411xe.s`.

The rest of it can be discarded, though it might be a good idea to keep a copy of it just in case if your project structure are messed up.


### 1.6. Modifing CubeMX auto-generated code
There's some file that need to be modified, the following file is:
* `Makefile`
* `stm32f4xx.h`
* `stm32f411xe.h`

#### 1.6.1. `Makefile`
I don't even know what to wrote here, since I heavily modified it that I don't even know where to start. Go see the `Makefile` yourself, maybe change the source and include directory or something? I gave up explaining it here.

#### 1.6.2. `stm32f4xx.h`
Look for the *define tree* like this:
```c
#if defined(STM32F405xx)
#include "stm32f405xx.h"
#elif defined(STM32F415xx)
#include "stm32f415xx.h"
#elif defined(STM32F407xx)
#include "stm32f407xx.h"
...
```
On top of it (the *define tree*), you can define which STM32 you use. In my case, I'll be using STM32F411xE, so I'll define it like this:
```c
#define STM32F411xE  /* <-- This is my define */

#if defined(STM32F405xx)
#include "stm32f405xx.h"
#elif defined(STM32F415xx)
#include "stm32f415xx.h"
...
```
Or, you can just delete the whole *define tree* and include the header directly:
```c
#include "stm32f411xe.h"
```

#### 1.6.3. `stm32f411xe.h`
I commented these relative include:
```c
// #include "system_stm32f4xx.h"
// #include "core_cm4.h"
```
This would result in an error
>"__IO" is not defined

This is because `__IO` is defined inside `core_cm4.h`. To fix it, simply define it:
```c
#define __IO volatile
```

****************************************
### 1.7. Setting up your main code
Inside your code (like main.c or whatever, a code that need to access the register definition), add:
```c
#include "include/stm32f4xx.h"
```

> *Why not just use `stm32f411xe.h`?*

You can, feel free to do so if you wanted to. It just that inside `stm32f4xx.h`, it has `#include "include/stm32f4xx.h"`, remember [this](#162-stm32f4xxh)?  
I just thought it's a good idea to use `stm32f4xx.h` all along. But then again, the choice is yours. Feel free to use either one.

## 2. Setting up the clock for STM32
This is the main reason why I wrote this in the first place, just a note for setting up the clock tree. But somehow I accidentally writing a whole documentation on how to setup my development environment (for my future self, and possibly others).

### 2.1. TL;DR
1. Enable HSE (HSEON), ***if you're using it.***
2. Configure `FLASH` wait states.
3. Configure PLL (PLLCFGR).
4. Enable PLL (PLLON).
5. Set bus prescalers (CFGR).  
6. Switch the SYSCLK to your desired input with SW register.
7. ***Depending on your configuration***, turn off HSI (HSION) if you're not using it.

### 2.2. RCC Registers
It easy to get overwhelmed by the shear amount of the bits and registers on the RCC alone. But fear not, it's actually not that complicated once you understand it. The only RCC register you need to know for now is:
* `CR`
* `CFGR`
* `PLLCFGR`

Yup, only 3 of those, the rest of it can be ignored for the time being.  
Now we'll see  what each register does.

I know this is *cliche* line, but...
> Read the datasheet.

No seriously, you should. And if you find navigating the PDF are troublesome, there's [Okular](https://okular.kde.org/download/) for Windows and Linux, or [Evince](https://wiki.gnome.org/Apps/Evince) (only for Linux). These software are *gamechanger* for me, and it really make reading and nagivating PDF hundreds times better that just using the browser's PDF reader.

#### 2.2.1. `CR`
This are the simplest register to understand. It basically just turning on and off the clock sources. Anything with `xxxON` controls the on-state of the xxx clock source, like `HSEON` for HSE, and `PLLON` for PLL.

`xxxRDY` is a read-only register that tells you if the clock source are ready or not. You can use this register with while loop to check if the clock source is stablized and ready for next instruction:
```c
while(!(RCC->CR & RCC_CR_HSERDY)); /* Checking for ready status of HSE for example */

/* OR */
while(!(RCC->CR & RCC_CR_PLLRDY)); /* Checking for ready status of PLL for example */
```
If you noticed, there's `PLL2`, and this is mainly for I2S (it's for audio stuff). We don't use it most of the time, unless if you wanted to build something that involve high-quality audio like music player or SDcard player. But that is way out of my skill.

There's also `HSEBYP`, this is used if you're going to use "pure signal" on OSC pins instead of crystals. Like a square signal from your function generator for example. Just like `PLL2`, we don't use this ~~most~~ all of the time. You can ignore it if you don't use it.

As for the rest (`HSICAL` and `HSITRIM`), you can find more info about it yourself. We aren't touching those.


#### 2.2.2. `CFGR`
***...WORK IN PROGRESS...***

#### 2.2.3. `PLLCFGR`
`PLL` `P` divisor can be ignored if you aren't using PLL for SYSCLK.

***...WORK IN PROGRESS...***
