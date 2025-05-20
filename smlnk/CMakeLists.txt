add_executable(smlnk
        smlnk.c
        )

# pull in common dependencies
target_link_libraries(smlnk pico_stdlib hardware_pwm hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(smlnk)

# add url via pico_set_program_url
example_auto_set_url(smlnk)
