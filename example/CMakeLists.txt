add_executable(consumer_producer consumer_producer.cpp)
add_executable(consumer_producer_dynamic consumer_producer_dynamic.cpp)

target_link_libraries(consumer_producer wolf_sim)
target_link_libraries(consumer_producer_dynamic wolf_sim)