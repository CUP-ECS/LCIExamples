cmake_minimum_required(VERSION 3.14)

project(LCIExamples)

include_directories(${LCI_PATH}/include)
link_directories(${LCI_PATH}/lib64)

add_executable(ping_pong_m_queue ping_pong_m_queue.c)
add_executable(ping_pong_l_queue ping_pong_l_queue.c)

target_link_libraries(ping_pong_m_queue lci lct)
target_link_libraries(ping_pong_l_queue lci lct)
