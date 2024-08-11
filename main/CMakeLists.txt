idf_component_register(
    SRCS "main.c"
         "synth.c"
         "sequencer.c"

         "driver/audiohw.c"

         "dsp/adsr.c"
         "dsp/oscil.c"
         "dsp/pan.c"
         "dsp/utils.c"
         "dsp/wavetable.c"
         
    INCLUDE_DIRS ""
)
