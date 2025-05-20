add_executable(main
        main.c
        )

# pull in common dependencies
target_link_libraries(main pico_stdlib hardware_pwm hardware_irq hardware_clocks pico_multicore hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(main)

# add url via pico_set_program_url
example_auto_set_url(main)
