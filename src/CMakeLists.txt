# Initialise the Pico SDK
pico_sdk_init()

add_executable(firmware 
    picosyn.c 
    interface.c 
    midi.c 
    voice.c 
    envelope.c 
    controls.c
    filter.c
    tables.c
    oscillators.c
)

pico_set_program_name(firmware "picosyn")
pico_set_program_version(firmware "0.1")

pico_enable_stdio_uart(firmware 1)
pico_enable_stdio_usb(firmware 1)

# Add any user requested libraries
target_link_libraries(firmware
    pico_stdlib
    pico_time
    hardware_dma
    hardware_pio
    hardware_timer
    hardware_clocks
    hardware_pwm
    hardware_uart
    hardware_adc
    pico_multicore
    hardware_timer
    m
)

pico_add_extra_outputs(firmware)