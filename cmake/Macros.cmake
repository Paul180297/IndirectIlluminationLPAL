# ----------
# Add source codes in the folder
# ----------
function(add_folder SOURCES FOLDER_NAME)
    file(GLOB LOCAL_FILES
         "${FOLDER_NAME}/*.cpp" "${FOLDER_NAME}/*.hpp"
         "${FOLDER_NAME}/*.c" "${FOLDER_NAME}/*.h")
    set(${SOURCES} ${${SOURCES}} ${LOCAL_FILES} PARENT_SCOPE)

    source_group(${FOLDER_NAME} FILES ${LOCAL_FILES})
endfunction()
