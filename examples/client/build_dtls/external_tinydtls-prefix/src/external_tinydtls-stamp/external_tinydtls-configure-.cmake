

set(command "/home/pi/1NCE/LwM2M/wakaama/examples/shared/tinydtls/configure;--host=cc")
execute_process(
  COMMAND ${command}
  RESULT_VARIABLE result
  OUTPUT_FILE "/home/pi/1NCE/LwM2M/wakaama/examples/client/build_dtls/external_tinydtls-prefix/src/external_tinydtls-stamp/external_tinydtls-configure-out.log"
  ERROR_FILE "/home/pi/1NCE/LwM2M/wakaama/examples/client/build_dtls/external_tinydtls-prefix/src/external_tinydtls-stamp/external_tinydtls-configure-err.log"
  )
if(result)
  set(msg "Command failed: ${result}\n")
  foreach(arg IN LISTS command)
    set(msg "${msg} '${arg}'")
  endforeach()
  set(msg "${msg}\nSee also\n  /home/pi/1NCE/LwM2M/wakaama/examples/client/build_dtls/external_tinydtls-prefix/src/external_tinydtls-stamp/external_tinydtls-configure-*.log")
  message(FATAL_ERROR "${msg}")
else()
  set(msg "external_tinydtls configure command succeeded.  See also /home/pi/1NCE/LwM2M/wakaama/examples/client/build_dtls/external_tinydtls-prefix/src/external_tinydtls-stamp/external_tinydtls-configure-*.log")
  message(STATUS "${msg}")
endif()
