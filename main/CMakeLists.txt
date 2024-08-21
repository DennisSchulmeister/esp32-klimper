idf_component_register(
    SRCS "main.c"
         "synth.c"
         "sequencer.c"
         "midi.c"

         "driver/audiohw.c"
         "driver/midiio.c"

         "dsp/adsr.c"
         "dsp/oscil.c"
         "dsp/pan.c"
         "dsp/utils.c"
         "dsp/wavetable.c"

         "ui/ui.c"
         "ui/display.c"

    INCLUDE_DIRS ""
)
