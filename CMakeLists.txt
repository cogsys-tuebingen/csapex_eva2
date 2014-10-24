cmake_minimum_required(VERSION 2.8.3)
project(csapex_eva)

find_package(catkin REQUIRED COMPONENTS
  csapex
)

catkin_package(
#  INCLUDE_DIRS include
  LIBRARIES csapex_eva
  CATKIN_DEPENDS csapex
)

include_directories(
 # include
  ${catkin_INCLUDE_DIRS}
)

csapex_moc_all(QT_MOC_PLUG src/*.h)

add_library(csapex_eva
    src/eva_optimizer.cpp
    src/eva_optimizer_adapter.cpp
    src/optimization_dummy.cpp
    src/parameter_dialog.cpp

    ${QT_MOC_PLUG}
)

target_link_libraries(csapex_eva
    ${catkin_LIBRARIES})

#
# INSTALL
#

install(FILES plugins.xml
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})

install(TARGETS csapex_eva
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION})
        
install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
