add_library(mpisan_rt SHARED mpisan_rt.cpp)
target_link_libraries(mpisan_rt PUBLIC MPI::MPI_CXX)
