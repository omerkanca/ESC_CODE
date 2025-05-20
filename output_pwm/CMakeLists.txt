add_executable(output_pwm
        output_pwm.c
        )

# pull in common dependencies
target_link_libraries(output_pwm pico_stdlib hardware_pwm hardware_adc)

# create map/bin/hex file etc.
pico_add_extra_outputs(output_pwm)

# add url via pico_set_program_url
example_auto_set_url(output_pwm)
