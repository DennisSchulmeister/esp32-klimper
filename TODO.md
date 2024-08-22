# TODOs

space = open, X = done, * = started

- [X] Finish basic implementation of the synthesizer
- [X] What is causing the audible clicks? -> The random pan on each note on.
- [X] Use LFO for random panning of the voices
- [*] Implement two-oscillator FM with random FM index and ratio, changing both over time
- [X] Hardware button to start/stop the sequencer
- [*] MIDI input to play the synthesizer in real-time from a MIDI keyboard
- [ ] Split voice allocator from synthesizer?
- [ ] Function `oscil_tick()` fine-tune the hard-coded FM limiting value by ear-comparison with a real DX7

## Ideas for performance improvements


* Scale synth wavetable to avoid the multiplication in oscil_tick for FM: sin(x) * wavetable_length

* Like the DX7, use (log2(sin(x)) * wavetable_length) as wavetable to convert multiplications into additions

    * e.g. each time a signal is multiplied with an envelope function

    * the envelope function must also output log2() values for this

    * BEWARE: Negative log! Only half-sine or quarter-sine is typically saved to avoid this

        * http://www.righto.com/2021/12/yamaha-dx7-reverse-engineering-part-iii.html -> Conclusion

        * "For a 10-bit input value n, the corresponding angle is ω = (n + .5)/1024×π/2 (Footnote 4) and
          output value y is -log2(sin(ω)), represented as the integer round(y×1024). (Footnote 5)"

        * Footnote 4: "Note that the input to the ROM is incremented by half a bit. This avoids duplication
          of the 0 value of the waveform when the quarter-wave is mirrored. It also avoids computation of the
          undefined value log(0)"

        * Footnote 5: "The value is rounded to an integer by computing int(y×1024 + .5002). The constant .5002
          rounds the value up, with just a tiny bit more that affects a single entry. I'm not sure why the rounding
          is not exact; perhaps Yamaha used a lower-precision sine or logarithm, which was just enough to change one bit.
          (Note that the value .0002 is somewhat arbitrary; a slightly larger or smaller number will yield the same result.)"

    * Use another exp(x) = 2^x wavetable to convert back to linear before addition

    * Might probably need log-versions of the dsp modules

    * See: https://www.righto.com/2021/11/reverse-engineering-yamaha-dx7.html (Logarithms and exponentials)

* Use a bigger wavetable (DX7 = 4096 samples) to avoid linear interpolation
