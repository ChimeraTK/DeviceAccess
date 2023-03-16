
# function that reformat flags so they become compatible with target_compile_options, target_link_options etc.
function(formatOptionsAsArray outVar inVar)
    string(REGEX REPLACE " +" ";" tmpVar "${inVar}")
    string(REGEX REPLACE "^;+" "" tmpVar "${tmpVar}")
    string(REGEX REPLACE ";+$" "" tmpVar "${tmpVar}")
    set(${outVar} "${tmpVar}" PARENT_SCOPE)
endfunction()
