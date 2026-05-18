# Building the LLVM pass as a loadable module
add_library(MPISanPass MODULE MPISanPass.cpp)

# Windows MSVC might need additional flags for LLVM passes, 
# but MODULE usually handles the basics.
if (APPLE)
  set_target_properties(MPISanPass PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

# We don't link against LLVM libraries directly for loadable passes 
# in most setups, they use symbols provided by the 'opt' or 'clang' binary.
