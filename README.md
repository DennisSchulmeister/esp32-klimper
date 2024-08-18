Klimper - ESP32 I²S Synthesizer Test
====================================

1. [Demo](#demo)
1. [Description](#description)
1. [Required Hardware](#required-hardware)
1. [Software Architecture](#software-architecture)
1. [Performance Considerations](#performance-considerations)
1. [How to build and flash](#how-to-build-and-flash)
1. [Copyright](#copyright)

Demo
----

TODO - Now that it is producing sound. :-)

This project is called "Klimper" because when you do nothing, it just plays some random notes
of the C major scale ("klimpern" in German).

Description
-----------

This is a small test project for me to learn a few things about embedded audio DSP programming.
It implements a very basic real-time synthesizer using simple wavetable synthesis, controlled
by a real-time sequencer that triggers random notes of the C-Major scale. My learning goals are:

1. **Espressif IDF:** I have done embedded development with Arduino for different boards, including
   ESP32. But so far I have never used the chip manufacturer's SDK directly. Having a good feel
   for the pros and cons of the Arduino core I want to see how much harder (if any) it is to use
   the Espressif IDF directly. So far the biggest challenge seems to be the documentation which is
   hard to navigate (long pages with no real ToC) and full of translation errors.

1. **C vs C++:** The books on C and C++ are among the oldest in my collection, but most of the time
   I have developed with other languages like ABAP, Java, Python, JavaScript, TypeScript, HTML/CSS
   and more recently Go. Most of the time I have chosen C++ over C. Modern C++ seems like a good idea,
   but I find it hard to approach (so many rules, so little practical context). So let's go back to the
   roots and try to implement a real-world application with plain C, taking into account the learnings
   from all those other languages.

1. **Low-Level Audio DSP:** I chose PureData and Csound to learn audio programming as they abstract
   away many of the low-level details. Yet, I have been reading on low-level audio DSP for many years.
   So let's try to make some noise with plain C/C++.

1. **Floating Point vs. Fixed Point:** The Espressif documentation recommends to use fixed-point math
   whenever possible ([Improving Overall Speed](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/speed.html?highlight=floating%20point#improving-overall-speed)).
   However, some benchmarks seem to suggest comparable performance for both
   ([ESP32 Forum](https://www.esp32.com/viewtopic.php?f=14&t=800&start=20)).
   Let's see how far we can get with conventional floating-point arithmetic.

Required Hardware
-----------------

For this experiment I am using an (now outdated) ESP-WROOM-32 and a PCM5102 DAC. They are cheap and
I have a few of them lying around. :-)

* **GPIO25:** Left/Right Clock (sometimes called Word-Select Clock)
* **GPIO26:** Bit Clock
* **GPIO27:** Serial Data

Serial audio data is sent from the ESP32 to the DAC. We are in half-duplex mode as we are not reading
audio data from an ADC.

Software Architecture
---------------------

The overall design goal is to keep the code simple and easy to understand, yet performant enough
to hopefully get acceptable real-time performance on the ESP32.

The firmware is split into a few modules:

* `main.c`: Startup and glue code
* `audiohw.c`: I²S initialization and interrupt handling
* `synth.c`: A generic synthesizer to generate some sound
* `sequencer.c`: Control logic that "plays" the synthesizer
* `utils.c`: General utility functions
* `dsp/*.c`: Elementary DSP building blocks

`main.c` starts a "DSP task" that is woken up by the audio interrupt whenever a new chunk of audio
must be produced. This task calls the sequencer and the synthesizer to fill a sample buffer that will
then be converted in the audio hardware's native format and copied into the DMA buffer.

DSP processing is using `float` values, as `double` is only implemented in software in the ESP32.
The audio hardware is using 16-bit signed integers in CD quality (44.1 kHz samplerate, stereo).
Audio buffers are left/right interleaved.

Each module except `main.c` follows a semi-object-oriented pattern. There is always a main structure
that needs to be created with `xyz_new()` and freed with `xyz_free()`. All other functions operate
on that structure. For simple objects the `xyz_new()` constructor function takes individual parameters.
For more complex objects parameters are passed as a configuration object, similar as in Espressif IDF.

Pointers given to a function are usually not stored by the function. Most of the time it is safe to allocate
the objects on stack (e.g. the configuration objects) or free the pointer otherwise. The only exception is
the synthesizer given to the sequencer, which of course must exist for as long as the sequencer is active.
And the `xyz_free()` functions with by design remove an object from memory.

Performance Considerations
--------------------------

1. Most code runs from flash memory, which uses `DIO` (double I/O) by default. If the hardware
   supports it, setting this to quad I/O in the `menuconfig` doubles the access speed.

1. Performance critical code can be loaded into IRAM on start-up. But it has only small capacity
   and there is no guarantee that the code remains there forever.

1. `double` calculations are implemented in software as the FPU only supports single-precission
   `float` values. But that should be okay for generic audio DSP.

1. There are mixed opinions on the Internet about the floating point performance of the ESP32.
   The documentation says that integer arithmethic is much faster. But there are benchmarks that
   show only little difference. And the bit-shift after multiplication/division in fixed-point
   math makes both use the same number of cycles on average.

   After implementing this project I must say, I am either bad at writing performant real-time
   code or the ESP32 has rather weak math performance. Probably a little bit of both. :-)

1. ESP32 supports some very basic SIMD operations. But I am not sure if they are actually used
   by the compiler. Need to check this.

How to build and flash
----------------------

1. Install the Espressif IDF if not done, yet.
1. Plug-in the ESP32 via USB.
1. Run `get_idf` once to enable the build environment.
1. Run `idf.py build flash monitor` to compile, flash and monitor the serial output.

Copyright
---------

© 2024 Dennis Schulmeister-Zimolong (github@windows3.de)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
