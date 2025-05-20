if (TARGET tinyusb_device)
    add_executable(deneme
            deneme.c
            )

    # pull in common dependencies
    target_link_libraries(deneme pico_stdlib)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(deneme 1)
    pico_enable_stdio_uart(deneme 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(deneme)

    # add url via pico_set_program_url
    example_auto_set_url(deneme)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
