#######################################################################################################################
# Generate the Doxygen page listing all change request documents.
#
# A Doxygen page 'change_requests' is generated from the markdown files in doc/change-requests/ during the CMake
# configuration step, so it does not need to be maintained by hand. The generated page is written to the build
# directory and added to the Doxygen INPUT through the variable DOXYGEN_EXTRA_INPUT (see cmake/Doxyfile.in).
#
# Each change request document is a markdown page whose name Doxygen derives from the path relative to the input
# root (STRIP_FROM_PATH in cmake/Doxyfile.in), i.e. md_doc_2change-requests_2<cleaned file name without extension>.
# The <cleaned file name> is the lower-cased file name with an underscore inserted before every letter that was
# upper-case in the original name (Doxygen's clean-name scheme, used because CASE_SENSE_NAMES is NO). The documents
# are linked from the generated page with \ref commands using these page names.
#
#######################################################################################################################

# Add an underscore before every upper-case letter and lower-case the result, as Doxygen's clean-name scheme does.
function(clean_doxygen_file_name input_name output_name)
  set(cleaned "")
  string(LENGTH "${input_name}" name_length)
  set(index 0)
  while(index LESS name_length)
    string(SUBSTRING "${input_name}" ${index} 1 current_character)
    if(current_character MATCHES "[A-Z]")
      string(APPEND cleaned "_")
    endif()
    string(TOLOWER "${current_character}" current_character)
    string(APPEND cleaned "${current_character}")
    math(EXPR index "${index}+1")
  endwhile()
  set(${output_name} "${cleaned}" PARENT_SCOPE)
endfunction()

file(GLOB change_request_documents "${CMAKE_CURRENT_SOURCE_DIR}/doc/change-requests/*.md")
list(SORT change_request_documents)

set(CR_DOXYGEN_LIST "")
foreach(change_request IN LISTS change_request_documents)
  get_filename_component(change_request_stem "${change_request}" NAME_WE)
  clean_doxygen_file_name("${change_request_stem}" change_request_page_name)
  string(APPEND CR_DOXYGEN_LIST "\\li \\ref md_doc_2change-requests_2${change_request_page_name}\n")
endforeach()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/changeRequests.dox.in"
               "${CMAKE_CURRENT_BINARY_DIR}/generated/changeRequests.dox" @ONLY)

# make the generated page part of the Doxygen INPUT
set(DOXYGEN_EXTRA_INPUT "${CMAKE_CURRENT_BINARY_DIR}/generated")
