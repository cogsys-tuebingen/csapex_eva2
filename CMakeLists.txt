cmake_minimum_required(VERSION 2.8.3)
project(csapex_eva)

find_package(catkin REQUIRED COMPONENTS
  csapex_optimization cslibs_jcppsocket
)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)

catkin_package(
  CATKIN_DEPENDS csapex_optimization
)

include_directories(
 # include
  ${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS}
  ${catkin_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}_node
    src/abstract_optimizer.cpp
    src/optimizer_de.cpp
    src/eva_optimizer.cpp
)

target_link_libraries(${PROJECT_NAME}_node
    ${catkin_LIBRARIES})


add_library(${PROJECT_NAME}_qt
    src/eva_optimizer_adapter.cpp
)

target_link_libraries(${PROJECT_NAME}_qt
    ${catkin_LIBRARIES})

#
# INSTALL
#

install(FILES plugins.xml
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS ${PROJECT_NAME}_node ${PROJECT_NAME}_qt
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
