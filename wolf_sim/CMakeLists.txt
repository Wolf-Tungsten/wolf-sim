file(GLOB_RECURSE WOLF_SIM_SRC "*.cpp")

add_library(wolf_sim STATIC ${WOLF_SIM_SRC})