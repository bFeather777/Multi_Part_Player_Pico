cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH "/Users/leorio0828/pico-sdk")

# 引導 Pico SDK
set(PICO_PLATFORM rp2350)
include(pico_sdk_import.cmake)

project(Multi_Part_Player C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 初始化 SDK
pico_sdk_init()

# 建立執行檔
add_executable(Multi_Part_Player
    main.cpp
    main.h
    score_library.h
)

target_include_directories(Multi_Part_Player PUBLIC lib)

# 連結標準函式庫 (這樣 #include "pico/stdlib.h" 才不會報錯)
target_link_libraries(Multi_Part_Player
    pico_stdlib 
    hardware_spi 
    hardware_gpio 
    hardware_pwm

)

# 你的專案名稱如果是 Multi_Part_Player
pico_enable_stdio_usb(Multi_Part_Player 1)
pico_enable_stdio_uart(Multi_Part_Player 0)

# 產生燒錄用的 .uf2 檔案
pico_add_extra_outputs(Multi_Part_Player) 