target_sources(
  firmware
  PRIVATE frame_icon_qr_prompt.cpp
          frame_progress_prompt.cpp
          frame_prompt.cpp
          frame_qr_prompt.cpp
          frame_text_image_prompt.cpp
          frame_text_prompt.cpp
          frame_title_text_image_prompt.cpp
          frame_wait.cpp
          frame_wait_temp.cpp
  )

if(HAS_ANFC)
  target_sources(firmware PRIVATE frame_opt_prompt.cpp)
endif()
