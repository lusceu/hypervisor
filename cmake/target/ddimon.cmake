# ------------------------------------------------------------------------------
# Add custom targets for loading hook_identifier driver
# ------------------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
add_custom_target(ddimon_hookidnt_load
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_LIST_DIR}/../../loader/linux sudo make load CMAKE_BINARY_DIR='${CMAKE_BINARY_DIR}'
    VERBATIM
)
else()

add_custom_target(ddimon_hookidnt_load
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_LIST_DIR}/../../example/ddimon/driver/windows/hook_identifier certmgr /add x64/Debug/hook_identifier.cer /s /r localMachine root
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_LIST_DIR}/../../example/ddimon/driver/windows/hook_identifier certmgr /add x64/Debug/hook_identifier.cer /s /r localMachine trustedpublisher
    COMMAND sc.exe create hook_identifier type=kernel binPath=${CMAKE_CURRENT_LIST_DIR}/../../example/ddimon/driver/windows/hook_identifier/x64/Debug/hook_identifier.sys
    COMMAND sc.exe start hook_identifier 
    COMMAND sc.exe stop hook_identifier
    COMMAND sc.exe delete hook_identifier
    VERBATIM
)
endif()